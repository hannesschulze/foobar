#include "services/network-service.h"
#include <NetworkManager.h>
#include <gtk/gtk.h>
#include <stdint.h>

typedef struct _ApNetworkMembership ApNetworkMembership;

#define SCAN_REFRESH_INTERVAL 10000

//
// FoobarNetwork:
//
// Represents a Wi-Fi network, identified by its SSID. Instances contain a set of NMAccessPoints and the strength is
// updated to match that of the best available AP.
//

struct _FoobarNetwork
{
	GObject     parent_instance;
	GBytes*     ssid;
	gchar*      name;
	gint        strength;
	gboolean    is_active;
	GHashTable* aps;
};

enum
{
	NETWORK_PROP_NAME = 1,
	NETWORK_PROP_STRENGTH,
	NETWORK_PROP_IS_ACTIVE,
	N_NETWORK_PROPS,
};

static GParamSpec* network_props[N_NETWORK_PROPS] = { 0 };

static void           foobar_network_class_init            ( FoobarNetworkClass* klass );
static void           foobar_network_init                  ( FoobarNetwork*      self );
static void           foobar_network_get_property          ( GObject*            object,
                                                             guint               prop_id,
                                                             GValue*             value,
                                                             GParamSpec*         pspec );
static void           foobar_network_finalize              ( GObject*            object );
static FoobarNetwork* foobar_network_new                   ( void );
static void           foobar_network_set_ssid              ( FoobarNetwork*      self,
                                                             GBytes*             value );
static void           foobar_network_set_active            ( FoobarNetwork*      self,
                                                             gboolean            value );
static void           foobar_network_add_ap                ( FoobarNetwork*      self,
                                                             NMAccessPoint*      ap );
static void           foobar_network_remove_ap             ( FoobarNetwork*      self,
                                                             NMAccessPoint*      ap );
static void           foobar_network_update_strength       ( FoobarNetwork*      self );
static void           foobar_network_handle_strength_change( GObject*            ap,
                                                             GParamSpec*         pspec,
                                                             gpointer            userdata );

G_DEFINE_FINAL_TYPE( FoobarNetwork, foobar_network, G_TYPE_OBJECT )

//
// FoobarNetworkAdapterState:
//
// The current connection/connectivity state of a network adapter.
//

G_DEFINE_ENUM_TYPE(
	FoobarNetworkAdapterState,
	foobar_network_adapter_state,
	G_DEFINE_ENUM_VALUE( FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED, "disconnected" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_NETWORK_ADAPTER_STATE_CONNECTING, "connecting" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_NETWORK_ADAPTER_STATE_CONNECTED, "connected" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_NETWORK_ADAPTER_STATE_LIMITED, "limited" ) )

//
// FoobarNetworkAdapter:
//
// Base class for Wi-Fi/ethernet network adapters.
//

typedef struct _FoobarNetworkAdapterPrivate
{
	NMDevice*                 device;
	NMActiveConnection*       connection;
	FoobarNetworkAdapterState state;
	gulong                    connectivity_handler_id;
	gulong                    connection_handler_id;
	gulong                    connection_state_handler_id;
} FoobarNetworkAdapterPrivate;

enum
{
	ADAPTER_PROP_STATE = 1,
	N_ADAPTER_PROPS,
};

static GParamSpec* adapter_props[N_ADAPTER_PROPS] = { 0 };

static void foobar_network_adapter_class_init              ( FoobarNetworkAdapterClass* klass );
static void foobar_network_adapter_init                    ( FoobarNetworkAdapter*      self );
static void foobar_network_adapter_get_property            ( GObject*                   object,
                                                             guint                      prop_id,
                                                             GValue*                    value,
                                                             GParamSpec*                pspec );
static void foobar_network_adapter_finalize                ( GObject*                   object );
static void foobar_network_adapter_set_device              ( FoobarNetworkAdapter*      self,
                                                             NMDevice*                  value );
static void foobar_network_adapter_update_state            ( FoobarNetworkAdapter*      self );
static void foobar_network_adapter_handle_state_change     ( GObject*                   sender,
                                                             GParamSpec*                pspec,
                                                             gpointer                   userdata );
static void foobar_network_adapter_handle_connection_change( GObject*                   sender,
                                                             GParamSpec*                pspec,
                                                             gpointer                   userdata );

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE( FoobarNetworkAdapter, foobar_network_adapter, G_TYPE_OBJECT )

//
// FoobarNetworkAdapterWired:
//
// Represents an ethernet network adapter.
//

struct _FoobarNetworkAdapterWired
{
	FoobarNetworkAdapter parent_instance;
	NMDeviceEthernet*    device;
	gulong               speed_handler_id;
};

static void                       foobar_network_adapter_wired_class_init         ( FoobarNetworkAdapterWiredClass* klass );
static void                       foobar_network_adapter_wired_init               ( FoobarNetworkAdapterWired*      self );
static void                       foobar_network_adapter_wired_get_property       ( GObject*                        object,
                                                                                    guint                           prop_id,
                                                                                    GValue*                         value,
                                                                                    GParamSpec*                     pspec );
static void                       foobar_network_adapter_wired_finalize           ( GObject*                        object );
static FoobarNetworkAdapterWired* foobar_network_adapter_wired_new                ( NMDeviceEthernet*               device );
static void                       foobar_network_adapter_wired_handle_speed_change( GObject*                        device,
                                                                                    GParamSpec*                     pspec,
                                                                                    gpointer                        userdata );

enum
{
	WIRED_PROP_SPEED = 1,
	N_WIRED_PROPS,
};

static GParamSpec* wired_props[N_WIRED_PROPS] = { 0 };

G_DEFINE_FINAL_TYPE( FoobarNetworkAdapterWired, foobar_network_adapter_wired, FOOBAR_TYPE_NETWORK_ADAPTER )

//
// FoobarNetworkAdapterWifi:
//
// Represents a Wi-Fi network adapter.
//

struct _FoobarNetworkAdapterWifi
{
	FoobarNetworkAdapter parent_instance;
	GHashTable*          ap_networks;
	GHashTable*          named_networks;
	GListStore*          networks;
	GtkFilterListModel*  filtered_networks;
	GtkSortListModel*    sorted_networks;
	FoobarNetwork*       active;
	gboolean             is_scanning;
	NMClient*            client;
	NMDeviceWifi*        device;
	gulong               enabled_handler_id;
	gulong               active_handler_id;
	gulong               added_handler_id;
	gulong               removed_handler_id;
	guint                refresh_source_id;
};

enum
{
	WIFI_PROP_NETWORKS = 1,
	WIFI_PROP_ACTIVE,
	WIFI_PROP_IS_ENABLED,
	WIFI_PROP_IS_SCANNING,
	N_WIFI_PROPS,
};

static GParamSpec* wifi_props[N_WIFI_PROPS] = { 0 };

static void                      foobar_network_adapter_wifi_class_init           ( FoobarNetworkAdapterWifiClass* klass );
static void                      foobar_network_adapter_wifi_init                 ( FoobarNetworkAdapterWifi*      self );
static void                      foobar_network_adapter_wifi_get_property         ( GObject*                       object,
                                                                                    guint                          prop_id,
                                                                                    GValue*                        value,
                                                                                    GParamSpec*                    pspec );
static void                      foobar_network_adapter_wifi_set_property         ( GObject*                       object,
                                                                                    guint                          prop_id,
                                                                                    GValue const*                  value,
                                                                                    GParamSpec*                    pspec );
static void                      foobar_network_adapter_wifi_finalize             ( GObject*                       object );
static FoobarNetworkAdapterWifi* foobar_network_adapter_wifi_new                  ( NMClient*                      client,
                                                                                    NMDeviceWifi*                  device );
static void                      foobar_network_adapter_wifi_add_ap_to_network    ( FoobarNetworkAdapterWifi*      self,
                                                                                    NMAccessPoint*                 ap );
static void                      foobar_network_adapter_wifi_update_active        ( FoobarNetworkAdapterWifi*      self );
static void                      foobar_network_adapter_wifi_handle_ssid_change   ( GObject*                       ap,
                                                                                    GParamSpec*                    pspec,
                                                                                    gpointer                       userdata );
static void                      foobar_network_adapter_wifi_handle_enabled_change( GObject*                       client,
                                                                                    GParamSpec*                    pspec,
                                                                                    gpointer                       userdata );
static void                      foobar_network_adapter_wifi_handle_active_change ( GObject*                       device,
                                                                                    GParamSpec*                    pspec,
                                                                                    gpointer                       userdata );
static void                      foobar_network_adapter_wifi_handle_added         ( NMDeviceWifi*                  device,
                                                                                    NMAccessPoint*                 ap,
                                                                                    gpointer                       userdata );
static void                      foobar_network_adapter_wifi_handle_removed       ( NMDeviceWifi*                  device,
                                                                                    NMAccessPoint*                 ap,
                                                                                    gpointer                       userdata );
static gboolean                  foobar_network_adapter_wifi_refresh_list         ( gpointer                       userdata );
static gboolean                  foobar_network_adapter_wifi_filter_func          ( gpointer                       item,
                                                                                    gpointer                       userdata );
static gint                      foobar_network_adapter_wifi_sort_func            ( gconstpointer                  item_a,
                                                                                    gconstpointer                  item_b,
                                                                                    gpointer                       userdata );

G_DEFINE_FINAL_TYPE( FoobarNetworkAdapterWifi, foobar_network_adapter_wifi, FOOBAR_TYPE_NETWORK_ADAPTER )

//
// ApNetworkMembership:
//
// Structure representing an AP's membership in a network. The AP is removed when the structure is destroyed.
//

struct _ApNetworkMembership
{
	NMAccessPoint* ap;
	FoobarNetwork* network;
};

static ApNetworkMembership* ap_network_membership_new    ( NMAccessPoint*       ap,
                                                           FoobarNetwork*       network );
static void                 ap_network_membership_destroy( ApNetworkMembership* self );

//
// FoobarNetworkService:
//
// Service monitoring the network state of the computer. This is implemented using the NetworkManager client library.
//

struct _FoobarNetworkService
{
	GObject                    parent_instance;
	NMClient*                  client;
	FoobarNetworkAdapterWired* wired;
	FoobarNetworkAdapterWifi*  wifi;
	FoobarNetworkAdapter*      active;
	gulong                     primary_handler_id;
	gulong                     activating_handler_id;
};

enum
{
	PROP_WIRED = 1,
	PROP_WIFI,
	PROP_ACTIVE,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_network_service_class_init              ( FoobarNetworkServiceClass* klass );
static void foobar_network_service_init                    ( FoobarNetworkService*      self );
static void foobar_network_service_get_property            ( GObject*                   object,
                                                             guint                      prop_id,
                                                             GValue*                    value,
                                                             GParamSpec*                pspec );
static void foobar_network_service_finalize                ( GObject*                   object );
static void foobar_network_service_handle_connection_change( GObject*                   client,
                                                             GParamSpec*                pspec,
                                                             gpointer                   userdata );
static void foobar_network_service_update_active           ( FoobarNetworkService*      self );

G_DEFINE_FINAL_TYPE( FoobarNetworkService, foobar_network_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Wi-Fi Network
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for networks.
//
void foobar_network_class_init( FoobarNetworkClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_network_get_property;
	object_klass->finalize = foobar_network_finalize;

	network_props[NETWORK_PROP_NAME] = g_param_spec_string(
		"name",
		"Name",
		"The advertised SSID of the network.",
		NULL,
		G_PARAM_READABLE );
	network_props[NETWORK_PROP_STRENGTH] = g_param_spec_int(
		"strength",
		"Strength",
		"The strength of the network in percent.",
		0,
		100,
		0,
		G_PARAM_READABLE );
	network_props[NETWORK_PROP_IS_ACTIVE] = g_param_spec_boolean(
		"is-active",
		"Is Active",
		"Indicates whether the network is currently used by the client.",
		FALSE,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_NETWORK_PROPS, network_props );
}

//
// Instance initialization for networks.
//
void foobar_network_init( FoobarNetwork* self )
{
	self->aps = g_hash_table_new_full( g_direct_hash, g_direct_equal, NULL, NULL );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_network_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNetwork* self = (FoobarNetwork*)object;

	switch ( prop_id )
	{
		case NETWORK_PROP_NAME:
			g_value_set_string( value, foobar_network_get_name( self ) );
			break;
		case NETWORK_PROP_STRENGTH:
			g_value_set_int( value, foobar_network_get_strength( self ) );
			break;
		case NETWORK_PROP_IS_ACTIVE:
			g_value_set_boolean( value, foobar_network_is_active( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for networks.
//
void foobar_network_finalize( GObject* object )
{
	FoobarNetwork* self = (FoobarNetwork*)object;

	GHashTableIter iter;
	gpointer ap;
	g_hash_table_iter_init( &iter, self->aps );
	while ( g_hash_table_iter_next( &iter, &ap, NULL ) )
	{
		g_signal_handlers_disconnect_by_data( ap, self );
	}

	g_clear_pointer( &self->ssid, g_bytes_unref );
	g_clear_pointer( &self->name, g_free );
	g_clear_pointer( &self->aps, g_hash_table_unref );

	G_OBJECT_CLASS( foobar_network_parent_class )->finalize( object );
}

//
// Create a new network instance.
//
FoobarNetwork* foobar_network_new( void )
{
	return g_object_new( FOOBAR_TYPE_NETWORK, NULL );
}

//
// Get the advertized SSID of the network.
//
gchar const* foobar_network_get_name( FoobarNetwork* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK( self ), NULL );
	return self->name;
}

//
// Get the current strength percentage value for the network.
//
gint foobar_network_get_strength( FoobarNetwork* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK( self ), 0 );
	return self->strength;
}

//
// Check whether the network is currently used by the client.
//
gboolean foobar_network_is_active( FoobarNetwork* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK( self ), FALSE );
	return self->is_active;
}

//
// Update the advertized SSID of the network.
//
// This also updates the string representation (which may be a lossy conversion).
//
void foobar_network_set_ssid(
	FoobarNetwork* self,
	GBytes*        value )
{
	g_return_if_fail( FOOBAR_IS_NETWORK( self ) );

	if ( self->ssid != value )
	{
		g_clear_pointer( &self->ssid, g_bytes_unref );
		g_clear_pointer( &self->name, g_free );

		if ( value )
		{
			self->ssid = g_bytes_ref( value );
			gsize ssid_len;
			guint8 const* ssid_data = g_bytes_get_data( self->ssid, &ssid_len );
			self->name = nm_utils_ssid_to_utf8( ssid_data, ssid_len );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), network_props[NETWORK_PROP_NAME] );
	}
}

//
// Update the "active" flag of the network. This does not actually activate the network.
//
void foobar_network_set_active(
	FoobarNetwork* self,
	gboolean       value )
{
	g_return_if_fail( FOOBAR_IS_NETWORK( self ) );

	value = !!value;
	if ( self->is_active != value )
	{
		self->is_active = value;
		g_object_notify_by_pspec( G_OBJECT( self ), network_props[NETWORK_PROP_NAME] );
	}
}

//
// Include the specified AP when computing the network's strength.
//
void foobar_network_add_ap(
	FoobarNetwork* self,
	NMAccessPoint* ap )
{
	if ( g_hash_table_add( self->aps, g_object_ref( ap ) ) )
	{
		g_signal_connect( ap, "notify::strength", G_CALLBACK( foobar_network_handle_strength_change ), self );
		foobar_network_update_strength( self );
	}
}

//
// Stop including the specified AP when computing the network's strength.
//
void foobar_network_remove_ap(
	FoobarNetwork* self,
	NMAccessPoint* ap )
{
	if ( g_hash_table_remove( self->aps, ap ) )
	{
		g_signal_handlers_disconnect_by_data( ap, self );
		foobar_network_update_strength( self );
	}
}

//
// Update the network's "strength" property to match that of the best AP for it.
//
void foobar_network_update_strength( FoobarNetwork* self )
{
	gint strength = 0;
	GHashTableIter iter;
	gpointer ap;
	g_hash_table_iter_init( &iter, self->aps );
	while ( g_hash_table_iter_next( &iter, &ap, NULL ) )
	{
		gint ap_strength = nm_access_point_get_strength( ap );
		strength = MAX( strength, ap_strength );
	}

	strength = CLAMP( strength, 0, 100 );
	if ( self->strength != strength )
	{
		self->strength = strength;
		g_object_notify_by_pspec( G_OBJECT( self ), network_props[NETWORK_PROP_STRENGTH] );
	}
}

//
// Called when the strength of an access point for this network has changed.
//
void foobar_network_handle_strength_change(
	GObject*    ap,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)ap;
	(void)pspec;
	FoobarNetwork* self = (FoobarNetwork*)userdata;

	foobar_network_update_strength( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Network Adapter
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for network adapters.
//
void foobar_network_adapter_class_init( FoobarNetworkAdapterClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_network_adapter_get_property;
	object_klass->finalize = foobar_network_adapter_finalize;

	adapter_props[ADAPTER_PROP_STATE] = g_param_spec_enum(
		"state",
		"State",
		"The current connection state of the network.",
		FOOBAR_TYPE_NETWORK_ADAPTER_STATE,
		FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_ADAPTER_PROPS, adapter_props );
}

//
// Instance initialization for network adapters.
//
void foobar_network_adapter_init( FoobarNetworkAdapter* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_network_adapter_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNetworkAdapter* self = (FoobarNetworkAdapter*)object;

	switch ( prop_id )
	{
		case ADAPTER_PROP_STATE:
			g_value_set_enum( value, foobar_network_adapter_get_state( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for network adapters.
//
void foobar_network_adapter_finalize( GObject* object )
{
	FoobarNetworkAdapter* self = (FoobarNetworkAdapter*)object;

	foobar_network_adapter_set_device( self, NULL );

	G_OBJECT_CLASS( foobar_network_adapter_parent_class )->finalize( object );
}

//
// Get the current connection/connectivity state of the adapter.
//
FoobarNetworkAdapterState foobar_network_adapter_get_state( FoobarNetworkAdapter* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_ADAPTER( self ), FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED );

	FoobarNetworkAdapterPrivate* priv = foobar_network_adapter_get_instance_private( self );
	return priv->state;
}

//
// Update the backing network device which is used to derive the adapter's state.
//
void foobar_network_adapter_set_device(
	FoobarNetworkAdapter* self,
	NMDevice*             value )
{
	FoobarNetworkAdapterPrivate* priv = foobar_network_adapter_get_instance_private( self );
	if ( priv->device != value )
	{
		g_clear_signal_handler( &priv->connection_state_handler_id, priv->connection );
		g_clear_signal_handler( &priv->connectivity_handler_id, priv->device );
		g_clear_signal_handler( &priv->connection_handler_id, priv->device );
		g_clear_object( &priv->connection );
		g_clear_object( &priv->device );

		priv->device = value;
		if ( priv->device )
		{
			g_object_ref( priv->device );
			priv->connectivity_handler_id = g_signal_connect(
				priv->device,
				"notify::ip4-connectivity",
				G_CALLBACK( foobar_network_adapter_handle_state_change ), self );
			priv->connection_handler_id = g_signal_connect(
				priv->device,
				"notify::active-connection",
				G_CALLBACK( foobar_network_adapter_handle_connection_change ), self );
			priv->connection = nm_device_get_active_connection( priv->device );
			if ( priv->connection )
			{
				g_object_ref( priv->connection );
				priv->connection_state_handler_id = g_signal_connect(
					priv->connection,
					"notify::state",
					G_CALLBACK( foobar_network_adapter_handle_state_change ),
					self );
			}
		}

		foobar_network_adapter_update_state( self );
	}
}

//
// Derive the connection/connectivity state for the network from the backing device.
//
void foobar_network_adapter_update_state( FoobarNetworkAdapter* self )
{
	FoobarNetworkAdapterPrivate* priv = foobar_network_adapter_get_instance_private( self );

	FoobarNetworkAdapterState state = FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED;
	if ( priv->device )
	{
		NMActiveConnection* connection = nm_device_get_active_connection( priv->device );
		if ( connection )
		{
			switch ( nm_active_connection_get_state( connection ) )
			{
				case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
					state = ( nm_device_get_connectivity( priv->device, AF_INET ) == NM_CONNECTIVITY_FULL )
						? FOOBAR_NETWORK_ADAPTER_STATE_CONNECTED
						: FOOBAR_NETWORK_ADAPTER_STATE_LIMITED;
					break;
				case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
					state = FOOBAR_NETWORK_ADAPTER_STATE_CONNECTING;
					break;
				case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
				case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
				case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
				default:
					state = FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED;
					break;
			}
		}
	}

	if ( priv->state != state )
	{
		priv->state = state;
		g_object_notify_by_pspec( G_OBJECT( self ), adapter_props[ADAPTER_PROP_STATE] );
	}
}

//
// Called when the "ip4-connectivity" property of the NetworkManager device or the "state" property of the active
// connection has changed.
//
static void foobar_network_adapter_handle_state_change(
	GObject*    sender,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)sender;
	(void)pspec;
	FoobarNetworkAdapter* self = (FoobarNetworkAdapter*)userdata;

	foobar_network_adapter_update_state( self );
}

//
// Called when the "active-connection" property of the NetworkManager device has changed.
//
// This updates the state of the adapter and sets the connection up for state change listening.
//
static void foobar_network_adapter_handle_connection_change(
	GObject*    sender,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)sender;
	(void)pspec;
	FoobarNetworkAdapter* self = (FoobarNetworkAdapter*)userdata;
	FoobarNetworkAdapterPrivate* priv = foobar_network_adapter_get_instance_private( self );

	g_clear_signal_handler( &priv->connection_state_handler_id, priv->connection );
	g_clear_object( &priv->connection );

	priv->connection = nm_device_get_active_connection( priv->device );
	if ( priv->connection )
	{
		g_object_ref( priv->connection );
		priv->connection_state_handler_id = g_signal_connect(
			priv->connection,
			"notify::state",
			G_CALLBACK( foobar_network_adapter_handle_state_change ),
			self );
	}

	foobar_network_adapter_update_state( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Wired Network Adapter
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for wired network adapters.
//
void foobar_network_adapter_wired_class_init( FoobarNetworkAdapterWiredClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_network_adapter_wired_get_property;
	object_klass->finalize = foobar_network_adapter_wired_finalize;

	wired_props[WIRED_PROP_SPEED] = g_param_spec_int64(
		"speed",
		"Speed",
		"The supported speed of the adapter in Mbit/s.",
		0,
		INT64_MAX,
		0,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_WIRED_PROPS, wired_props );
}

//
// Instance initialization for wired network adapters.
//
void foobar_network_adapter_wired_init( FoobarNetworkAdapterWired* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_network_adapter_wired_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNetworkAdapterWired* self = (FoobarNetworkAdapterWired*)object;

	switch ( prop_id )
	{
		case WIRED_PROP_SPEED:
			g_value_set_int64( value, foobar_network_adapter_wired_get_speed( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for wired network adapters.
//
void foobar_network_adapter_wired_finalize( GObject* object )
{
	FoobarNetworkAdapterWired* self = (FoobarNetworkAdapterWired*)object;

	g_clear_signal_handler( &self->speed_handler_id, self->device );
	g_clear_object( &self->device );

	G_OBJECT_CLASS( foobar_network_adapter_wired_parent_class )->finalize( object );
}

//
// Create a new wired network adapter wrapping the provided NetworkManager device.
//
FoobarNetworkAdapterWired* foobar_network_adapter_wired_new( NMDeviceEthernet* device )
{
	FoobarNetworkAdapterWired* self = g_object_new( FOOBAR_TYPE_NETWORK_ADAPTER_WIRED, NULL );
	foobar_network_adapter_set_device( FOOBAR_NETWORK_ADAPTER( self ), NM_DEVICE( device ) );

	self->device = g_object_ref( device );
	self->speed_handler_id = g_signal_connect(
		self->device,
		"notify::speed",
		G_CALLBACK( foobar_network_adapter_wired_handle_speed_change ),
		self );
	return self;
}

//
// Get the supported speed of the adapter in Mbit/s.
//
gint64 foobar_network_adapter_wired_get_speed( FoobarNetworkAdapterWired* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIRED( self ), 0 );
	return nm_device_ethernet_get_speed( self->device );
}

//
// Called when the "speed" property of the NetworkManager device changes.
//
void foobar_network_adapter_wired_handle_speed_change(
	GObject*    device,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)device;
	(void)pspec;
	FoobarNetworkAdapterWired* self = (FoobarNetworkAdapterWired*)userdata;

	g_object_notify_by_pspec( G_OBJECT( self ), wired_props[WIRED_PROP_SPEED] );
}

// ---------------------------------------------------------------------------------------------------------------------
// Wireless Network Adapter
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for wireless network adapters.
//
void foobar_network_adapter_wifi_class_init( FoobarNetworkAdapterWifiClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_network_adapter_wifi_get_property;
	object_klass->set_property = foobar_network_adapter_wifi_set_property;
	object_klass->finalize = foobar_network_adapter_wifi_finalize;

	wifi_props[WIFI_PROP_NETWORKS] = g_param_spec_object(
		"networks",
		"Networks",
		"Sorted list of available networks.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	wifi_props[WIFI_PROP_ACTIVE] = g_param_spec_object(
		"active",
		"Active",
		"The currently active network (if any).",
		FOOBAR_TYPE_NETWORK,
		G_PARAM_READABLE );
	wifi_props[WIFI_PROP_IS_ENABLED] = g_param_spec_boolean(
		"is-enabled",
		"Is Enabled",
		"Indicates whether the wifi adapter is currently enabled.",
		FALSE,
		G_PARAM_READWRITE );
	wifi_props[WIFI_PROP_IS_SCANNING] = g_param_spec_boolean(
		"is-scanning",
		"Is Scanning",
		"Indicates whether the wifi adapter is currently scanning for available networks.",
		FALSE,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_WIFI_PROPS, wifi_props );
}

//
// Instance initialization for wireless network adapters.
//
void foobar_network_adapter_wifi_init( FoobarNetworkAdapterWifi* self )
{
	self->ap_networks = g_hash_table_new_full(
		g_direct_hash,
		g_direct_equal,
		g_object_unref,
		(GDestroyNotify)ap_network_membership_destroy );
	self->named_networks = g_hash_table_new_full(
		g_bytes_hash,
		g_bytes_equal,
		(GDestroyNotify)g_bytes_unref,
		g_object_unref );
	self->networks = g_list_store_new( FOOBAR_TYPE_NETWORK );

	GtkCustomFilter* filter = gtk_custom_filter_new( foobar_network_adapter_wifi_filter_func, NULL, NULL );
	self->filtered_networks = gtk_filter_list_model_new(
		G_LIST_MODEL( g_object_ref( self->networks ) ),
		GTK_FILTER( filter ) );

	GtkCustomSorter* sorter = gtk_custom_sorter_new( foobar_network_adapter_wifi_sort_func, NULL, NULL );
	self->sorted_networks = gtk_sort_list_model_new(
		G_LIST_MODEL( g_object_ref( self->filtered_networks ) ),
		GTK_SORTER( sorter ) );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_network_adapter_wifi_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)object;

	switch ( prop_id )
	{
		case WIFI_PROP_NETWORKS:
			g_value_set_object( value, foobar_network_adapter_wifi_get_networks( self ) );
			break;
		case WIFI_PROP_ACTIVE:
			g_value_set_object( value, foobar_network_adapter_wifi_get_active( self ) );
			break;
		case WIFI_PROP_IS_ENABLED:
			g_value_set_boolean( value, foobar_network_adapter_wifi_is_enabled( self ) );
			break;
		case WIFI_PROP_IS_SCANNING:
			g_value_set_boolean( value, foobar_network_adapter_wifi_is_scanning( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_network_adapter_wifi_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)object;

	switch ( prop_id )
	{
		case WIFI_PROP_IS_ENABLED:
			foobar_network_adapter_wifi_set_enabled( self, g_value_get_boolean( value ) );
			break;
		case WIFI_PROP_IS_SCANNING:
			foobar_network_adapter_wifi_set_scanning( self, g_value_get_boolean( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for wireless network adapters.
//
void foobar_network_adapter_wifi_finalize( GObject* object )
{
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)object;

	GPtrArray const* aps = nm_device_wifi_get_access_points( self->device );
	for ( guint i = 0; i < aps->len; ++i )
	{
		NMAccessPoint* ap = g_ptr_array_index( aps, i );
		g_signal_handlers_disconnect_by_data( ap, self );
	}

	g_clear_handle_id( &self->refresh_source_id, g_source_remove );
	g_clear_signal_handler( &self->enabled_handler_id, self->client );
	g_clear_signal_handler( &self->active_handler_id, self->device );
	g_clear_signal_handler( &self->added_handler_id, self->device );
	g_clear_signal_handler( &self->removed_handler_id, self->device );
	g_clear_object( &self->client );
	g_clear_object( &self->device );
	g_clear_object( &self->sorted_networks );
	g_clear_object( &self->filtered_networks );
	g_clear_object( &self->networks );
	g_clear_object( &self->active );
	g_clear_pointer( &self->named_networks, g_hash_table_unref );
	g_clear_pointer( &self->ap_networks, g_hash_table_unref );

	G_OBJECT_CLASS( foobar_network_adapter_wifi_parent_class )->finalize( object );
}

//
// Create a new wireless network adapter wrapping the provided NetworkManager device.
//
FoobarNetworkAdapterWifi* foobar_network_adapter_wifi_new(
	NMClient*     client,
	NMDeviceWifi* device )
{
	FoobarNetworkAdapterWifi* self = g_object_new( FOOBAR_TYPE_NETWORK_ADAPTER_WIFI, NULL );
	foobar_network_adapter_set_device( FOOBAR_NETWORK_ADAPTER( self ), NM_DEVICE( device ) );

	self->client = g_object_ref( client );
	self->device = g_object_ref( device );
	self->enabled_handler_id = g_signal_connect(
		self->client,
		"notify::wireless-enabled",
		G_CALLBACK( foobar_network_adapter_wifi_handle_enabled_change ),
		self );
	self->active_handler_id = g_signal_connect(
		self->device,
		"notify::active-access-point",
		G_CALLBACK( foobar_network_adapter_wifi_handle_active_change ),
		self );
	self->added_handler_id = g_signal_connect(
		self->device,
		"access-point-added",
		G_CALLBACK( foobar_network_adapter_wifi_handle_added ),
		self );
	self->removed_handler_id = g_signal_connect(
		self->device,
		"access-point-removed",
		G_CALLBACK( foobar_network_adapter_wifi_handle_removed ),
		self );

	GPtrArray const* aps = nm_device_wifi_get_access_points( self->device );
	for ( guint i = 0; i < aps->len; ++i )
	{
		NMAccessPoint* ap = g_ptr_array_index( aps, i );
		g_signal_connect(
			ap,
			"notify::ssid",
			G_CALLBACK( foobar_network_adapter_wifi_handle_ssid_change ),
			self );
		foobar_network_adapter_wifi_add_ap_to_network( self, ap );
	}

	foobar_network_adapter_wifi_update_active( self );
	return self;
}

//
// Get a sorted list of available networks.
//
GListModel* foobar_network_adapter_wifi_get_networks( FoobarNetworkAdapterWifi* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIFI( self ), NULL );
	return G_LIST_MODEL( self->sorted_networks );
}

//
// Get the currently active network (if any).
//
FoobarNetwork* foobar_network_adapter_wifi_get_active( FoobarNetworkAdapterWifi* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIFI( self ), NULL );
	return self->active;
}

//
// Check whether the wifi adapter is currently enabled.
//
gboolean foobar_network_adapter_wifi_is_enabled( FoobarNetworkAdapterWifi* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIFI( self ), FALSE );
	return nm_client_wireless_get_enabled( self->client );
}

//
// Check whether the wifi adapter is currently scanning for available networks.
//
gboolean foobar_network_adapter_wifi_is_scanning( FoobarNetworkAdapterWifi* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIFI( self ), FALSE );
	return self->is_scanning;
}

//
// Update whether the wifi adapter is currently enabled.
//
void foobar_network_adapter_wifi_set_enabled(
	FoobarNetworkAdapterWifi* self,
	gboolean                  value )
{
	g_return_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIFI( self ) );

	if ( !value ) { foobar_network_adapter_wifi_set_scanning( self, FALSE ); }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	nm_client_wireless_set_enabled( self->client, value );
#pragma GCC diagnostic pop
}

//
// Update whether the wifi adapter is currently scanning for available networks.
//
void foobar_network_adapter_wifi_set_scanning(
	FoobarNetworkAdapterWifi* self,
	gboolean                  value )
{
	g_return_if_fail( FOOBAR_IS_NETWORK_ADAPTER_WIFI( self ) );

	value = value && foobar_network_adapter_wifi_is_enabled( self );
	if ( self->is_scanning != value )
	{
		self->is_scanning = value;
		g_object_notify_by_pspec( G_OBJECT( self ), wifi_props[WIFI_PROP_IS_SCANNING] );

		// Periodically start scans until "is-scanning" is disabled again.

		g_clear_handle_id( &self->refresh_source_id, g_source_remove );
		if ( self->is_scanning )
		{
			foobar_network_adapter_wifi_refresh_list( self );
			self->refresh_source_id = g_timeout_add(
				SCAN_REFRESH_INTERVAL,
				foobar_network_adapter_wifi_refresh_list,
				self );
		}
	}
}

//
// Ensure that a network for the specified access point exists.
//
// If the AP has an SSID, it is added to the network for this SSID. Otherwise, a new network for this AP is created just
// for this network.
//
void foobar_network_adapter_wifi_add_ap_to_network(
	FoobarNetworkAdapterWifi* self,
	NMAccessPoint*            ap )
{
	// The AP is automatically added/removed from the network when using g_hash_table_replace because of the
	// ApNetworkMembership destroy function.

	GBytes* ssid = nm_access_point_get_ssid( ap );
	if ( ssid )
	{
		FoobarNetwork* network = g_hash_table_lookup( self->named_networks, ssid );
		if ( !network )
		{
			network = foobar_network_new( );
			foobar_network_set_ssid( network, ssid );
			g_list_store_append( self->networks, network );
			g_hash_table_insert( self->named_networks, g_bytes_ref( ssid ), network );
		}

		g_hash_table_replace( self->ap_networks, g_object_ref( ap ), ap_network_membership_new( ap, network ) );
	}
	else
	{
		g_autoptr( FoobarNetwork ) network = foobar_network_new( );
		g_hash_table_replace( self->ap_networks, g_object_ref( ap ), ap_network_membership_new( ap, network ) );
	}

	gtk_filter_changed( gtk_filter_list_model_get_filter( self->filtered_networks ), GTK_FILTER_CHANGE_DIFFERENT );
}

//
// Update the active network based on the active access point.
//
void foobar_network_adapter_wifi_update_active( FoobarNetworkAdapterWifi* self )
{
	NMAccessPoint* active_ap = nm_device_wifi_get_active_access_point( self->device );
	ApNetworkMembership* membership = active_ap ? g_hash_table_lookup( self->ap_networks, active_ap ) : NULL;
	FoobarNetwork* new_active = membership ? membership->network : NULL;

	if ( self->active != new_active )
	{
		if ( self->active )
		{
			foobar_network_set_active( self->active, FALSE );
		}
		g_clear_object( &self->active );

		self->active = new_active;
		if ( self->active )
		{
			g_object_ref( self->active );
			foobar_network_set_active( self->active, TRUE );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), wifi_props[WIFI_PROP_ACTIVE] );
	}
}

//
// Called when the SSID for an access point changes.
//
// This will remove the access point from the old network and associate it with its new network.
//
void foobar_network_adapter_wifi_handle_ssid_change(
	GObject*    ap,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)pspec;
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)userdata;

	foobar_network_adapter_wifi_add_ap_to_network( self, NM_ACCESS_POINT( ap ) );
	foobar_network_adapter_wifi_update_active( self );
}

//
// Called when the "wireless-enabled" state of the client changes.
//
void foobar_network_adapter_wifi_handle_enabled_change(
	GObject*    client,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)client;
	(void)pspec;
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)userdata;

	g_object_notify_by_pspec( G_OBJECT( self ), wifi_props[WIFI_PROP_IS_ENABLED] );
}

//
// Called when the active access point of the backing device changes.
//
void foobar_network_adapter_wifi_handle_active_change(
	GObject*    device,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)device;
	(void)pspec;
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)userdata;

	foobar_network_adapter_wifi_update_active( self );
}

//
// Called when a new access point is added.
//
// This will associate the access point with its network.
//
void foobar_network_adapter_wifi_handle_added(
	NMDeviceWifi*  device,
	NMAccessPoint* ap,
	gpointer       userdata )
{
	(void)device;
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)userdata;

	g_signal_connect(
		ap,
		"notify::ssid",
		G_CALLBACK( foobar_network_adapter_wifi_handle_ssid_change ),
		self );
	foobar_network_adapter_wifi_add_ap_to_network( self, ap );
}

//
// Called when an access point is removed.
//
// This will remove it from its network.
//
void foobar_network_adapter_wifi_handle_removed(
	NMDeviceWifi*  device,
	NMAccessPoint* ap,
	gpointer       userdata )
{
	(void)device;
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)userdata;

	g_signal_handlers_disconnect_by_data( ap, self );
	g_hash_table_remove( self->ap_networks, ap ); // Will remove the AP from the network.
	gtk_filter_changed( gtk_filter_list_model_get_filter( self->filtered_networks ), GTK_FILTER_CHANGE_MORE_STRICT );
}

//
// Periodic callback to start a network scan.
//
gboolean foobar_network_adapter_wifi_refresh_list( gpointer userdata )
{
	FoobarNetworkAdapterWifi* self = (FoobarNetworkAdapterWifi*)userdata;

	g_autoptr( GError ) error = NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	if ( !nm_device_wifi_request_scan( self->device, NULL, &error ) )
#pragma GCC diagnostic pop
	{
		g_warning( "Unable to start Wi-Fi network scan: %s", error->message );
	}

	return G_SOURCE_CONTINUE;
}

//
// Filtering callback for the list of networks. Only networks with at least one AP are shown.
//
gboolean foobar_network_adapter_wifi_filter_func(
	gpointer item,
	gpointer userdata )
{
	(void)userdata;

	FoobarNetwork* network = (FoobarNetwork*)item;
	return g_hash_table_size( network->aps ) > 0;
}

//
// Sorting callback for the list of networks.
//
gint foobar_network_adapter_wifi_sort_func(
	gconstpointer item_a,
	gconstpointer item_b,
	gpointer      userdata )
{
	(void)userdata;

	FoobarNetwork* network_a = (FoobarNetwork*)item_a;
	FoobarNetwork* network_b = (FoobarNetwork*)item_b;
	return g_strcmp0( foobar_network_get_name( network_a ), foobar_network_get_name( network_b ) );
}

// ---------------------------------------------------------------------------------------------------------------------
// Access Point Network Memberships
// ---------------------------------------------------------------------------------------------------------------------

//
// Create an association between an access point and the network it belongs to.
//
ApNetworkMembership* ap_network_membership_new(
	NMAccessPoint* ap,
	FoobarNetwork* network )
{
	ApNetworkMembership* self = g_new0( ApNetworkMembership, 1 );
	self->ap = ap;
	self->network = g_object_ref( network );
	foobar_network_add_ap( self->network, self->ap );
	return self;
}

//
// Destroy an association between an access point and its previous network.
//
void ap_network_membership_destroy( ApNetworkMembership* self )
{
	if ( self )
	{
		foobar_network_remove_ap( self->network, self->ap );
		g_object_unref( self->network );
		g_free( self );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the network service.
//
void foobar_network_service_class_init( FoobarNetworkServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_network_service_get_property;
	object_klass->finalize = foobar_network_service_finalize;

	props[PROP_WIRED] = g_param_spec_object(
		"wired",
		"Wired",
		"The wired network adapter (if available).",
		FOOBAR_TYPE_NETWORK_ADAPTER_WIRED,
		G_PARAM_READABLE );
	props[PROP_WIFI] = g_param_spec_object(
		"wifi",
		"WiFi",
		"The wireless network adapter (if available).",
		FOOBAR_TYPE_NETWORK_ADAPTER_WIFI,
		G_PARAM_READABLE );
	props[PROP_ACTIVE] = g_param_spec_object(
		"active",
		"Active",
		"The network adapter currently in use.",
		FOOBAR_TYPE_NETWORK_ADAPTER,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the network service.
//
void foobar_network_service_init( FoobarNetworkService* self )
{
	g_autoptr( GError ) error = NULL;
	self->client = nm_client_new( NULL, &error );
	if ( !self->client )
	{
		g_warning( "Unable to connect to NetworkManager: %s", error->message );
		return;
	}

	NMDeviceEthernet* device_wired = NULL;
	NMDeviceWifi* device_wifi = NULL;
	GPtrArray const* devices = nm_client_get_devices( self->client );
	for ( guint i = 0; i < devices->len; ++i )
	{
		// XXX: Add support for multiple devices
		NMDevice* device = g_ptr_array_index( devices, i );
		if ( !device_wired && NM_IS_DEVICE_ETHERNET( device ) )
		{
			device_wired = NM_DEVICE_ETHERNET( device );
		}
		if ( !device_wifi && NM_IS_DEVICE_WIFI( device ) )
		{
			device_wifi = NM_DEVICE_WIFI( device );
		}
		if ( device_wired && device_wifi )
		{
			break;
		}
	}

	self->wired = device_wired ? foobar_network_adapter_wired_new( device_wired ) : NULL;
	self->wifi = device_wifi ? foobar_network_adapter_wifi_new( self->client, device_wifi ) : NULL;

	foobar_network_service_update_active( self );
	self->primary_handler_id = g_signal_connect(
		self->client,
		"notify::primary-connection",
		G_CALLBACK( foobar_network_service_handle_connection_change ),
		self );
	self->activating_handler_id = g_signal_connect(
		self->client,
		"notify::activating-connection",
		G_CALLBACK( foobar_network_service_handle_connection_change ),
		self );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_network_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNetworkService* self = (FoobarNetworkService*)object;

	switch ( prop_id )
	{
		case PROP_WIRED:
			g_value_set_object( value, foobar_network_service_get_wired( self ) );
			break;
		case PROP_WIFI:
			g_value_set_object( value, foobar_network_service_get_wifi( self ) );
			break;
		case PROP_ACTIVE:
			g_value_set_object( value, foobar_network_service_get_active( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the network service.
//
void foobar_network_service_finalize( GObject* object )
{
	FoobarNetworkService* self = (FoobarNetworkService*)object;

	g_clear_signal_handler( &self->primary_handler_id, self->client );
	g_clear_signal_handler( &self->activating_handler_id, self->client );
	g_clear_object( &self->wifi );
	g_clear_object( &self->wired );
	g_clear_object( &self->client );

	G_OBJECT_CLASS( foobar_network_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new network service instance.
//
FoobarNetworkService* foobar_network_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_NETWORK_SERVICE, NULL );
}

//
// Get the wired network adapter (if available).
//
FoobarNetworkAdapterWired* foobar_network_service_get_wired( FoobarNetworkService* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_SERVICE( self ), NULL );
	return self->wired;
}

//
// Get the wireless network adapter (if available).
//
FoobarNetworkAdapterWifi* foobar_network_service_get_wifi( FoobarNetworkService* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_SERVICE( self ), NULL );
	return self->wifi;
}

//
// Get the network adapter currently in use.
//
FoobarNetworkAdapter* foobar_network_service_get_active( FoobarNetworkService* self )
{
	g_return_val_if_fail( FOOBAR_IS_NETWORK_SERVICE( self ), NULL );
	return self->active;
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when either the primary connection or the activating connection of the client has changed.
//
void foobar_network_service_handle_connection_change(
	GObject*    client,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)client;
	(void)pspec;
	FoobarNetworkService* self = (FoobarNetworkService*)userdata;

	foobar_network_service_update_active( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Update the active network adapter.
//
// This uses the primary connection of the client to determine whether the wireless or wired adapter is used. Otherwise,
// the activating connection is used instead.
//
void foobar_network_service_update_active( FoobarNetworkService* self )
{
	NMActiveConnection* connection = nm_client_get_primary_connection( self->client );
	connection = connection ? connection : nm_client_get_activating_connection( self->client );
	gchar const* type = connection ? nm_active_connection_get_connection_type( connection ) : NULL;

	FoobarNetworkAdapter* new_active;
	if ( !g_strcmp0( type, "802-11-wireless" ) )
	{
		new_active = FOOBAR_NETWORK_ADAPTER( self->wifi );
	}
	else if ( !g_strcmp0( type, "802-3-ethernet" ) )
	{
		new_active = FOOBAR_NETWORK_ADAPTER( self->wired );
	}
	else
	{
		new_active = self->wifi ? FOOBAR_NETWORK_ADAPTER( self->wifi ) : FOOBAR_NETWORK_ADAPTER( self->wired );
	}

	if ( self->active != new_active )
	{
		self->active = new_active;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_ACTIVE] );
	}
}

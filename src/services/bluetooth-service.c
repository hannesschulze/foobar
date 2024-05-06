#include "services/bluetooth-service.h"
#include "dbus/bluez.h"
#include <gtk/gtk.h>

//
// FoobarBluetoothDeviceState:
//
// The current connection state of a remote bluetooth device.
//

G_DEFINE_ENUM_TYPE(
	FoobarBluetoothDeviceState,
	foobar_bluetooth_device_state,
	G_DEFINE_ENUM_VALUE( FOOBAR_BLUETOOTH_DEVICE_STATE_DISCONNECTED, "disconnected" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTING, "connecting" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTED, "connected" ) )

//
// FoobarBluetoothDevice:
//
// Represents an external bluetooth device the client can connect to.
//

struct _FoobarBluetoothDevice
{
	GObject                    parent_instance;
	FoobarBluetoothService*    service;
	FoobarBluetoothDeviceState state;
	FoobarBluezDevice*         device;
	GCancellable*              connect_cancellable;
	gulong                     notify_handler_id;
};

enum
{
	DEVICE_PROP_NAME = 1,
	DEVICE_PROP_STATE,
	N_DEVICE_PROPS,
};

static GParamSpec* device_props[N_DEVICE_PROPS] = { 0 };

static void                   foobar_bluetooth_device_class_init   ( FoobarBluetoothDeviceClass* klass );
static void                   foobar_bluetooth_device_init         ( FoobarBluetoothDevice*      self );
static void                   foobar_bluetooth_device_get_property ( GObject*                    object,
                                                                     guint                       prop_id,
                                                                     GValue*                     value,
                                                                     GParamSpec*                 pspec );
static void                   foobar_bluetooth_device_finalize     ( GObject*                    object );
static FoobarBluetoothDevice* foobar_bluetooth_device_new          ( FoobarBluetoothService*     service,
                                                                     FoobarBluezDevice*          device );
static void                   foobar_bluetooth_device_handle_notify( GObject*                    object,
                                                                     GParamSpec*                 pspec,
                                                                     gpointer                    userdata );
static void                   foobar_bluetooth_device_connect_cb   ( GObject*                    object,
                                                                     GAsyncResult*               result,
                                                                     gpointer                    userdata );
static void                   foobar_bluetooth_device_disconnect_cb( GObject*                    object,
                                                                     GAsyncResult*               result,
                                                                     gpointer                    userdata );
static void                   foobar_bluetooth_device_update_state ( FoobarBluetoothDevice*      self );

G_DEFINE_FINAL_TYPE( FoobarBluetoothDevice, foobar_bluetooth_device, G_TYPE_OBJECT )

//
// FoobarBluetoothService:
//
// Service managing the state of the bluetooth adapter. This is implemented using the Bluez DBus API.
//

struct _FoobarBluetoothService
{
	GObject             parent_instance;
	GDBusObjectManager* object_manager;
	GListStore*         devices;
	GtkFilterListModel* filtered_devices;
	GtkSortListModel*   sorted_devices;
	GtkFilterListModel* connected_devices;
	GPtrArray*          adapters; // First adapter is the default one.
	FoobarBluezAdapter* default_adapter;
	gulong              interface_added_handler_id;
	gulong              interface_removed_handler_id;
	gulong              object_added_handler_id;
	gulong              object_removed_handler_id;
	gulong              adapter_notify_handler_id;
};

enum
{
	PROP_DEVICES = 1,
	PROP_CONNECTED_DEVICES,
	PROP_IS_AVAILABLE,
	PROP_IS_ENABLED,
	PROP_IS_SCANNING,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void           foobar_bluetooth_service_class_init              ( FoobarBluetoothServiceClass* klass );
static void           foobar_bluetooth_service_init                    ( FoobarBluetoothService*      self );
static void           foobar_bluetooth_service_get_property            ( GObject*                     object,
                                                                         guint                        prop_id,
                                                                         GValue*                      value,
                                                                         GParamSpec*                  pspec );
static void           foobar_bluetooth_service_set_property            ( GObject*                     object,
                                                                         guint                        prop_id,
                                                                         GValue const*                value,
                                                                         GParamSpec*                  pspec );
static void           foobar_bluetooth_service_finalize                ( GObject*                     object );
static void           foobar_bluetooth_service_handle_interface_added  ( GDBusObjectManager*          manager,
                                                                         GDBusObject*                 object,
                                                                         GDBusInterface*              interface,
                                                                         gpointer                     userdata );
static void           foobar_bluetooth_service_handle_interface_removed( GDBusObjectManager*          manager,
                                                                         GDBusObject*                 object,
                                                                         GDBusInterface*              interface,
                                                                         gpointer                     userdata );
static void           foobar_bluetooth_service_handle_object_added     ( GDBusObjectManager*          manager,
                                                                         GDBusObject*                 object,
                                                                         gpointer                     userdata );
static void           foobar_bluetooth_service_handle_object_removed   ( GDBusObjectManager*          manager,
                                                                         GDBusObject*                 object,
                                                                         gpointer                     userdata );
static void           foobar_bluetooth_service_handle_adapter_notify   ( GObject*                     object,
                                                                         GParamSpec*                  pspec,
                                                                         gpointer                     userdata );
static void           foobar_bluetooth_service_update_default_adapter  ( FoobarBluetoothService*      self );
static GType          foobar_bluetooth_service_dbus_proxy_type_cb      ( GDBusObjectManagerClient*    manager,
                                                                         gchar const*                 object_path,
                                                                         gchar const*                 interface_name,
                                                                         gpointer                     userdata );
static gboolean       foobar_bluetooth_service_filter_func             ( gpointer                     item,
                                                                         gpointer                     userdata );
static gboolean       foobar_bluetooth_service_connected_filter_func   ( gpointer                     item,
                                                                         gpointer                     userdata );
static gint           foobar_bluetooth_service_sort_func               ( gconstpointer                item_a,
                                                                         gconstpointer                item_b,
                                                                         gpointer                     userdata );

G_DEFINE_FINAL_TYPE( FoobarBluetoothService, foobar_bluetooth_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for devices.
//
void foobar_bluetooth_device_class_init( FoobarBluetoothDeviceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_bluetooth_device_get_property;
	object_klass->finalize = foobar_bluetooth_device_finalize;

	device_props[DEVICE_PROP_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Human-readable name for the device.",
		NULL,
		G_PARAM_READABLE );
	device_props[DEVICE_PROP_STATE] = g_param_spec_enum(
		"state",
		"State",
		"Current connection state of the device.",
		FOOBAR_TYPE_BLUETOOTH_DEVICE_STATE,
		0,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_DEVICE_PROPS, device_props );
}

//
// Instance initialization for devices.
//
void foobar_bluetooth_device_init( FoobarBluetoothDevice* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_bluetooth_device_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarBluetoothDevice* self = (FoobarBluetoothDevice*)object;

	switch ( prop_id )
	{
		case DEVICE_PROP_NAME:
			g_value_set_string( value, foobar_bluetooth_device_get_name( self ) );
			break;
		case DEVICE_PROP_STATE:
			g_value_set_enum( value, foobar_bluetooth_device_get_state( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for devices.
//
void foobar_bluetooth_device_finalize( GObject* object )
{
	FoobarBluetoothDevice* self = (FoobarBluetoothDevice*)object;

	g_clear_signal_handler( &self->notify_handler_id, self->device );
	g_clear_object( &self->device );
	g_clear_object( &self->connect_cancellable );

	G_OBJECT_CLASS( foobar_bluetooth_device_parent_class )->finalize( object );
}

//
// Create a new device object, wrapping a DBus device instance.
//
FoobarBluetoothDevice* foobar_bluetooth_device_new(
	FoobarBluetoothService* service,
	FoobarBluezDevice*      device )
{
	FoobarBluetoothDevice* self = g_object_new( FOOBAR_TYPE_BLUETOOTH_DEVICE, NULL );

	self->service = service;
	self->device = g_object_ref( device );
	foobar_bluetooth_device_update_state( self );

	self->notify_handler_id = g_signal_connect(
		self->device, "notify", G_CALLBACK( foobar_bluetooth_device_handle_notify ), self );

	return self;
}

//
// Get the name/alias for the device.
//
gchar const* foobar_bluetooth_device_get_name( FoobarBluetoothDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_DEVICE( self ), NULL );
	return foobar_bluez_device_get_alias( self->device );
}

//
// Get the current connection state for the device.
//
FoobarBluetoothDeviceState foobar_bluetooth_device_get_state( FoobarBluetoothDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_DEVICE( self ), 0 );
	return self->state;
}

//
// Toggle whether the device is connected:
//
// - If the device is currently disconnected, it is connected.
// - If the device is currently being connected, that process is aborted.
// - If the device is currently connected, it is disconnected.
//
void foobar_bluetooth_device_toggle_connection( FoobarBluetoothDevice* self )
{
	g_return_if_fail( FOOBAR_IS_BLUETOOTH_DEVICE( self ) );

	switch ( self->state )
	{
		case FOOBAR_BLUETOOTH_DEVICE_STATE_DISCONNECTED:
			self->connect_cancellable = g_cancellable_new( );
			foobar_bluetooth_device_update_state( self );
			foobar_bluez_device_call_connect(
				self->device,
				self->connect_cancellable,
				foobar_bluetooth_device_connect_cb,
				g_object_ref( self ) );
			break;
		case FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTING:
			g_cancellable_cancel( self->connect_cancellable );
			break;
		case FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTED:
			foobar_bluez_device_call_disconnect(
				self->device,
				NULL,
				foobar_bluetooth_device_disconnect_cb,
				g_object_ref( self ) );
			break;
		default:
			g_warn_if_reached( );
	}
}

//
// Called by the underlying device when one of its properties changes.
//
void foobar_bluetooth_device_handle_notify(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	FoobarBluetoothDevice* self = (FoobarBluetoothDevice*)userdata;

	gchar const* property = g_param_spec_get_name( pspec );
	if ( !g_strcmp0( property, "alias" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_NAME] );
	}
	else if ( !g_strcmp0( property, "name" ) || !g_strcmp0( property, "adapter" ) )
	{
		if ( self->service )
		{
			gtk_filter_changed(
				gtk_filter_list_model_get_filter( self->service->filtered_devices ),
				GTK_FILTER_CHANGE_DIFFERENT );
		}
	}
	else if ( !g_strcmp0( property, "connected" ) )
	{
		foobar_bluetooth_device_update_state( self );
	}
}

//
// Called when a request to connect the device is completed.
//
// This is mainly used to update the device's state.
//
void foobar_bluetooth_device_connect_cb(
	GObject*      object,
    GAsyncResult* result,
    gpointer      userdata )
{
	(void)object;
	g_autoptr( FoobarBluetoothDevice ) self = (FoobarBluetoothDevice*)userdata;

	g_autoptr( GError ) error = NULL;
	if ( !foobar_bluez_device_call_connect_finish( self->device, result, &error ) )
	{
		g_warning( "Unable to connect bluetooth device: %s", error->message );
	}

	g_clear_object( &self->connect_cancellable );
	foobar_bluetooth_device_update_state( self );
}

//
// Called when a request to disconnect the device is completed.
//
void foobar_bluetooth_device_disconnect_cb(
	GObject*      object,
    GAsyncResult* result,
    gpointer      userdata )
{
	(void)object;
	g_autoptr( FoobarBluetoothDevice ) self = (FoobarBluetoothDevice*)userdata;

	g_autoptr( GError ) error = NULL;
	if ( !foobar_bluez_device_call_disconnect_finish( self->device, result, &error ) )
	{
		g_warning( "Unable to disconnect bluetooth device: %s", error->message );
	}
}

//
// Update the current device state.
//
// The connect_cancellable member variable is used to indicate an ongoing connection attempt. Otherwise, the value of
// is_connected is used.
//
void foobar_bluetooth_device_update_state( FoobarBluetoothDevice* self )
{
	FoobarBluetoothDeviceState new_state = foobar_bluez_device_get_connected( self->device )
		? FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTED
		: FOOBAR_BLUETOOTH_DEVICE_STATE_DISCONNECTED;
	if ( self->connect_cancellable )
	{
		new_state = FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTING;
	}

	if ( self->state != new_state )
	{
		self->state = new_state;
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_STATE] );

		if ( self->service )
		{
			gtk_filter_changed(
				gtk_filter_list_model_get_filter( self->service->connected_devices ),
				GTK_FILTER_CHANGE_DIFFERENT );
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the bluetooth service.
//
void foobar_bluetooth_service_class_init( FoobarBluetoothServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_bluetooth_service_get_property;
	object_klass->set_property = foobar_bluetooth_service_set_property;
	object_klass->finalize = foobar_bluetooth_service_finalize;

	props[PROP_DEVICES] = g_param_spec_object(
		"devices",
		"Devices",
		"Sorted list of available bluetooth devices.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	props[PROP_CONNECTED_DEVICES] = g_param_spec_object(
		"connected-devices",
		"Connected Devices",
		"Sorted list of currently connected bluetooth devices.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	props[PROP_IS_AVAILABLE] = g_param_spec_boolean(
		"is-available",
		"Is Available",
		"Indicates whether the system has a bluetooth adapter.",
		FALSE,
		G_PARAM_READABLE );
	props[PROP_IS_ENABLED] = g_param_spec_boolean(
		"is-enabled",
		"Is Enabled",
		"Indicates whether the bluetooth adapter is currently enabled.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_IS_SCANNING] = g_param_spec_boolean(
		"is-scanning",
		"Is Scanning",
		"Indicates whether the bluetooth adapter is currently scanning for devices.",
		FALSE,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the bluetooth service.
//
void foobar_bluetooth_service_init( FoobarBluetoothService* self )
{
	self->devices = g_list_store_new( FOOBAR_TYPE_BLUETOOTH_DEVICE );

	GtkCustomFilter* filter = gtk_custom_filter_new( foobar_bluetooth_service_filter_func, self, NULL );
	self->filtered_devices = gtk_filter_list_model_new(
		G_LIST_MODEL( g_object_ref( self->devices ) ),
		GTK_FILTER( filter ) );
	
	GtkCustomSorter* sorter = gtk_custom_sorter_new( foobar_bluetooth_service_sort_func, NULL, NULL );
	self->sorted_devices = gtk_sort_list_model_new(
		G_LIST_MODEL( g_object_ref( self->filtered_devices ) ),
		GTK_SORTER( sorter ) );
	
	GtkCustomFilter* connected_filter = gtk_custom_filter_new( foobar_bluetooth_service_connected_filter_func, NULL, NULL );
	self->connected_devices = gtk_filter_list_model_new(
		G_LIST_MODEL( g_object_ref( self->sorted_devices ) ),
		GTK_FILTER( connected_filter ) );
	
	self->adapters = g_ptr_array_new_with_free_func( g_object_unref );

	g_autoptr( GError ) error = NULL;
	self->object_manager = g_dbus_object_manager_client_new_for_bus_sync(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START,
		"org.bluez",
		"/",
		foobar_bluetooth_service_dbus_proxy_type_cb,
		NULL,
		NULL,
		NULL,
		&error );

	if ( self->object_manager )
	{
		self->interface_added_handler_id = g_signal_connect(
			self->object_manager,
			"interface-added",
			G_CALLBACK( foobar_bluetooth_service_handle_interface_added ),
			self );
		self->interface_removed_handler_id = g_signal_connect(
			self->object_manager,
			"interface-removed",
			G_CALLBACK( foobar_bluetooth_service_handle_interface_removed ),
			self );
		self->object_added_handler_id = g_signal_connect(
			self->object_manager,
			"object-added",
			G_CALLBACK( foobar_bluetooth_service_handle_object_added ),
			self );
		self->object_removed_handler_id = g_signal_connect(
			self->object_manager,
			"object-removed",
			G_CALLBACK( foobar_bluetooth_service_handle_object_removed ),
			self );
		
		g_autolist( GDBusObject ) objects = g_dbus_object_manager_get_objects( self->object_manager );
		for ( GList* it = objects; it; it = it->next )
		{
			foobar_bluetooth_service_handle_object_added( self->object_manager, it->data, self );
		}
	}
	else
	{
		g_warning( "Unable to create a DBus object manager for communication with Bluez: %s", error->message );
	}
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_bluetooth_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarBluetoothService* self = (FoobarBluetoothService*)object;

	switch ( prop_id )
	{
		case PROP_DEVICES:
			g_value_set_object( value, foobar_bluetooth_service_get_devices( self ) );
			break;
		case PROP_CONNECTED_DEVICES:
			g_value_set_object( value, foobar_bluetooth_service_get_connected_devices( self ) );
			break;
		case PROP_IS_AVAILABLE:
			g_value_set_boolean( value, foobar_bluetooth_service_is_available( self ) );
			break;
		case PROP_IS_ENABLED:
			g_value_set_boolean( value, foobar_bluetooth_service_is_enabled( self ) );
			break;
		case PROP_IS_SCANNING:
			g_value_set_boolean( value, foobar_bluetooth_service_is_scanning( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_bluetooth_service_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarBluetoothService* self = (FoobarBluetoothService*)object;

	switch ( prop_id )
	{
		case PROP_IS_ENABLED:
			foobar_bluetooth_service_set_enabled( self, g_value_get_boolean( value ) );
			break;
		case PROP_IS_SCANNING:
			foobar_bluetooth_service_set_scanning( self, g_value_get_boolean( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the bluetooth service.
//
void foobar_bluetooth_service_finalize( GObject* object )
{
	FoobarBluetoothService* self = (FoobarBluetoothService*)object;

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->devices ) ); ++i )
	{
		FoobarBluetoothDevice* device = g_list_model_get_item( G_LIST_MODEL( self->devices ), i );
		device->service = NULL;
	}

	g_clear_signal_handler( &self->adapter_notify_handler_id, self->default_adapter );
	g_clear_signal_handler( &self->interface_added_handler_id, self->object_manager );
	g_clear_signal_handler( &self->interface_removed_handler_id, self->object_manager );
	g_clear_signal_handler( &self->object_added_handler_id, self->object_manager );
	g_clear_signal_handler( &self->object_removed_handler_id, self->object_manager );
	g_clear_object( &self->object_manager );
	g_clear_object( &self->connected_devices );
	g_clear_object( &self->sorted_devices );
	g_clear_object( &self->filtered_devices );
	g_clear_object( &self->devices );
	g_clear_object( &self->default_adapter );
	g_clear_pointer( &self->adapters, g_ptr_array_unref );

	G_OBJECT_CLASS( foobar_bluetooth_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new bluetooth service instance.
//
FoobarBluetoothService* foobar_bluetooth_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_BLUETOOTH_SERVICE, NULL );
}

//
// Get the sorted list of available bluetooth devices.
//
GListModel* foobar_bluetooth_service_get_devices( FoobarBluetoothService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->sorted_devices );
}

//
// Get the sorted list of currently connected bluetooth devices.
//
GListModel* foobar_bluetooth_service_get_connected_devices( FoobarBluetoothService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->connected_devices );
}

//
// Check whether the system has a bluetooth adapter.
//
gboolean foobar_bluetooth_service_is_available( FoobarBluetoothService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ), FALSE );
	return self->default_adapter != NULL;
}

//
// Check whether the bluetooth adapter is currently enabled.
//
gboolean foobar_bluetooth_service_is_enabled( FoobarBluetoothService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ), FALSE );
	return self->default_adapter ? foobar_bluez_adapter_get_powered( self->default_adapter ) : FALSE;
}

//
// Check whether the bluetooth adapter is currently scanning for devices.
//
gboolean foobar_bluetooth_service_is_scanning( FoobarBluetoothService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ), FALSE );
	return self->default_adapter ? foobar_bluez_adapter_get_discovering( self->default_adapter ) : FALSE;
}

//
// Update whether the bluetooth adapter is currently enabled.
//
void foobar_bluetooth_service_set_enabled(
	FoobarBluetoothService* self,
    gboolean                value )
{
	g_return_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ) );
	
	if (self->default_adapter)
	{
		foobar_bluez_adapter_set_powered( self->default_adapter, value );
	}
}

//
// Update whether the bluetooth adapter is currently scanning for devices.
//
void foobar_bluetooth_service_set_scanning(
	FoobarBluetoothService* self,
    gboolean                value )
{
	g_return_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( self ) );

	g_autoptr( GError ) error = NULL;
	
	if (self->default_adapter && foobar_bluetooth_service_is_scanning( self ) != value)
	{
		if (value)
		{
			// Make this device discoverable while discovery is active.

			GVariantBuilder filter_builder;
			g_variant_builder_init( &filter_builder, G_VARIANT_TYPE_VARDICT );
			g_variant_builder_add( &filter_builder, "{sv}", "Discoverable", g_variant_new_boolean( TRUE ) );
			GVariant* filter = g_variant_builder_end( &filter_builder );
			if ( !foobar_bluez_adapter_call_set_discovery_filter_sync( self->default_adapter, filter, NULL, &error ) )
			{
				g_warning( "Failed to set discovery filter: %s", error->message );
				return;
			}
			if ( !foobar_bluez_adapter_call_start_discovery_sync( self->default_adapter, NULL, &error ) )
			{
				g_warning( "Failed to start discovery: %s", error->message );
			}
		}
		else
		{
			if ( !foobar_bluez_adapter_call_stop_discovery_sync( self->default_adapter, NULL, &error ) )
			{
				g_warning( "Failed to stop discovery: %s", error->message );
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when an interface was added to an existing DBus object (either adapter or device).
//
void foobar_bluetooth_service_handle_interface_added(
	GDBusObjectManager* manager,
	GDBusObject*        object,
	GDBusInterface*     interface,
	gpointer            userdata )
{
	(void)manager;
	(void)object;
	FoobarBluetoothService* self = (FoobarBluetoothService*)userdata;

	if ( FOOBAR_IS_BLUEZ_DEVICE( interface ) )
	{
		g_autoptr( FoobarBluetoothDevice ) device = foobar_bluetooth_device_new(
			self,
			FOOBAR_BLUEZ_DEVICE( interface ) );
		g_list_store_append( self->devices, device );
	}
	else if ( FOOBAR_IS_BLUEZ_ADAPTER( interface ) )
	{
		g_ptr_array_add( self->adapters, g_object_ref( interface ) );
		foobar_bluetooth_service_update_default_adapter( self );
	}
}

//
// Called when an interface was removed from an existing DBus object (either adapter or device).
//
void foobar_bluetooth_service_handle_interface_removed(
	GDBusObjectManager* manager,
	GDBusObject*        object,
	GDBusInterface*     interface,
	gpointer            userdata )
{
	(void)manager;
	FoobarBluetoothService* self = (FoobarBluetoothService*)userdata;

	gchar const* removed_path = g_dbus_object_get_object_path( object );
	if ( FOOBAR_IS_BLUEZ_DEVICE( interface ) )
	{
		for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->devices ) ); ++i )
		{
			FoobarBluetoothDevice* device = g_list_model_get_item( G_LIST_MODEL( self->devices ), i );
			gchar const* path = g_dbus_proxy_get_object_path( G_DBUS_PROXY( device->device ) );
			if ( !g_strcmp0( path, removed_path ) )
			{
				device->service = NULL;
				g_list_store_remove( self->devices, i );
				break;
			}
		}
	}
	else if ( FOOBAR_IS_BLUEZ_ADAPTER( interface ) )
	{
		for ( guint i = 0; i < self->adapters->len; ++i )
		{
			FoobarBluezAdapter* adapter = g_ptr_array_index( self->adapters, i );
			gchar const* path = g_dbus_proxy_get_object_path( G_DBUS_PROXY( adapter ) );
			if ( !g_strcmp0( path, removed_path ) )
			{
				g_ptr_array_remove_index( self->adapters, i );
				break;
			}
		}
		foobar_bluetooth_service_update_default_adapter( self );
	}
}

//
// Called when a DBus object was added (either adapter or device).
//
void foobar_bluetooth_service_handle_object_added(
	GDBusObjectManager* manager,
	GDBusObject*        object,
	gpointer            userdata )
{
	g_autolist( GDBusInterface ) interfaces = g_dbus_object_get_interfaces( object );
	for ( GList* it = interfaces; it; it = it->next )
	{
		foobar_bluetooth_service_handle_interface_added( manager, object, it->data, userdata );
	}
}

//
// Called when a DBus object was removed (either adapter or device).
//
void foobar_bluetooth_service_handle_object_removed(
	GDBusObjectManager* manager,
	GDBusObject*        object,
	gpointer            userdata )
{
	g_autolist( GDBusInterface ) interfaces = g_dbus_object_get_interfaces( object );
	for ( GList* it = interfaces; it; it = it->next )
	{
		foobar_bluetooth_service_handle_interface_removed( manager, object, it->data, userdata );
	}
}

//
// Called when a property of the default adapter has changed.
//
void foobar_bluetooth_service_handle_adapter_notify(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	FoobarBluetoothService* self = (FoobarBluetoothService*)userdata;

	gchar const* property = g_param_spec_get_name( pspec );
	if ( !g_strcmp0( property, "powered" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_ENABLED] );
	}
	else if ( !g_strcmp0( property, "discovering" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_SCANNING] );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Update the default_adapter to match the first item in the adapters array.
//
void foobar_bluetooth_service_update_default_adapter( FoobarBluetoothService* self )
{
	FoobarBluezAdapter* new_adapter = ( self->adapters->len > 0 ) ? g_ptr_array_index( self->adapters, 0 ) : NULL;

	if ( self->default_adapter != new_adapter )
	{
		g_clear_signal_handler( &self->adapter_notify_handler_id, self->default_adapter );
		g_clear_object( &self->default_adapter );

		if ( new_adapter )
		{
			self->default_adapter = g_object_ref( new_adapter );
			self->adapter_notify_handler_id = g_signal_connect(
				self->default_adapter,
				"notify",
				G_CALLBACK( foobar_bluetooth_service_handle_adapter_notify ),
				self );
		}

		gtk_filter_changed( gtk_filter_list_model_get_filter( self->filtered_devices ), GTK_FILTER_CHANGE_DIFFERENT );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_AVAILABLE] );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_ENABLED] );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_SCANNING] );
	}
}

//
// Get the type for creating a DBus proxy object.
//
// This is used because the DBus object manager should give us instances of the FoobarBluezAdapterProxy and
// FoobarBluezDeviceProxy types.
//
GType foobar_bluetooth_service_dbus_proxy_type_cb(
	GDBusObjectManagerClient* manager,
	gchar const*              object_path,
	gchar const*              interface_name,
	gpointer                  userdata )
{
	if ( !interface_name )
	{
		return G_TYPE_DBUS_OBJECT_PROXY;
	}

	if ( !g_strcmp0( interface_name, "org.bluez.Device1" ) )
	{
		return FOOBAR_TYPE_BLUEZ_DEVICE_PROXY;
	}

	if ( !g_strcmp0( interface_name, "org.bluez.Adapter1" ) )
	{
		return FOOBAR_TYPE_BLUEZ_ADAPTER_PROXY;
	}

	return G_TYPE_DBUS_PROXY;
}

//
// Filtering callback for the list of devices, only showing devices from the default adapter.
//
gboolean foobar_bluetooth_service_filter_func(
	gpointer item,
	gpointer userdata )
{
	FoobarBluetoothService* self = (FoobarBluetoothService*)userdata;
	FoobarBluetoothDevice* device = item;

	gchar const* device_adapter = foobar_bluez_device_get_adapter( device->device );
	gchar const* default_adapter = self->default_adapter
		? g_dbus_proxy_get_object_path( G_DBUS_PROXY( self->default_adapter ) )
		: NULL;
	gchar const* name = foobar_bluez_device_get_name( device->device );
	return name != NULL && !g_strcmp0( device_adapter, default_adapter );
}

//
// Filtering callback for the list of connected devices.
//
gboolean foobar_bluetooth_service_connected_filter_func(
	gpointer item,
	gpointer userdata )
{
	(void)userdata;

	FoobarBluetoothDevice* device = item;
	return foobar_bluetooth_device_get_state( device ) == FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTED;
}

//
// Sorting callback for the list of devices.
//
// Devices are sorted by their names.
//
gint foobar_bluetooth_service_sort_func(
	gconstpointer item_a,
	gconstpointer item_b,
	gpointer      userdata )
{
	(void)userdata;

	FoobarBluetoothDevice* device_a = (FoobarBluetoothDevice*)item_a;
	FoobarBluetoothDevice* device_b = (FoobarBluetoothDevice*)item_b;
	return g_strcmp0( foobar_bluetooth_device_get_name( device_a ), foobar_bluetooth_device_get_name( device_b ) );
}
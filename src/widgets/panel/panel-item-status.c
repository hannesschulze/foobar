#include "widgets/panel/panel-item-status.h"

//
// FoobarPanelItemStatus:
//
// A panel item showing status indicators for various things such as network state, audio volume, screen brightness,
// battery level, etc.
//

struct _FoobarPanelItemStatus
{
	FoobarPanelItem            parent_instance;
	FoobarPanelItemAction      action;
	GtkWidget*                 button;
	GtkWidget*                 layout;
	FoobarBatteryService*      battery_service;
	FoobarBrightnessService*   brightness_service;
	FoobarAudioService*        audio_service;
	FoobarNetworkService*      network_service;
	FoobarBluetoothService*    bluetooth_service;
	FoobarNotificationService* notification_service;
	GtkWidget*                 network_icon;
	GtkWidget*                 network_label;
	GtkExpression*             network_icon_expr;
	FoobarNetwork*             network;
	gulong                     network_handler_id;
	gulong                     network_strength_handler_id;
};

enum
{
	PROP_ORIENTATION = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void       foobar_panel_item_status_class_init                    ( FoobarPanelItemStatusClass* klass );
static void       foobar_panel_item_status_init                          ( FoobarPanelItemStatus*      self );
static void       foobar_panel_item_status_get_property                  ( GObject*                    object,
                                                                           guint                       prop_id,
                                                                           GValue*                     value,
                                                                           GParamSpec*                 pspec );
static void       foobar_panel_item_status_set_property                  ( GObject*                    object,
                                                                           guint                       prop_id,
                                                                           GValue const*               value,
                                                                           GParamSpec*                 pspec );
static void       foobar_panel_item_status_finalize                      ( GObject*                    object );
static void       foobar_panel_item_status_handle_network_change         ( FoobarNetworkAdapterWifi*   adapter,
                                                                           GParamSpec*                 pspec,
                                                                           gpointer                    userdata );
static void       foobar_panel_item_status_handle_network_strength_change( GObject*                    object,
                                                                           GParamSpec*                 pspec,
                                                                           gpointer                    userdata );
static void       foobar_panel_item_status_handle_clicked                ( GtkButton*                  button,
                                                                           gpointer                    userdata );
static gboolean   foobar_panel_item_status_handle_brightness_scroll      ( GtkEventControllerScroll*   controller,
                                                                           gdouble                     dx,
                                                                           gdouble                     dy,
                                                                           gpointer                    userdata );
static gboolean   foobar_panel_item_status_handle_audio_scroll           ( GtkEventControllerScroll*   controller,
                                                                           gdouble                     dx,
                                                                           gdouble                     dy,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_network_icon          ( GtkExpression*              expression,
                                                                           FoobarNetworkAdapter*       adapter,
                                                                           FoobarNetworkAdapterState   state,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_bluetooth_icon        ( GtkExpression*              expression,
                                                                           gboolean                    is_enabled,
                                                                           guint                       connected_device_count,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_battery_icon          ( GtkExpression*              expression,
                                                                           FoobarBatteryState const*   state,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_battery_label         ( GtkExpression*              expression,
                                                                           FoobarBatteryState const*   state,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_brightness_icon       ( GtkExpression*              expression,
                                                                           gint                        percentage,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_brightness_label      ( GtkExpression*              expression,
                                                                           gint                        percentage,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_audio_icon            ( GtkExpression*              expression,
                                                                           gboolean                    is_available,
                                                                           gint                        volume,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_audio_label           ( GtkExpression*              expression,
                                                                           gboolean                    is_available,
                                                                           gint                        volume,
                                                                           gpointer                    userdata );
static gchar*     foobar_panel_item_status_compute_notifications_icon    ( GtkExpression*              expression,
                                                                           guint                       count,
                                                                           gpointer                    userdata );
static GtkWidget* foobar_panel_item_status_create_item                   ( FoobarPanelItemStatus*      self,
                                                                           FoobarStatusItem            item,
                                                                           gboolean                    show_labels,
                                                                           gboolean                    enable_scrolling );
static void       foobar_panel_item_status_reevaluate_network            ( FoobarPanelItemStatus*      self );

G_DEFINE_FINAL_TYPE_WITH_CODE(
	FoobarPanelItemStatus,
	foobar_panel_item_status,
	FOOBAR_TYPE_PANEL_ITEM,
	G_IMPLEMENT_INTERFACE( GTK_TYPE_ORIENTABLE, NULL ) )

// ---------------------------------------------------------------------------------------------------------------------
// Panel Item Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for status panel items.
//
void foobar_panel_item_status_class_init( FoobarPanelItemStatusClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_panel_item_status_get_property;
	object_klass->set_property = foobar_panel_item_status_set_property;
	object_klass->finalize = foobar_panel_item_status_finalize;

	gpointer orientable_iface = g_type_default_interface_peek( GTK_TYPE_ORIENTABLE );
	props[PROP_ORIENTATION] = g_param_spec_override(
		"orientation",
		g_object_interface_find_property( orientable_iface, "orientation" ) );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for status panel items.
//
void foobar_panel_item_status_init( FoobarPanelItemStatus* self )
{
	self->layout = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	self->button = gtk_button_new( );
	gtk_button_set_has_frame( GTK_BUTTON( self->button ), FALSE );
	gtk_button_set_child( GTK_BUTTON( self->button ), self->layout );
	gtk_widget_set_halign( self->button, GTK_ALIGN_CENTER );
	gtk_widget_set_valign( self->button, GTK_ALIGN_CENTER );
	g_signal_connect(
		self->button,
		"clicked",
		G_CALLBACK( foobar_panel_item_status_handle_clicked ),
		self );

	gtk_widget_add_css_class( GTK_WIDGET( self ), "status" );
	foobar_panel_item_set_child( FOOBAR_PANEL_ITEM( self ), self->button );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_panel_item_status_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)object;

	switch ( prop_id )
	{
		case PROP_ORIENTATION:
			g_value_set_enum( value, gtk_orientable_get_orientation( GTK_ORIENTABLE( self->layout ) ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_panel_item_status_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)object;

	switch ( prop_id )
	{
		case PROP_ORIENTATION:
			gtk_orientable_set_orientation( GTK_ORIENTABLE( self->layout ), g_value_get_enum( value ) );
			g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_ORIENTATION] );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for status panel items.
//
void foobar_panel_item_status_finalize( GObject* object )
{
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)object;

	FoobarNetworkAdapterWifi* adapter = foobar_network_service_get_wifi( self->network_service );
	g_clear_signal_handler( &self->network_handler_id, adapter );
	g_clear_signal_handler( &self->network_strength_handler_id, self->network );
	g_clear_object( &self->network );
	g_clear_object( &self->battery_service );
	g_clear_object( &self->brightness_service );
	g_clear_object( &self->audio_service );
	g_clear_object( &self->network_service );
	g_clear_object( &self->bluetooth_service );
	g_clear_object( &self->notification_service );
	g_clear_pointer( &self->network_icon_expr, gtk_expression_unref );

	G_OBJECT_CLASS( foobar_panel_item_status_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new status panel item with the given configuration.
//
FoobarPanelItem* foobar_panel_item_status_new(
	FoobarPanelItemConfiguration const* config,
	FoobarBatteryService*               battery_service,
	FoobarBrightnessService*            brightness_service,
	FoobarAudioService*                 audio_service,
	FoobarNetworkService*               network_service,
	FoobarBluetoothService*             bluetooth_service,
	FoobarNotificationService*          notification_service )
{
	g_return_val_if_fail( config != NULL, NULL );
	g_return_val_if_fail( foobar_panel_item_configuration_get_kind( config ) == FOOBAR_PANEL_ITEM_KIND_STATUS, NULL );
	g_return_val_if_fail( FOOBAR_IS_BATTERY_SERVICE( battery_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_BRIGHTNESS_SERVICE( brightness_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( audio_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_NETWORK_SERVICE( network_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( bluetooth_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_SERVICE( notification_service ), NULL );

	FoobarPanelItemStatus* self = g_object_new( FOOBAR_TYPE_PANEL_ITEM_STATUS, NULL );
	self->action = foobar_panel_item_configuration_get_action( config );
	self->battery_service = g_object_ref( battery_service );
	self->brightness_service = g_object_ref( brightness_service );
	self->audio_service = g_object_ref( audio_service );
	self->network_service = g_object_ref( network_service );
	self->bluetooth_service = g_object_ref( bluetooth_service );
	self->notification_service = g_object_ref( notification_service );

	gtk_widget_set_sensitive( self->button, self->action != FOOBAR_PANEL_ITEM_ACTION_NONE );

	gtk_box_set_spacing( GTK_BOX( self->layout ), foobar_panel_item_status_configuration_get_spacing( config ) );
	gboolean show_labels = foobar_panel_item_status_configuration_get_show_labels( config );
	gboolean enable_scrolling = foobar_panel_item_status_configuration_get_enable_scrolling( config );
	gsize items_count;
	FoobarStatusItem const* items = foobar_panel_item_status_configuration_get_items( config, &items_count );
	for ( gsize i = 0; i < items_count; ++i )
	{
		GtkWidget* widget = foobar_panel_item_status_create_item( self, items[i], show_labels, enable_scrolling );
		if ( widget )
		{
			gtk_widget_set_halign( widget, GTK_ALIGN_CENTER );
			gtk_box_append( GTK_BOX( self->layout ), widget );
		}
	}

	return FOOBAR_PANEL_ITEM( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the wi-fi adapter's active network changes.
//
// This is used to then subscribe to changes to the active network's strength. Unfortunately, we can't just use
// GtkExpressions here because the active network may be NULL in which case the strength expression would fail. If one
// expression fails to evaluate, then the closure expression used to derive the network icon/label also does not get
// updated.
//
// In addition, this method also updates the network label to show the active network's name or become invisible.
//
void foobar_panel_item_status_handle_network_change(
	FoobarNetworkAdapterWifi* adapter,
	GParamSpec*               pspec,
	gpointer                  userdata )
{
	(void)pspec;
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)userdata;

	FoobarNetwork* network = foobar_network_adapter_wifi_get_active( adapter );
	if ( self->network != network )
	{
		g_clear_signal_handler( &self->network_strength_handler_id, self->network );
		g_clear_object( &self->network );

		if ( network )
		{
			self->network = g_object_ref( network );
			self->network_strength_handler_id = g_signal_connect(
				self->network,
				"notify::strength",
				G_CALLBACK( foobar_panel_item_status_handle_network_strength_change ),
				self );
		}

		if ( self->network_label )
		{
			gtk_label_set_label(
				GTK_LABEL( self->network_label ), self->network ? foobar_network_get_name( self->network ) : NULL );
			gtk_widget_set_visible( GTK_WIDGET( self->network_label ), self->network != NULL );
		}

		foobar_panel_item_status_reevaluate_network( self );
	}
}

//
// Called when the wi-fi adapter's active network strength changes.
//
// This just forces re-evaluation of the network icon expression for reasons mentioned in
// foobar_panel_item_status_handle_network_change.
//
void foobar_panel_item_status_handle_network_strength_change(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	(void)pspec;
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)userdata;
	foobar_panel_item_status_reevaluate_network( self );
}

//
// Called when the button was clicked.
//
void foobar_panel_item_status_handle_clicked(
	GtkButton* button,
	gpointer   userdata )
{
	(void)button;
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)userdata;

	foobar_panel_item_invoke_action( FOOBAR_PANEL_ITEM( self ), self->action );
}

//
// Called when the user has scrolled over the brightness item, used to adjust the brightness level.
//
gboolean foobar_panel_item_status_handle_brightness_scroll(
	GtkEventControllerScroll* controller,
	gdouble                   dx,
	gdouble                   dy,
	gpointer                  userdata )
{
	(void)controller;
	(void)dx;
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)userdata;

	gint new_brightness = foobar_brightness_service_get_percentage( self->brightness_service ) + (gint)dy;
	foobar_brightness_service_set_percentage( self->brightness_service, new_brightness );

	return TRUE;
}

//
// Called when the user has scrolled over the audio item, used to adjust the volume.
//
gboolean foobar_panel_item_status_handle_audio_scroll(
	GtkEventControllerScroll* controller,
	gdouble                   dx,
	gdouble                   dy,
	gpointer                  userdata )
{
	(void)controller;
	(void)dx;
	FoobarPanelItemStatus* self = (FoobarPanelItemStatus*)userdata;

	FoobarAudioDevice* device = foobar_audio_service_get_default_output( self->audio_service );
	gint new_volume = foobar_audio_device_get_volume( device ) + (gint)dy;
	foobar_audio_device_set_volume( device, new_volume );

	return TRUE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Value Converters
// ---------------------------------------------------------------------------------------------------------------------

//
// Derive the network icon from the active network adapter and its state.
//
// In addition, this also uses the wi-fi adapter's active network strength which is monitored separately (see
// foobar_panel_item_status_handle_network_change).
//
gchar* foobar_panel_item_status_compute_network_icon(
	GtkExpression*            expression,
	FoobarNetworkAdapter*     adapter,
	FoobarNetworkAdapterState state,
	gpointer                  userdata )
{
	(void)expression;
	(void)userdata;

	if ( FOOBAR_IS_NETWORK_ADAPTER_WIFI( adapter ) )
	{
		if ( state == FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED ) { return g_strdup( "fluent-wifi-off-symbolic" ); }
		if ( state != FOOBAR_NETWORK_ADAPTER_STATE_CONNECTED ) { return g_strdup( "fluent-wifi-warning-symbolic" ); }

		FoobarNetwork* network = foobar_network_adapter_wifi_get_active( FOOBAR_NETWORK_ADAPTER_WIFI( adapter ) );
		gint strength = network ? foobar_network_get_strength( network ) : 0;
		if ( strength <= 25 ) { return g_strdup( "fluent-wifi-4-symbolic" ); }
		if ( strength <= 50 ) { return g_strdup( "fluent-wifi-3-symbolic" ); }
		if ( strength <= 75 ) { return g_strdup( "fluent-wifi-2-symbolic" ); }
		return g_strdup( "fluent-wifi-1-symbolic" );
	}
	else
	{
		return g_strdup( "fluent-virtual-network-symbolic" );
	}
}

//
// Derive the bluetooth icon from the bluetooth adapter's state and the number of currently connected devices.
//
gchar* foobar_panel_item_status_compute_bluetooth_icon(
	GtkExpression* expression,
	gboolean       is_enabled,
	guint          connected_device_count,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	if (is_enabled)
	{
		return ( connected_device_count > 0 )
			? g_strdup( "fluent-bluetooth-connected-symbolic" )
			: g_strdup( "fluent-bluetooth-symbolic" );
	}
	else
	{
		return g_strdup( "fluent-bluetooth-off-symbolic" );
	}
}

//
// Derive the battery icon from the current state structure.
//
gchar* foobar_panel_item_status_compute_battery_icon(
	GtkExpression*            expression,
	FoobarBatteryState const* state,
	gpointer                  userdata )
{
	(void)expression;
	(void)userdata;

	if ( state )
	{
		gint percentage = foobar_battery_state_get_percentage( state );
		if ( foobar_battery_state_is_charging( state ) ) { return g_strdup( "fluent-battery-charge-symbolic" ); }
		if ( percentage <= 5 ) { return g_strdup( "fluent-battery-0-symbolic" ); }
		if ( percentage <= 15 ) { return g_strdup( "fluent-battery-1-symbolic" ); }
		if ( percentage <= 25 ) { return g_strdup( "fluent-battery-2-symbolic" ); }
		if ( percentage <= 35 ) { return g_strdup( "fluent-battery-3-symbolic" ); }
		if ( percentage <= 45 ) { return g_strdup( "fluent-battery-4-symbolic" ); }
		if ( percentage <= 55 ) { return g_strdup( "fluent-battery-5-symbolic" ); }
		if ( percentage <= 65 ) { return g_strdup( "fluent-battery-6-symbolic" ); }
		if ( percentage <= 75 ) { return g_strdup( "fluent-battery-7-symbolic" ); }
		if ( percentage <= 85 ) { return g_strdup( "fluent-battery-8-symbolic" ); }
		if ( percentage <= 95 ) { return g_strdup( "fluent-battery-9-symbolic" ); }
		return g_strdup( "fluent-battery-10-symbolic" );
	}
	else
	{
		return g_strdup( "fluent-plug-connected-symbolic" );
	}
}

//
// Derive the battery label from the current state structure, showing the percentage if available.
//
gchar* foobar_panel_item_status_compute_battery_label(
	GtkExpression*            expression,
	FoobarBatteryState const* state,
	gpointer                  userdata )
{
	(void)expression;
	(void)userdata;

	if ( state )
	{
		return g_strdup_printf( "%d%%", foobar_battery_state_get_percentage( state ) );
	}
	else
	{
		return g_strdup( "n/a" );
	}
}

//
// Derive the brightness icon from the current percentage value.
//
gchar* foobar_panel_item_status_compute_brightness_icon(
	GtkExpression* expression,
	gint           percentage,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	if ( percentage <= 50 )
	{
		return g_strdup( "fluent-brightness-low-symbolic" );
	}
	else
	{
		return g_strdup( "fluent-brightness-high-symbolic" );
	}
}

//
// Derive the brightness label from the current percentage value.
//
gchar* foobar_panel_item_status_compute_brightness_label(
	GtkExpression* expression,
	gint           percentage,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return g_strdup_printf( "%d%%", percentage );
}

//
// Derive the audio icon from the current state and volume of the default output device.
//
gchar* foobar_panel_item_status_compute_audio_icon(
	GtkExpression* expression,
	gboolean       is_available,
	gint           volume,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	if ( is_available )
	{
		if ( volume == 0 ) { return g_strdup( "fluent-speaker-mute-symbolic" ); }
		if ( volume <= 33 ) { return g_strdup( "fluent-speaker-0-symbolic" ); }
		if ( volume <= 67 ) { return g_strdup( "fluent-speaker-1-symbolic" ); }
		return g_strdup( "fluent-speaker-2-symbolic" );
	}
	else
	{
		return g_strdup( "fluent-speaker-off-symbolic" );
	}
}

//
// Derive the audio label from the current state and volume of the default output device.
//
gchar* foobar_panel_item_status_compute_audio_label(
	GtkExpression* expression,
	gboolean       is_available,
	gint           volume,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	if ( is_available )
	{
		return g_strdup_printf( "%d%%", volume );
	}
	else
	{
		return g_strdup( "n/a" );
	}
}

//
// Derive the notification icon from the current number of notifications.
//
gchar* foobar_panel_item_status_compute_notifications_icon(
	GtkExpression* expression,
	guint          count,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	switch ( count )
	{
		case 0:
			return g_strdup( "fluent-checkmark-circle-symbolic" );
		case 1:
			return g_strdup( "fluent-number-circle-1-symbolic" );
		case 2:
			return g_strdup( "fluent-number-circle-2-symbolic" );
		case 3:
			return g_strdup( "fluent-number-circle-3-symbolic" );
		case 4:
			return g_strdup( "fluent-number-circle-4-symbolic" );
		case 5:
			return g_strdup( "fluent-number-circle-5-symbolic" );
		case 6:
			return g_strdup( "fluent-number-circle-6-symbolic" );
		case 7:
			return g_strdup( "fluent-number-circle-7-symbolic" );
		case 8:
			return g_strdup( "fluent-number-circle-8-symbolic" );
		case 9:
			return g_strdup( "fluent-number-circle-9-symbolic" );
		default:
			return g_strdup( "fluent-more-circle-symbolic" );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new icon displaying the specified status item, optionally adding a label to it (if supported).
//
GtkWidget* foobar_panel_item_status_create_item(
	FoobarPanelItemStatus* self,
	FoobarStatusItem       item,
	gboolean               show_labels,
	gboolean               enable_scrolling )
{
	switch ( item )
	{
		case FOOBAR_STATUS_ITEM_NETWORK:
		{
			GtkWidget* box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
			gtk_widget_set_valign( box, GTK_ALIGN_FILL );

			GtkExpression* service_expr = gtk_constant_expression_new(
				FOOBAR_TYPE_NETWORK_SERVICE,
				self->network_service );
			GtkExpression* active_expr = gtk_property_expression_new(
				FOOBAR_TYPE_NETWORK_SERVICE,
				service_expr,
				"active" );
			GtkExpression* state_expr = gtk_property_expression_new(
				FOOBAR_TYPE_NETWORK_ADAPTER,
				gtk_expression_ref( active_expr ),
				"state" );
			GtkExpression* icon_params[] = { active_expr, state_expr };
			self->network_icon_expr = gtk_cclosure_expression_new(
				G_TYPE_STRING,
				NULL,
				G_N_ELEMENTS( icon_params ),
				icon_params,
				G_CALLBACK( foobar_panel_item_status_compute_network_icon ),
				self,
				NULL );
			self->network_icon = gtk_image_new( );
			gtk_widget_set_valign( self->network_icon, GTK_ALIGN_CENTER );
			gtk_expression_bind( gtk_expression_ref( self->network_icon_expr ), self->network_icon, "icon-name", NULL );
			gtk_box_append( GTK_BOX( box ), self->network_icon );

			if ( show_labels )
			{
				self->network_label = gtk_label_new( NULL );
				gtk_widget_set_valign( self->network_label, GTK_ALIGN_CENTER );
				gtk_label_set_max_width_chars( GTK_LABEL( self->network_label ), 16 );
				gtk_label_set_ellipsize( GTK_LABEL( self->network_label ), PANGO_ELLIPSIZE_END );
				gtk_box_append( GTK_BOX( box ), self->network_label );
			}

			// Manually subscribe to network (/strength) because evaluation might fail.

			FoobarNetworkAdapterWifi* adapter = foobar_network_service_get_wifi( self->network_service );
			if ( adapter )
			{
				self->network_handler_id = g_signal_connect(
					adapter,
					"notify::active",
					G_CALLBACK( foobar_panel_item_status_handle_network_change ),
					self );
				if ( foobar_network_adapter_wifi_get_active( adapter ) )
				{
					self->network = g_object_ref( foobar_network_adapter_wifi_get_active( adapter ) );
					self->network_strength_handler_id = g_signal_connect(
						self->network,
						"notify::strength",
						G_CALLBACK( foobar_panel_item_status_handle_network_strength_change ),
						self );
				}
				if ( self->network_label )
				{
					gtk_label_set_label(
						GTK_LABEL( self->network_label ),
						self->network ? foobar_network_get_name( self->network ) : NULL );
					gtk_widget_set_visible( GTK_WIDGET( self->network_label ), self->network != NULL );
				}
			}

			return box;
		}
		case FOOBAR_STATUS_ITEM_BLUETOOTH:
		{
			GtkWidget* icon = gtk_image_new( );
			gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );

			GtkExpression* service_expr = gtk_constant_expression_new(
				FOOBAR_TYPE_BLUETOOTH_SERVICE,
				self->bluetooth_service );
			GtkExpression* enabled_expr = gtk_property_expression_new(
				FOOBAR_TYPE_BLUETOOTH_SERVICE,
				gtk_expression_ref( service_expr ),
				"is-enabled" );
			GtkExpression* connected_devices_expr = gtk_property_expression_new(
				FOOBAR_TYPE_BLUETOOTH_SERVICE,
				service_expr,
				"connected-devices" );
			GtkExpression* connected_device_count_expr = gtk_property_expression_new(
				GTK_TYPE_FILTER_LIST_MODEL,
				connected_devices_expr,
				"n-items" );
			GtkExpression* icon_params[] = { enabled_expr, connected_device_count_expr };
			GtkExpression* icon_expr = gtk_cclosure_expression_new(
				G_TYPE_STRING,
				NULL,
				G_N_ELEMENTS( icon_params ),
				icon_params,
				G_CALLBACK( foobar_panel_item_status_compute_bluetooth_icon ),
				NULL,
				NULL );
			gtk_expression_bind( icon_expr, icon, "icon-name", NULL );

			return icon;
		}
		case FOOBAR_STATUS_ITEM_BATTERY:
		{
			GtkWidget* box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
			gtk_widget_set_valign( box, GTK_ALIGN_FILL );

			GtkExpression* service_expr = gtk_constant_expression_new(
				FOOBAR_TYPE_BATTERY_SERVICE,
				self->battery_service );
			GtkExpression* state_expr = gtk_property_expression_new(
				FOOBAR_TYPE_BATTERY_SERVICE,
				service_expr,
				"state" );

			GtkExpression* icon_params[] = { state_expr };
			GtkExpression* icon_expr = gtk_cclosure_expression_new(
				G_TYPE_STRING,
				NULL,
				G_N_ELEMENTS( icon_params ),
				icon_params,
				G_CALLBACK( foobar_panel_item_status_compute_battery_icon ),
				NULL,
				NULL );
			GtkWidget* icon = gtk_image_new( );
			gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );
			gtk_expression_bind( icon_expr, icon, "icon-name", NULL );
			gtk_box_append( GTK_BOX( box ), icon );

			if ( show_labels )
			{
				GtkExpression* label_params[] = { gtk_expression_ref( state_expr ) };
				GtkExpression* label_expr = gtk_cclosure_expression_new(
					G_TYPE_STRING,
					NULL,
					G_N_ELEMENTS( label_params ),
					label_params,
					G_CALLBACK( foobar_panel_item_status_compute_battery_label ),
					NULL,
					NULL );
				GtkWidget* label = gtk_label_new( NULL );
				gtk_widget_set_valign( label, GTK_ALIGN_CENTER );
				gtk_expression_bind( label_expr, label, "label", NULL );
				gtk_box_append( GTK_BOX( box ), label );
			}

			return box;
		}
		case FOOBAR_STATUS_ITEM_BRIGHTNESS:
		{
			GtkWidget* box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
			gtk_widget_set_valign( box, GTK_ALIGN_FILL );

			if ( enable_scrolling )
			{
				GtkEventController* scroll_controller = gtk_event_controller_scroll_new(
					GTK_EVENT_CONTROLLER_SCROLL_VERTICAL | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE );
				g_signal_connect(
					scroll_controller,
					"scroll",
					G_CALLBACK( foobar_panel_item_status_handle_brightness_scroll ),
					self );
				gtk_widget_add_controller( box, scroll_controller );
			}

			GtkExpression* service_expr = gtk_constant_expression_new(
				FOOBAR_TYPE_BRIGHTNESS_SERVICE,
				self->brightness_service );
			GtkExpression* percentage_expr = gtk_property_expression_new(
				FOOBAR_TYPE_BRIGHTNESS_SERVICE,
				service_expr,
				"percentage" );

			GtkExpression* icon_params[] = { percentage_expr };
			GtkExpression* icon_expr = gtk_cclosure_expression_new(
				G_TYPE_STRING,
				NULL,
				G_N_ELEMENTS( icon_params ),
				icon_params,
				G_CALLBACK( foobar_panel_item_status_compute_brightness_icon ),
				NULL,
				NULL );
			GtkWidget* icon = gtk_image_new( );
			gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );
			gtk_expression_bind( icon_expr, icon, "icon-name", NULL );
			gtk_box_append( GTK_BOX( box ), icon );

			if ( show_labels )
			{
				GtkExpression* label_params[] = { gtk_expression_ref( percentage_expr ) };
				GtkExpression* label_expr = gtk_cclosure_expression_new(
					G_TYPE_STRING,
					NULL,
					G_N_ELEMENTS( label_params ),
					label_params,
					G_CALLBACK( foobar_panel_item_status_compute_brightness_label ),
					NULL,
					NULL );
				GtkWidget* label = gtk_label_new( NULL );
				gtk_widget_set_valign( label, GTK_ALIGN_CENTER );
				gtk_expression_bind( label_expr, label, "label", NULL );
				gtk_box_append( GTK_BOX( box ), label );
			}

			return box;
		}
		case FOOBAR_STATUS_ITEM_AUDIO:
		{
			GtkWidget* box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
			gtk_widget_set_valign( box, GTK_ALIGN_FILL );

			if ( enable_scrolling )
			{
				GtkEventController* scroll_controller = gtk_event_controller_scroll_new(
					GTK_EVENT_CONTROLLER_SCROLL_VERTICAL | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE );
				g_signal_connect(
					scroll_controller,
					"scroll",
					G_CALLBACK( foobar_panel_item_status_handle_audio_scroll ),
					self );
				gtk_widget_add_controller( box, scroll_controller );
			}

			GtkExpression* device_expr = gtk_constant_expression_new(
				FOOBAR_TYPE_AUDIO_DEVICE,
				foobar_audio_service_get_default_output( self->audio_service ) );
			GtkExpression* available_expr = gtk_property_expression_new(
				FOOBAR_TYPE_AUDIO_DEVICE,
				gtk_expression_ref( device_expr ),
				"is-available" );
			GtkExpression* volume_expr = gtk_property_expression_new(
				FOOBAR_TYPE_AUDIO_DEVICE,
				gtk_expression_ref( device_expr ),
				"volume" );

			GtkExpression* icon_params[] = { available_expr, volume_expr };
			GtkExpression* icon_expr = gtk_cclosure_expression_new(
				G_TYPE_STRING,
				NULL,
				G_N_ELEMENTS( icon_params ),
				icon_params,
				G_CALLBACK( foobar_panel_item_status_compute_audio_icon ),
				NULL,
				NULL );
			GtkWidget* icon = gtk_image_new( );
			gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );
			gtk_expression_bind( icon_expr, icon, "icon-name", NULL );
			gtk_box_append( GTK_BOX( box ), icon );

			if ( show_labels )
			{
				GtkExpression* label_params[] = { gtk_expression_ref( available_expr ), gtk_expression_ref( volume_expr ) };
				GtkExpression* label_expr = gtk_cclosure_expression_new(
					G_TYPE_STRING,
					NULL,
					G_N_ELEMENTS( label_params ),
					label_params,
					G_CALLBACK( foobar_panel_item_status_compute_audio_label ),
					NULL,
					NULL );
				GtkWidget* label = gtk_label_new( NULL );
				gtk_widget_set_valign( label, GTK_ALIGN_CENTER );
				gtk_expression_bind( label_expr, label, "label", NULL );
				gtk_box_append( GTK_BOX( box ), label );
			}

			return box;
		}
		case FOOBAR_STATUS_ITEM_NOTIFICATIONS:
		{
			GtkWidget* icon = gtk_image_new( );
			gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );

			GtkExpression* list_expr = gtk_constant_expression_new(
				G_TYPE_LIST_MODEL,
				foobar_notification_service_get_notifications( self->notification_service ) );
			GtkExpression* count_expr = gtk_property_expression_new(
				GTK_TYPE_SORT_LIST_MODEL,
				list_expr,
				"n-items" );
			GtkExpression* icon_params[] = { count_expr };
			GtkExpression* icon_expr = gtk_cclosure_expression_new(
				G_TYPE_STRING,
				NULL,
				G_N_ELEMENTS( icon_params ),
				icon_params,
				G_CALLBACK( foobar_panel_item_status_compute_notifications_icon ),
				NULL,
				NULL );
			gtk_expression_bind( icon_expr, icon, "icon-name", NULL );

			return icon;
		}
		default:
			g_warn_if_reached( );
			return NULL;
	}
}

//
// Force re-evaluation of the network icon expression.
//
// This is used whenever the active network or its strength changes.
//
void foobar_panel_item_status_reevaluate_network( FoobarPanelItemStatus* self )
{
	if ( self->network_icon && self->network_icon_expr )
	{
		GValue value = G_VALUE_INIT;
		if ( gtk_expression_evaluate( self->network_icon_expr, NULL, &value ) )
		{
			gtk_image_set_from_icon_name( GTK_IMAGE( self->network_icon ), g_value_get_string( &value ) );
		}
		g_value_unset( &value );
	}
}
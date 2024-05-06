#include "control-center.h"
#include "widgets/control-center/control-button.h"
#include "widgets/control-center/control-slider.h"
#include "widgets/control-center/control-details.h"
#include "widgets/control-center/control-details-item.h"
#include "widgets/inset-container.h"
#include "widgets/notification-widget.h"
#include <gtk4-layer-shell.h>

#define STACK_ITEM_LIST        "list"
#define STACK_ITEM_PLACEHOLDER "placeholder"

//
// FoobarControlCenter:
//
// The control center is made up of two sections:
// - The "controls" section for configuring wi-fi, volume, brightness, etc.
// - The "notifications" section showing all notifications (including dismissed ones) and allowing the user to close
//   them.
//

struct _FoobarControlCenter
{
	GtkWindow                   parent_instance;
	GtkWidget*                  control_container;
	GtkWidget*                  notification_list;
	GtkWidget*                  notification_container;
	GtkWidget*                  notification_placeholder;
	GtkWidget*                  notification_stack;
	GtkWidget*                  layout;
	FoobarBrightnessService*    brightness_service;
	FoobarAudioService*         audio_service;
	FoobarNetworkService*       network_service;
	FoobarBluetoothService*     bluetooth_service;
	FoobarNotificationService*  notification_service;
	FoobarConfigurationService* configuration_service;
	gchar*                      notification_time_format;
	gint                        notification_min_height;
	gint                        notification_close_button_inset;
	gint                        padding;
	gint                        spacing;
	gulong                      config_handler_id;
};

static void                          foobar_control_center_class_init                             ( FoobarControlCenterClass*  klass );
static void                          foobar_control_center_init                                   ( FoobarControlCenter*       self );
static void                          foobar_control_center_finalize                               ( GObject*                   object );
static void                          foobar_control_center_handle_notification_setup              ( GtkListItemFactory*        factory,
                                                                                                    GtkListItem*               list_item,
                                                                                                    gpointer                   userdata );
static void                          foobar_control_center_handle_network_setup                   ( GtkListItemFactory*        factory,
                                                                                                    GtkListItem*               list_item,
                                                                                                    gpointer                   userdata );
static void                          foobar_control_center_handle_bluetooth_device_setup          ( GtkListItemFactory*        factory,
                                                                                                    GtkListItem*               list_item,
                                                                                                    gpointer                   userdata );
static void                          foobar_control_center_handle_bluetooth_device_activate       ( GtkListView*               view,
                                                                                                    guint                      position,
                                                                                                    gpointer                   userdata );
static void                          foobar_control_center_handle_audio_device_setup              ( GtkListItemFactory*        factory,
                                                                                                    GtkListItem*               list_item,
                                                                                                    gpointer                   userdata );
static void                          foobar_control_center_handle_audio_device_activate           ( GtkListView*               view,
                                                                                                    guint                      position,
                                                                                                    gpointer                   userdata );
static void                          foobar_control_center_handle_config_change                   ( GObject*                   object,
                                                                                                    GParamSpec*                pspec,
                                                                                                    gpointer                   userdata );
static FoobarControlDetailsAccessory foobar_control_center_compute_bluetooth_device_accessory     ( GtkExpression*             expression,
                                                                                                    FoobarBluetoothDeviceState state,
                                                                                                    gpointer                   userdata );
static gchar*                        foobar_control_center_compute_visible_notification_child_name( GtkExpression*             expression,
                                                                                                    guint                      count,
                                                                                                    gpointer                   userdata );

G_DEFINE_FINAL_TYPE( FoobarControlCenter, foobar_control_center, GTK_TYPE_WINDOW )

// ---------------------------------------------------------------------------------------------------------------------
// Window Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the control center.
//
void foobar_control_center_class_init( FoobarControlCenterClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->finalize = foobar_control_center_finalize;
}

//
// Instance initialization for the control center.
//
void foobar_control_center_init( FoobarControlCenter* self )
{
	self->control_container = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_add_css_class( self->control_container, "controls" );
	gtk_widget_set_hexpand( self->control_container, TRUE );

	GtkListItemFactory* notification_factory = gtk_signal_list_item_factory_new( );
	g_signal_connect(
		notification_factory,
		"setup",
		G_CALLBACK( foobar_control_center_handle_notification_setup ),
		self );
	self->notification_list = gtk_list_view_new( NULL, notification_factory );
	gtk_widget_add_css_class( self->notification_list, "notifications" );

	// XXX: Get rid of this container to enable virtualization; currently needed because I haven't figured out how to
	//  set padding without using css (setting margin on list view causes glitches when scrolling).
	self->notification_container = foobar_inset_container_new( );
	foobar_inset_container_set_child( FOOBAR_INSET_CONTAINER( self->notification_container ), self->notification_list );

	GtkWidget* scrolled_window = gtk_scrolled_window_new( );
	gtk_scrolled_window_set_child( GTK_SCROLLED_WINDOW( scrolled_window ), self->notification_container );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL );

	self->notification_placeholder = gtk_label_new( "No Notifications" );
	gtk_widget_add_css_class( self->notification_placeholder, "placeholder" );
	gtk_label_set_ellipsize( GTK_LABEL( self->notification_placeholder ), PANGO_ELLIPSIZE_END );
	gtk_label_set_wrap( GTK_LABEL( self->notification_placeholder ), FALSE );

	self->notification_stack = gtk_stack_new( );
	gtk_stack_add_named( GTK_STACK( self->notification_stack ), scrolled_window, STACK_ITEM_LIST );
	gtk_stack_add_named( GTK_STACK( self->notification_stack ), self->notification_placeholder, STACK_ITEM_PLACEHOLDER );
	gtk_widget_set_vexpand( self->notification_stack, TRUE );
	gtk_widget_set_hexpand( self->notification_stack, TRUE );

	self->layout = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_append( GTK_BOX( self->layout ), self->control_container );
	gtk_box_append( GTK_BOX( self->layout ), gtk_separator_new( GTK_ORIENTATION_HORIZONTAL ) );
	gtk_box_append( GTK_BOX( self->layout ), self->notification_stack );

	g_autoptr( GtkSizeGroup ) size_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );
	gtk_size_group_add_widget( size_group, self->control_container );
	gtk_size_group_add_widget( size_group, self->notification_stack );

	gtk_window_set_child( GTK_WINDOW( self ), self->layout );
	gtk_window_set_title( GTK_WINDOW( self ), "Foobar Control Center" );
	gtk_widget_add_css_class( GTK_WIDGET( self ), "control-center" );
	gtk_layer_init_for_window( GTK_WINDOW( self ) );
	gtk_layer_set_namespace( GTK_WINDOW( self ), "foobar-control-center" );
}

//
// Instance cleanup for the control center.
//
void foobar_control_center_finalize( GObject* object )
{
	FoobarControlCenter* self = (FoobarControlCenter*)object;

	g_clear_signal_handler( &self->config_handler_id, self->configuration_service );
	g_clear_object( &self->brightness_service );
	g_clear_object( &self->audio_service );
	g_clear_object( &self->network_service );
	g_clear_object( &self->bluetooth_service );
	g_clear_object( &self->configuration_service );
	g_clear_object( &self->notification_service );
	g_clear_pointer( &self->notification_time_format, g_free );

	G_OBJECT_CLASS( foobar_control_center_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new control center instance.
//
FoobarControlCenter* foobar_control_center_new(
	FoobarBrightnessService*    brightness_service,
	FoobarAudioService*         audio_service,
	FoobarNetworkService*       network_service,
	FoobarBluetoothService*     bluetooth_service,
	FoobarNotificationService*  notification_service,
	FoobarConfigurationService* configuration_service )
{
	g_return_val_if_fail( FOOBAR_IS_BRIGHTNESS_SERVICE( brightness_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( audio_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_NETWORK_SERVICE( network_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( bluetooth_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_SERVICE( notification_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_CONFIGURATION_SERVICE( configuration_service ), NULL );

	FoobarControlCenter* self = g_object_new( FOOBAR_TYPE_CONTROL_CENTER, NULL );
	self->brightness_service = g_object_ref( brightness_service );
	self->audio_service = g_object_ref( audio_service );
	self->network_service = g_object_ref( network_service );
	self->bluetooth_service = g_object_ref( bluetooth_service );
	self->notification_service = g_object_ref( notification_service );
	self->configuration_service = g_object_ref( configuration_service );

	// Apply the configuration and subscribe to changes.

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_control_center_apply_configuration(
		self,
		foobar_configuration_get_control_center( config ),
		foobar_configuration_get_notifications( config ) );
	self->config_handler_id = g_signal_connect(
		self->configuration_service,
		"notify::current",
		G_CALLBACK( foobar_control_center_handle_config_change ),
		self );

	// Set up the notifications list view.

	GListModel* source_model = foobar_notification_service_get_notifications( self->notification_service );
	GtkNoSelection* selection_model = gtk_no_selection_new( g_object_ref( source_model ) );
	gtk_list_view_set_model( GTK_LIST_VIEW( self->notification_list ), GTK_SELECTION_MODEL( selection_model ) );

	// Set up bindings.

	{
		GtkExpression* count_expr = gtk_property_expression_new( GTK_TYPE_SORT_LIST_MODEL, NULL, "n-items" );
		GtkExpression* child_params[] = { count_expr };
		GtkExpression* child_expr = gtk_cclosure_expression_new(
			G_TYPE_STRING,
			NULL,
			G_N_ELEMENTS( child_params ),
			child_params,
			G_CALLBACK( foobar_control_center_compute_visible_notification_child_name ),
			NULL,
			NULL );
		gtk_expression_bind( child_expr, self->notification_stack, "visible-child-name", source_model );
	}

	return self;
}

//
// Apply the control center configuration provided by the configuration service, additionally using the provided
// notification configuration for some settings.
//
void foobar_control_center_apply_configuration(
	FoobarControlCenter*                    self,
	FoobarControlCenterConfiguration const* config,
	FoobarNotificationConfiguration const*  notification_config )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_CENTER( self ) );
	g_return_if_fail( config != NULL );
	g_return_if_fail( notification_config != NULL );

	// Copy configuration into member variables.

	g_clear_pointer( &self->notification_time_format, g_free );
	self->notification_time_format = g_strdup( foobar_notification_configuration_get_time_format( notification_config ) );
	self->notification_min_height = foobar_notification_configuration_get_min_height( notification_config );
	self->notification_close_button_inset = foobar_notification_configuration_get_close_button_inset( notification_config );
	self->padding = foobar_control_center_configuration_get_padding( config );
	self->spacing = foobar_control_center_configuration_get_spacing( config );

	// Configure the window.

	gtk_window_set_default_size(
		GTK_WINDOW( self ),
		foobar_control_center_configuration_get_width( config ),
		foobar_control_center_configuration_get_height( config ) );

	FoobarScreenEdge position = foobar_control_center_configuration_get_position( config );
	FoobarControlCenterAlignment alignment = foobar_control_center_configuration_get_alignment( config );
	gboolean attach_left = ( position == FOOBAR_SCREEN_EDGE_LEFT );
	gboolean attach_right = ( position == FOOBAR_SCREEN_EDGE_RIGHT );
	gboolean attach_top = ( position == FOOBAR_SCREEN_EDGE_TOP );
	gboolean attach_bottom = ( position == FOOBAR_SCREEN_EDGE_BOTTOM );
	switch ( position )
	{
		case FOOBAR_SCREEN_EDGE_LEFT:
		case FOOBAR_SCREEN_EDGE_RIGHT:
			attach_top =
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_START ) ||
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_FILL );
			attach_bottom =
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_END ) ||
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_FILL );
			break;
		case FOOBAR_SCREEN_EDGE_TOP:
		case FOOBAR_SCREEN_EDGE_BOTTOM:
			attach_left =
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_START ) ||
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_FILL );
			attach_right =
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_END ) ||
				( alignment == FOOBAR_CONTROL_CENTER_ALIGNMENT_FILL );
			break;
		default:
			g_warn_if_reached( );
			break;
	}

	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_LEFT, attach_left );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_RIGHT, attach_right );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_TOP, attach_top );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_BOTTOM, attach_bottom );

	gint offset = foobar_control_center_configuration_get_offset( config );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_LEFT, attach_left ? offset : 0 );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_RIGHT, attach_right ? offset : 0 );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_TOP, attach_top ? offset : 0 );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_BOTTOM, attach_bottom ? offset : 0 );

	// Configure the orientation.

	FoobarOrientation orientation = foobar_control_center_configuration_get_orientation( config );
	switch ( orientation )
	{
		case FOOBAR_ORIENTATION_HORIZONTAL:
			gtk_orientable_set_orientation( GTK_ORIENTABLE( self->layout ), GTK_ORIENTATION_HORIZONTAL );
			break;
		case FOOBAR_ORIENTATION_VERTICAL:
			gtk_orientable_set_orientation( GTK_ORIENTABLE( self->layout ), GTK_ORIENTATION_VERTICAL );
			break;
		default:
			g_warn_if_reached( );
			break;
	}

	// Adjust spacing for the containers.

	foobar_inset_container_set_inset_vertical( FOOBAR_INSET_CONTAINER( self->notification_container ), -self->spacing / 2 );
	gtk_widget_set_margin_top( self->notification_container, self->padding );
	gtk_widget_set_margin_bottom( self->notification_container, self->padding );

	gtk_widget_set_margin_start( self->notification_placeholder, self->padding );
	gtk_widget_set_margin_end( self->notification_placeholder, self->padding );
	gtk_widget_set_margin_top( self->notification_placeholder, self->padding );
	gtk_widget_set_margin_bottom( self->notification_placeholder, self->padding );

	gtk_box_set_spacing( GTK_BOX( self->control_container ), self->spacing );
	gtk_widget_set_margin_top( self->control_container, self->padding );
	gtk_widget_set_margin_bottom( self->control_container, self->padding );

	// Re-create list items by resetting the factory.

	GtkListItemFactory* factory = gtk_list_view_get_factory( GTK_LIST_VIEW( self->notification_list ) );
	g_object_ref( factory );
	gtk_list_view_set_factory( GTK_LIST_VIEW( self->notification_list ), NULL );
	gtk_list_view_set_factory( GTK_LIST_VIEW( self->notification_list ), factory );
	g_object_unref( factory );

	// Re-create control rows.

	GtkWidget* row_widget;
	while ( ( row_widget = gtk_widget_get_first_child( self->control_container ) ) )
	{
		gtk_widget_unparent( row_widget );
	}

	gsize rows_count;
	FoobarControlCenterRow const* rows = foobar_control_center_configuration_get_rows( config, &rows_count );
	for ( gsize i = 0; i < rows_count; ++i )
	{
		// Add rows. If there are details for a row, add them into another box that is then added to the
		// control_container to circumvent the spacing of the box.

		switch ( rows[i] )
		{
			case FOOBAR_CONTROL_CENTER_ROW_CONNECTIVITY:
			{
				// Create the buttons for network and bluetooth.

				GtkWidget* network_button = foobar_control_button_new( );

				GtkWidget* bluetooth_button = foobar_control_button_new( );
				foobar_control_button_set_icon_name( FOOBAR_CONTROL_BUTTON( bluetooth_button ), "fluent-bluetooth-symbolic" );
				foobar_control_button_set_label( FOOBAR_CONTROL_BUTTON( bluetooth_button ), "Bluetooth" );

				GtkWidget* button_container = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, self->spacing );
				gtk_box_set_homogeneous( GTK_BOX( button_container ), TRUE );
				gtk_widget_set_margin_start( button_container, self->padding );
				gtk_widget_set_margin_end( button_container, self->padding );
				gtk_box_append( GTK_BOX( button_container ), network_button );
				gtk_box_append( GTK_BOX( button_container ), bluetooth_button );

				GtkWidget* row = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
				gtk_box_append( GTK_BOX( row ), button_container );

				// Show either a button for managing Wi-Fi or a static element for ethernet.

				FoobarNetworkAdapterWifi* wifi_adapter = foobar_network_service_get_wifi( self->network_service );
				if ( wifi_adapter )
				{
					foobar_control_button_set_icon_name( FOOBAR_CONTROL_BUTTON( network_button ), "fluent-wifi-1-symbolic" );
					foobar_control_button_set_label( FOOBAR_CONTROL_BUTTON( network_button ), "Wi-Fi" );
					foobar_control_button_set_can_expand( FOOBAR_CONTROL_BUTTON( network_button ), TRUE );
					foobar_control_button_set_can_toggle( FOOBAR_CONTROL_BUTTON( network_button ), TRUE );
					g_object_bind_property(
						wifi_adapter,
						"is-enabled",
						network_button,
						"is-toggled",
						G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL );
					g_object_bind_property(
						network_button,
						"is-toggled",
						network_button,
						"can-expand",
						G_BINDING_SYNC_CREATE );
					g_object_bind_property(
						network_button,
						"is-expanded",
						wifi_adapter,
						"is-scanning",
						G_BINDING_SYNC_CREATE );

					GtkListItemFactory* wifi_item_factory = gtk_signal_list_item_factory_new( );
					g_signal_connect(
						wifi_item_factory,
						"setup",
						G_CALLBACK( foobar_control_center_handle_network_setup ),
						self );

					GListModel* wifi_source_model = g_object_ref( foobar_network_adapter_wifi_get_networks( wifi_adapter ) );
					GtkNoSelection* wifi_selection_model = gtk_no_selection_new( wifi_source_model );

					GtkWidget* wifi_list_view = gtk_list_view_new( GTK_SELECTION_MODEL( wifi_selection_model ), wifi_item_factory );

					GtkWidget* wifi_details = foobar_control_details_new( );
					foobar_control_details_set_inset_top( FOOBAR_CONTROL_DETAILS( wifi_details ), self->spacing );
					foobar_control_details_set_child( FOOBAR_CONTROL_DETAILS( wifi_details ), wifi_list_view );
					g_object_bind_property(
						network_button,
						"is-expanded",
						wifi_details,
						"is-expanded",
						G_BINDING_SYNC_CREATE );

					gtk_box_append( GTK_BOX( row ), wifi_details );
				}
				else
				{
					foobar_control_button_set_icon_name( FOOBAR_CONTROL_BUTTON( network_button ), "fluent-virtual-network-symbolic" );
					foobar_control_button_set_label( FOOBAR_CONTROL_BUTTON( network_button ), "Ethernet" );
					foobar_control_button_set_can_expand( FOOBAR_CONTROL_BUTTON( network_button ), FALSE );
					foobar_control_button_set_can_toggle( FOOBAR_CONTROL_BUTTON( network_button ), FALSE );
				}

				// Enable interactions for the bluetooth button if available.

				g_object_bind_property(
					self->bluetooth_service,
					"is-available",
					bluetooth_button,
					"can-toggle",
					G_BINDING_SYNC_CREATE );
				g_object_bind_property(
					self->bluetooth_service,
					"is-enabled",
					bluetooth_button,
					"is-toggled",
					G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL );
				g_object_bind_property(
					bluetooth_button,
					"is-toggled",
					bluetooth_button,
					"can-expand",
					G_BINDING_SYNC_CREATE );
				g_object_bind_property(
					bluetooth_button,
					"is-expanded",
					self->bluetooth_service,
					"is-scanning",
					G_BINDING_SYNC_CREATE );

				GtkListItemFactory* bluetooth_item_factory = gtk_signal_list_item_factory_new( );
				g_signal_connect(
					bluetooth_item_factory,
					"setup",
					G_CALLBACK( foobar_control_center_handle_bluetooth_device_setup ),
					self );

				GListModel* bluetooth_source_model = g_object_ref( foobar_bluetooth_service_get_devices( self->bluetooth_service ) );
				GtkNoSelection* bluetooth_selection_model = gtk_no_selection_new( bluetooth_source_model );

				GtkWidget* bluetooth_list_view = gtk_list_view_new(
					GTK_SELECTION_MODEL( bluetooth_selection_model ),
					bluetooth_item_factory );
				gtk_list_view_set_single_click_activate( GTK_LIST_VIEW( bluetooth_list_view ), TRUE );
				g_signal_connect(
					bluetooth_list_view,
					"activate",
					G_CALLBACK( foobar_control_center_handle_bluetooth_device_activate ),
					NULL );

				GtkWidget* bluetooth_details = foobar_control_details_new( );
				foobar_control_details_set_inset_top( FOOBAR_CONTROL_DETAILS( bluetooth_details ), self->spacing );
				foobar_control_details_set_child( FOOBAR_CONTROL_DETAILS( bluetooth_details ), bluetooth_list_view );
				g_object_bind_property(
					bluetooth_button,
					"is-expanded",
					bluetooth_details,
					"is-expanded",
					G_BINDING_SYNC_CREATE );

				gtk_box_append( GTK_BOX( row ), bluetooth_details );

				gtk_box_append( GTK_BOX( self->control_container ), row );
				break;
			}
			case FOOBAR_CONTROL_CENTER_ROW_AUDIO_OUTPUT:
			case FOOBAR_CONTROL_CENTER_ROW_AUDIO_INPUT:
			{
				FoobarAudioDevice* default_device;
				GListModel* device_list;
				gchar const* icon_name;
				gchar const* label;
				if ( rows[i] == FOOBAR_CONTROL_CENTER_ROW_AUDIO_OUTPUT )
				{
					default_device = foobar_audio_service_get_default_output( self->audio_service );
					device_list = foobar_audio_service_get_outputs( self->audio_service );
					icon_name = "fluent-speaker-2-symbolic";
					label = "Audio Output";
				}
				else
				{
					default_device = foobar_audio_service_get_default_input( self->audio_service );
					device_list = foobar_audio_service_get_inputs( self->audio_service );
					icon_name = "fluent-microphone-symbolic";
					label = "Audio Input";
				}

				// Create the slider for controlling the volume.

				GtkWidget* slider = foobar_control_slider_new( );
				gtk_widget_set_margin_start( slider, self->padding );
				gtk_widget_set_margin_end( slider, self->padding );
				foobar_control_slider_set_icon_name( FOOBAR_CONTROL_SLIDER( slider ), icon_name );
				foobar_control_slider_set_label( FOOBAR_CONTROL_SLIDER( slider ), label );
				foobar_control_slider_set_can_expand( FOOBAR_CONTROL_SLIDER( slider ), TRUE );
				g_object_bind_property(
					default_device,
					"volume",
					slider,
					"percentage",
					G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL );

				// Create the details view for selecting the audio device.

				GtkListItemFactory* item_factory = gtk_signal_list_item_factory_new( );
				g_signal_connect( item_factory, "setup", G_CALLBACK( foobar_control_center_handle_audio_device_setup ), self );

				GListModel* source_model = g_object_ref( device_list );
				GtkNoSelection* selection_model = gtk_no_selection_new( source_model );

				GtkWidget* list_view = gtk_list_view_new( GTK_SELECTION_MODEL( selection_model ), item_factory );
				gtk_list_view_set_single_click_activate( GTK_LIST_VIEW( list_view ), TRUE );
				g_signal_connect(
					list_view,
					"activate",
					G_CALLBACK( foobar_control_center_handle_audio_device_activate ),
					NULL );

				GtkWidget* details = foobar_control_details_new( );
				foobar_control_details_set_inset_top( FOOBAR_CONTROL_DETAILS( details ), self->spacing );
				foobar_control_details_set_child( FOOBAR_CONTROL_DETAILS( details ), list_view );
				g_object_bind_property(
					slider,
					"is-expanded",
					details,
					"is-expanded",
					G_BINDING_SYNC_CREATE );

				GtkWidget* row = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
				gtk_box_append( GTK_BOX( row ), slider );
				gtk_box_append( GTK_BOX( row ), details );

				gtk_box_append( GTK_BOX( self->control_container ), row );
				break;
			}
			case FOOBAR_CONTROL_CENTER_ROW_BRIGHTNESS:
			{
				// Create the slider for adjusting the brightness.

				GtkWidget* slider = foobar_control_slider_new( );
				gtk_widget_set_margin_start( slider, self->padding );
				gtk_widget_set_margin_end( slider, self->padding );
				foobar_control_slider_set_icon_name( FOOBAR_CONTROL_SLIDER( slider ), "fluent-brightness-high-symbolic" );
				foobar_control_slider_set_label( FOOBAR_CONTROL_SLIDER( slider ), "Brightness" );
				g_object_bind_property(
					self->brightness_service,
					"percentage",
					slider,
					"percentage",
					G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL );

				gtk_box_append( GTK_BOX( self->control_container ), slider );
				break;
			}
			default:
			{
				g_warn_if_reached( );
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called by the notification list view to create a widget for displaying a notification.
//
void foobar_control_center_handle_notification_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	FoobarControlCenter* self = (FoobarControlCenter*)userdata;

	GtkWidget* widget = foobar_notification_widget_new( );
	foobar_notification_widget_set_close_action( FOOBAR_NOTIFICATION_WIDGET( widget ), FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_REMOVE );
	foobar_notification_widget_set_time_format( FOOBAR_NOTIFICATION_WIDGET( widget ), self->notification_time_format );
	foobar_notification_widget_set_min_height( FOOBAR_NOTIFICATION_WIDGET( widget ), self->notification_min_height );
	foobar_notification_widget_set_close_button_inset( FOOBAR_NOTIFICATION_WIDGET( widget ), self->notification_close_button_inset );
	foobar_notification_widget_set_inset_start( FOOBAR_NOTIFICATION_WIDGET( widget ), self->padding );
	foobar_notification_widget_set_inset_end( FOOBAR_NOTIFICATION_WIDGET( widget ), self->padding );
	foobar_notification_widget_set_inset_top( FOOBAR_NOTIFICATION_WIDGET( widget ), self->spacing / 2 );
	foobar_notification_widget_set_inset_bottom( FOOBAR_NOTIFICATION_WIDGET( widget ), self->spacing / 2 );
	gtk_list_item_set_child( list_item, widget );

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		gtk_expression_bind( item_expr, widget, "notification", list_item );
	}
}

//
// Called by the wi-fi details list view to create a widget for displaying a network.
//
void foobar_control_center_handle_network_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	FoobarControlCenter* self = (FoobarControlCenter*)userdata;

	GtkWidget* item = foobar_control_details_item_new( );
	gtk_widget_set_margin_start( item, self->padding );
	gtk_widget_set_margin_end( item, self->padding );
	gtk_list_item_set_child( list_item, item );

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* label_expr = gtk_property_expression_new( FOOBAR_TYPE_NETWORK, item_expr, "name" );
		gtk_expression_bind( label_expr, item, "label", list_item );
	}

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* checked_expr = gtk_property_expression_new( FOOBAR_TYPE_NETWORK, item_expr, "is-active" );
		gtk_expression_bind( checked_expr, item, "is-checked", list_item );
	}
}

//
// Called by the bluetooth details list view to create a widget for displaying a device.
//
void foobar_control_center_handle_bluetooth_device_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	FoobarControlCenter* self = (FoobarControlCenter*)userdata;

	GtkWidget* item = foobar_control_details_item_new( );
	gtk_widget_set_margin_start( item, self->padding );
	gtk_widget_set_margin_end( item, self->padding );
	gtk_list_item_set_child( list_item, item );

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* label_expr = gtk_property_expression_new( FOOBAR_TYPE_BLUETOOTH_DEVICE, item_expr, "name" );
		gtk_expression_bind( label_expr, item, "label", list_item );
	}

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* state_expr = gtk_property_expression_new( FOOBAR_TYPE_BLUETOOTH_DEVICE, item_expr, "state" );
		GtkExpression* accessory_params[] = { state_expr };
		GtkExpression* accessory_expr = gtk_cclosure_expression_new(
			FOOBAR_TYPE_CONTROL_DETAILS_ACCESSORY,
			NULL,
			G_N_ELEMENTS( accessory_params ),
			accessory_params,
			G_CALLBACK( foobar_control_center_compute_bluetooth_device_accessory ),
			NULL,
			NULL );
		gtk_expression_bind( accessory_expr, item, "accessory", list_item );
	}
}

//
// Called by the bluetooth details list view when a device was selected.
//
void foobar_control_center_handle_bluetooth_device_activate(
	GtkListView* view,
	guint        position,
	gpointer     userdata )
{
	(void)view;
	(void)userdata;

	GListModel* device_list = G_LIST_MODEL( gtk_list_view_get_model( view ) );
	FoobarBluetoothDevice* device = g_list_model_get_item( device_list, position );
	foobar_bluetooth_device_toggle_connection( device );
}

//
// Called by the audio details list view to create a widget for displaying a device.
//
void foobar_control_center_handle_audio_device_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	FoobarControlCenter* self = (FoobarControlCenter*)userdata;

	GtkWidget* item = foobar_control_details_item_new( );
	gtk_widget_set_margin_start( item, self->padding );
	gtk_widget_set_margin_end( item, self->padding );
	gtk_list_item_set_child( list_item, item );

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* label_expr = gtk_property_expression_new( FOOBAR_TYPE_AUDIO_DEVICE, item_expr, "description" );
		gtk_expression_bind( label_expr, item, "label", list_item );
	}

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* checked_expr = gtk_property_expression_new( FOOBAR_TYPE_AUDIO_DEVICE, item_expr, "is-default" );
		gtk_expression_bind( checked_expr, item, "is-checked", list_item );
	}
}

//
// Called by the audio details list view when a device was selected.
//
void foobar_control_center_handle_audio_device_activate(
	GtkListView* view,
	guint        position,
	gpointer     userdata )
{
	(void)view;
	(void)userdata;

	GListModel* device_list = G_LIST_MODEL( gtk_list_view_get_model( view ) );
	FoobarAudioDevice* device = g_list_model_get_item( device_list, position );
	foobar_audio_device_make_default( device );
}

//
// Signal handler called when the global configuration file has changed.
//
void foobar_control_center_handle_config_change(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	(void)pspec;
	FoobarControlCenter* self = (FoobarControlCenter*)userdata;

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_control_center_apply_configuration(
		self,
		foobar_configuration_get_control_center( config ),
		foobar_configuration_get_notifications( config ) );
}

// ---------------------------------------------------------------------------------------------------------------------
// Value Converters
// ---------------------------------------------------------------------------------------------------------------------

//
// Derive the item accessory from a bluetooth device's connection state.
//
FoobarControlDetailsAccessory foobar_control_center_compute_bluetooth_device_accessory(
	GtkExpression*             expression,
	FoobarBluetoothDeviceState state,
	gpointer                   userdata )
{
	(void)expression;
	(void)userdata;

	switch ( state )
	{
		case FOOBAR_BLUETOOTH_DEVICE_STATE_DISCONNECTED:
			return FOOBAR_CONTROL_DETAILS_ACCESSORY_NONE;
		case FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTING:
			return FOOBAR_CONTROL_DETAILS_ACCESSORY_PROGRESS;
		case FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTED:
			return FOOBAR_CONTROL_DETAILS_ACCESSORY_CHECKED;
		default:
			g_warn_if_reached( );
			return FOOBAR_CONTROL_DETAILS_ACCESSORY_NONE;
	}
}

//
// Derive the visible child in the "notifications" stack from the number of notifications.
//
// If there are no notifications, a placeholder is shown.
//
gchar* foobar_control_center_compute_visible_notification_child_name(
	GtkExpression* expression,
	guint          count,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return count > 0 ? g_strdup( STACK_ITEM_LIST ) : g_strdup( STACK_ITEM_PLACEHOLDER );
}
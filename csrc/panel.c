#include "panel.h"
#include "widgets/panel/panel-item-icon.h"
#include "widgets/panel/panel-item-clock.h"
#include "widgets/panel/panel-item-workspaces.h"
#include "widgets/panel/panel-item-status.h"
#include <gtk4-layer-shell.h>

//
// FoobarPanel:
//
// A panel that is divided into start, center and end sections. Each section can be configured to present a range of
// items.
//

struct _FoobarPanel
{
	GtkWindow                   parent_instance;
	GtkWidget*                  main_layout;
	GtkWidget*                  start_items;
	GtkWidget*                  center_items;
	GtkWidget*                  end_items;
	GPtrArray*                  item_widgets;
	GdkMonitor*                 monitor;
	FoobarBatteryService*       battery_service;
	FoobarClockService*         clock_service;
	FoobarBrightnessService*    brightness_service;
	FoobarWorkspaceService*     workspace_service;
	FoobarAudioService*         audio_service;
	FoobarNetworkService*       network_service;
	FoobarBluetoothService*     bluetooth_service;
	FoobarNotificationService*  notification_service;
	FoobarConfigurationService* configuration_service;
	gulong                      config_handler_id;
};

static void foobar_panel_class_init          ( FoobarPanelClass* klass );
static void foobar_panel_init                ( FoobarPanel*      self );
static void foobar_panel_finalize            ( GObject*          object );
static void foobar_panel_handle_config_change( GObject*          object,
                                               GParamSpec*       pspec,
                                               gpointer          userdata );

G_DEFINE_FINAL_TYPE( FoobarPanel, foobar_panel, GTK_TYPE_APPLICATION_WINDOW )

// ---------------------------------------------------------------------------------------------------------------------
// Window Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the panel.
//
void foobar_panel_class_init( FoobarPanelClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->finalize = foobar_panel_finalize;
}

//
// Instance initialization for the panel.
//
void foobar_panel_init( FoobarPanel* self )
{
	self->start_items = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	self->center_items = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	self->end_items = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	self->main_layout = gtk_center_box_new( );
	gtk_center_box_set_start_widget( GTK_CENTER_BOX( self->main_layout ), self->start_items );
	gtk_center_box_set_center_widget( GTK_CENTER_BOX( self->main_layout ), self->center_items );
	gtk_center_box_set_end_widget( GTK_CENTER_BOX( self->main_layout ), self->end_items );

	gtk_window_set_child( GTK_WINDOW( self ), self->main_layout );
	gtk_window_set_title( GTK_WINDOW( self ), "Foobar Panel" );
	gtk_widget_add_css_class( GTK_WIDGET( self ), "panel" );
	gtk_layer_init_for_window( GTK_WINDOW( self ) );
	gtk_layer_set_namespace( GTK_WINDOW( self ), "foobar-panel" );
	gtk_layer_auto_exclusive_zone_enable( GTK_WINDOW( self ) );
}

//
// Instance cleanup for the panel.
//
void foobar_panel_finalize( GObject* object )
{
	FoobarPanel* self = (FoobarPanel*)object;

	g_clear_signal_handler( &self->config_handler_id, self->configuration_service );
	g_clear_object( &self->monitor );
	g_clear_object( &self->battery_service );
	g_clear_object( &self->clock_service );
	g_clear_object( &self->brightness_service );
	g_clear_object( &self->workspace_service );
	g_clear_object( &self->audio_service );
	g_clear_object( &self->network_service );
	g_clear_object( &self->bluetooth_service );
	g_clear_object( &self->notification_service );
	g_clear_object( &self->configuration_service );
	g_clear_pointer( &self->item_widgets, g_ptr_array_unref );

	G_OBJECT_CLASS( foobar_panel_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new panel instance.
//
// An optional GdkMonitor instance can be provided for multi-monitor setups. Otherwise, the default monitor is used.
//
FoobarPanel* foobar_panel_new(
	GtkApplication*             app,
	GdkMonitor*                 monitor,
	FoobarBatteryService*       battery_service,
	FoobarClockService*         clock_service,
	FoobarBrightnessService*    brightness_service,
	FoobarWorkspaceService*     workspace_service,
	FoobarAudioService*         audio_service,
	FoobarNetworkService*       network_service,
	FoobarBluetoothService*     bluetooth_service,
	FoobarNotificationService*  notification_service,
	FoobarConfigurationService* configuration_service )
{
	g_return_val_if_fail( GTK_IS_APPLICATION( app ), NULL );
	g_return_val_if_fail( monitor == NULL || GDK_IS_MONITOR( monitor ), NULL );
	g_return_val_if_fail( FOOBAR_IS_BATTERY_SERVICE( battery_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_CLOCK_SERVICE( clock_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_BRIGHTNESS_SERVICE( brightness_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE_SERVICE( workspace_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( audio_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_NETWORK_SERVICE( network_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_BLUETOOTH_SERVICE( bluetooth_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_SERVICE( notification_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_CONFIGURATION_SERVICE( configuration_service ), NULL );

	FoobarPanel* self = g_object_new( FOOBAR_TYPE_PANEL, "application", app, NULL );
	self->battery_service = g_object_ref( battery_service );
	self->clock_service = g_object_ref( clock_service );
	self->brightness_service = g_object_ref( brightness_service );
	self->workspace_service = g_object_ref( workspace_service );
	self->audio_service = g_object_ref( audio_service );
	self->network_service = g_object_ref( network_service );
	self->bluetooth_service = g_object_ref( bluetooth_service );
	self->notification_service = g_object_ref( notification_service );
	self->configuration_service = g_object_ref( configuration_service );

	// Set the monitor.

	if ( monitor )
	{
		self->monitor = g_object_ref( monitor );
		gtk_layer_set_monitor( GTK_WINDOW( self ), self->monitor );
	}

	// Apply the configuration and subscribe to changes.

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_panel_apply_configuration( self, foobar_configuration_get_panel( config ) );
	self->config_handler_id = g_signal_connect(
		self->configuration_service,
		"notify::current",
		G_CALLBACK( foobar_panel_handle_config_change ),
		self );

	return self;
}

//
// Apply the panel configuration provided by the configuration service.
//
void foobar_panel_apply_configuration(
	FoobarPanel*                    self,
	FoobarPanelConfiguration const* config )
{
	g_return_if_fail( FOOBAR_IS_PANEL( self ) );
	g_return_if_fail( config != NULL );

	// Derive the orientation from the panel's position.

	FoobarScreenEdge position = foobar_panel_configuration_get_position( config );
	GtkOrientation orientation;
	gchar const* orientation_css_class;
	switch ( position )
	{
		case FOOBAR_SCREEN_EDGE_LEFT:
		case FOOBAR_SCREEN_EDGE_RIGHT:
			orientation = GTK_ORIENTATION_VERTICAL;
			orientation_css_class = "vertical";
			break;
		case FOOBAR_SCREEN_EDGE_TOP:
		case FOOBAR_SCREEN_EDGE_BOTTOM:
			orientation = GTK_ORIENTATION_HORIZONTAL;
			orientation_css_class = "horizontal";
			break;
		default:
			g_warn_if_reached( );
			return;
	}

	// Configure the window.

	gtk_widget_remove_css_class( GTK_WIDGET( self ), "horizontal" );
	gtk_widget_remove_css_class( GTK_WIDGET( self ), "vertical" );
	gtk_widget_add_css_class( GTK_WIDGET( self ), orientation_css_class );

	gint size = foobar_panel_configuration_get_size( config );
	gtk_window_set_default_size(
		GTK_WINDOW( self ),
		orientation == GTK_ORIENTATION_VERTICAL ? size : -1,
		orientation == GTK_ORIENTATION_HORIZONTAL ? size : -1 );

	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_LEFT, position != FOOBAR_SCREEN_EDGE_RIGHT );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_RIGHT, position != FOOBAR_SCREEN_EDGE_LEFT );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_TOP, position != FOOBAR_SCREEN_EDGE_BOTTOM );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_BOTTOM, position != FOOBAR_SCREEN_EDGE_TOP );

	// Configure layout spacing.

	gint margin = foobar_panel_configuration_get_margin( config );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_LEFT, position != FOOBAR_SCREEN_EDGE_RIGHT ? margin : 0 );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_RIGHT, position != FOOBAR_SCREEN_EDGE_LEFT ? margin : 0 );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_TOP, position != FOOBAR_SCREEN_EDGE_BOTTOM ? margin : 0 );
	gtk_layer_set_margin( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_BOTTOM, position != FOOBAR_SCREEN_EDGE_TOP ? margin : 0 );

	gint padding = foobar_panel_configuration_get_padding( config );
	gtk_widget_set_margin_start( GTK_WIDGET( self->main_layout ), orientation == GTK_ORIENTATION_HORIZONTAL ? padding : 0 );
	gtk_widget_set_margin_end( GTK_WIDGET( self->main_layout ), orientation == GTK_ORIENTATION_HORIZONTAL ? padding : 0 );
	gtk_widget_set_margin_top( GTK_WIDGET( self->main_layout ), orientation == GTK_ORIENTATION_VERTICAL ? padding : 0 );
	gtk_widget_set_margin_bottom( GTK_WIDGET( self->main_layout ), orientation == GTK_ORIENTATION_VERTICAL ? padding : 0 );

	gint spacing = foobar_panel_configuration_get_spacing( config );
	gtk_box_set_spacing( GTK_BOX(self->start_items), spacing );
	gtk_box_set_spacing( GTK_BOX(self->center_items), spacing );
	gtk_box_set_spacing( GTK_BOX(self->end_items), spacing );
	gtk_widget_set_margin_top( GTK_WIDGET( self->center_items ), orientation == GTK_ORIENTATION_VERTICAL ? spacing : 0 );
	gtk_widget_set_margin_bottom( GTK_WIDGET( self->center_items ), orientation == GTK_ORIENTATION_VERTICAL ? spacing : 0 );
	gtk_widget_set_margin_start( GTK_WIDGET( self->center_items ), orientation == GTK_ORIENTATION_HORIZONTAL ? spacing : 0 );
	gtk_widget_set_margin_end( GTK_WIDGET( self->center_items ), orientation == GTK_ORIENTATION_HORIZONTAL ? spacing : 0 );

	// Configure the container orientations.

	gtk_orientable_set_orientation( GTK_ORIENTABLE( self->main_layout ), orientation );
	gtk_orientable_set_orientation( GTK_ORIENTABLE( self->start_items ), orientation );
	gtk_orientable_set_orientation( GTK_ORIENTABLE( self->center_items ), orientation );
	gtk_orientable_set_orientation( GTK_ORIENTABLE( self->end_items ), orientation );

	// Re-create panel items.

	if ( self->item_widgets )
	{
		for ( guint i = 0; i < self->item_widgets->len; ++i )
		{
			gtk_widget_unparent( g_ptr_array_index( self->item_widgets, i ) );
		}
		g_clear_pointer( &self->item_widgets, g_ptr_array_unref );
	}

	gsize items_count;
	FoobarPanelItemConfiguration const* const* items = foobar_panel_configuration_get_items( config, &items_count );
	self->item_widgets = g_ptr_array_sized_new( items_count );
	for ( gsize i = 0; i < items_count; ++i )
	{
		// Create an item for the specified type and configuration.

		FoobarPanelItemConfiguration const* item = items[i];
		FoobarPanelItem* item_widget;
		switch ( foobar_panel_item_configuration_get_kind( item ) )
		{
			case FOOBAR_PANEL_ITEM_KIND_ICON:
				item_widget = foobar_panel_item_icon_new( item );
				break;
			case FOOBAR_PANEL_ITEM_KIND_CLOCK:
				item_widget = foobar_panel_item_clock_new(
					item,
					self->clock_service );
				break;
			case FOOBAR_PANEL_ITEM_KIND_WORKSPACES:
				item_widget = foobar_panel_item_workspaces_new(
					item,
					self->monitor,
					self->workspace_service );
				break;
			case FOOBAR_PANEL_ITEM_KIND_STATUS:
				item_widget = foobar_panel_item_status_new(
					item,
					self->battery_service,
					self->brightness_service,
					self->audio_service,
					self->network_service,
					self->bluetooth_service,
					self->notification_service );
				break;
			default:
				g_warn_if_reached( );
				continue;
		}

		// Propagate the panel's orientation if the item supports it.

		if ( GTK_IS_ORIENTABLE( item_widget ) )
		{
			gtk_orientable_set_orientation( GTK_ORIENTABLE( item_widget ), orientation );
		}

		// Add the item to the correct container, depending on its configured position.

		switch ( foobar_panel_item_configuration_get_position( item ) )
		{
			case FOOBAR_PANEL_ITEM_POSITION_START:
				gtk_box_append( GTK_BOX( self->start_items ), GTK_WIDGET( item_widget ) );
				break;
			case FOOBAR_PANEL_ITEM_POSITION_CENTER:
				gtk_box_append( GTK_BOX( self->center_items ), GTK_WIDGET( item_widget ) );
				break;
			case FOOBAR_PANEL_ITEM_POSITION_END:
				gtk_box_append( GTK_BOX( self->end_items ), GTK_WIDGET( item_widget ) );
				break;
			default:
				g_warn_if_reached( );
				break;
		}

		// Add the item to the pointer array for easier cleanup.

		g_ptr_array_add( self->item_widgets, GTK_WIDGET( item_widget ) );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Signal handler called when the global configuration file has changed.
//
void foobar_panel_handle_config_change(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	(void)pspec;
	FoobarPanel* self = (FoobarPanel*)userdata;

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_panel_apply_configuration( self, foobar_configuration_get_panel( config ) );
}

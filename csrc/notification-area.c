#include "notification-area.h"
#include "widgets/notification-widget.h"
#include <gtk4-layer-shell.h>

//
// FoobarNotificationArea:
//
// A (usually invisible) window in the corner of the screen, displaying incoming notifications before they are either
// dismissed or a timeout has passed.
//

struct _FoobarNotificationArea
{
	GtkWindow                   parent_instance;
	GtkWidget*                  notification_list;
	FoobarNotificationService*  notification_service;
	FoobarConfigurationService* configuration_service;
	gint                        min_height;
	gint                        spacing;
	gint                        close_button_inset;
	gchar*                      time_format;
	gulong                      config_handler_id;
};

static void foobar_notification_area_class_init          ( FoobarNotificationAreaClass* klass );
static void foobar_notification_area_init                ( FoobarNotificationArea*      self );
static void foobar_notification_area_finalize            ( GObject*                     object );
static void foobar_notification_area_handle_config_change( GObject*                     object,
                                                           GParamSpec*                  pspec,
                                                           gpointer                     userdata );
static void foobar_notification_area_handle_item_setup   ( GtkListItemFactory*          factory,
                                                           GtkListItem*                 list_item,
                                                           gpointer                     userdata );

G_DEFINE_FINAL_TYPE( FoobarNotificationArea, foobar_notification_area, GTK_TYPE_WINDOW )

// ---------------------------------------------------------------------------------------------------------------------
// Window Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the notification area.
//
void foobar_notification_area_class_init( FoobarNotificationAreaClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->finalize = foobar_notification_area_finalize;
}

//
// Instance initialization for the notification area.
//
void foobar_notification_area_init( FoobarNotificationArea* self )
{
	GtkListItemFactory* notification_factory = gtk_signal_list_item_factory_new( );
	g_signal_connect( notification_factory, "setup", G_CALLBACK( foobar_notification_area_handle_item_setup ), self );
	self->notification_list = gtk_list_view_new( NULL, notification_factory );

	gtk_window_set_child( GTK_WINDOW( self ), self->notification_list );
	gtk_window_set_title( GTK_WINDOW( self ), "Foobar Notification Area" );
	gtk_widget_add_css_class( GTK_WIDGET( self ), "notification-area" );
	gtk_layer_init_for_window( GTK_WINDOW( self ) );
	gtk_layer_set_namespace( GTK_WINDOW( self ), "foobar-notification-area" );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_TOP, TRUE );
}

//
// Instance cleanup for the notification area.
//
void foobar_notification_area_finalize( GObject* object )
{
	FoobarNotificationArea* self = (FoobarNotificationArea*)object;

	g_clear_signal_handler( &self->config_handler_id, self->configuration_service );
	g_clear_object( &self->notification_service );
	g_clear_object( &self->configuration_service );
	g_clear_pointer( &self->time_format, g_free );

	G_OBJECT_CLASS( foobar_notification_area_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new notification area instance.
//
FoobarNotificationArea* foobar_notification_area_new(
	FoobarNotificationService*  notification_service,
	FoobarConfigurationService* configuration_service )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_SERVICE( notification_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_CONFIGURATION_SERVICE( configuration_service ), NULL );

	FoobarNotificationArea* self = g_object_new( FOOBAR_TYPE_NOTIFICATION_AREA, NULL );
	self->notification_service = g_object_ref( notification_service );
	self->configuration_service = g_object_ref( configuration_service );

	// Apply the configuration and subscribe to changes.

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_notification_area_apply_configuration( self, foobar_configuration_get_notifications( config ) );
	self->config_handler_id = g_signal_connect(
		self->configuration_service,
		"notify::current",
		G_CALLBACK( foobar_notification_area_handle_config_change ),
		self );

	// Set up the notifications list view.

	GListModel* source_model = foobar_notification_service_get_popup_notifications( self->notification_service );
	GtkNoSelection* selection_model = gtk_no_selection_new( g_object_ref( source_model ) );
	gtk_list_view_set_model( GTK_LIST_VIEW( self->notification_list ), GTK_SELECTION_MODEL( selection_model ) );

	return self;
}

//
// Apply the notification configuration provided by the configuration service.
//
void foobar_notification_area_apply_configuration(
	FoobarNotificationArea*                self,
	FoobarNotificationConfiguration const* config )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_AREA( self ) );
	g_return_if_fail( config != NULL );

	// Copy configuration into member variables.

	gint width = foobar_notification_configuration_get_width( config );
	self->min_height = foobar_notification_configuration_get_min_height( config );
	self->spacing = foobar_notification_configuration_get_spacing( config );
	self->close_button_inset = foobar_notification_configuration_get_close_button_inset( config );
	g_clear_pointer( &self->time_format, g_free );
	self->time_format = g_strdup( foobar_notification_configuration_get_time_format( config ) );

	// Recreate list items by resetting the factory.

	GtkListItemFactory* factory = gtk_list_view_get_factory( GTK_LIST_VIEW( self->notification_list ) );
	g_object_ref( factory );
	gtk_list_view_set_factory( GTK_LIST_VIEW( self->notification_list ), NULL );
	gtk_list_view_set_factory( GTK_LIST_VIEW( self->notification_list ), factory );
	g_object_unref( factory );

	// We need to use height 2, otherwise it crashes for some reason.

	gtk_window_set_default_size( GTK_WINDOW( self ), width + self->spacing, 2 );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Signal handler called when the global configuration file has changed.
//
void foobar_notification_area_handle_config_change(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	(void)pspec;
	FoobarNotificationArea* self = (FoobarNotificationArea*)userdata;

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_notification_area_apply_configuration( self, foobar_configuration_get_notifications( config ) );
}

//
// Called by the notification list view to create a widget for displaying a notification.
//
void foobar_notification_area_handle_item_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	FoobarNotificationArea* self = (FoobarNotificationArea*)userdata;

	GtkWidget* widget = foobar_notification_widget_new( );
	foobar_notification_widget_set_close_action( FOOBAR_NOTIFICATION_WIDGET( widget ), FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_DISMISS );
	foobar_notification_widget_set_time_format( FOOBAR_NOTIFICATION_WIDGET( widget ), self->time_format );
	foobar_notification_widget_set_min_height( FOOBAR_NOTIFICATION_WIDGET( widget ), self->min_height );
	foobar_notification_widget_set_close_button_inset( FOOBAR_NOTIFICATION_WIDGET( widget ), self->close_button_inset );
	foobar_notification_widget_set_inset_end( FOOBAR_NOTIFICATION_WIDGET( widget ), self->spacing );
	foobar_notification_widget_set_inset_top( FOOBAR_NOTIFICATION_WIDGET( widget ), self->spacing );
	gtk_list_item_set_child( list_item, widget );

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		gtk_expression_bind( item_expr, widget, "notification", list_item );
	}
}

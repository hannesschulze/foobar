#include "application.h"
#include "panel.h"
#include "launcher.h"
#include "control-center.h"
#include "notification-area.h"
#include "dbus/server.h"
#include "services/battery-service.h"
#include "services/clock-service.h"
#include "services/brightness-service.h"
#include "services/quick-answer-service.h"
#include "services/workspace-service.h"
#include "services/notification-service.h"
#include "services/audio-service.h"
#include "services/network-service.h"
#include "services/bluetooth-service.h"
#include "services/application-service.h"
#include "services/configuration-service.h"

//
// FoobarApplication:
//
// Entry point for the application. This class is responsible for parsing and handling command line arguments, creating
// services and presenting windows as needed.
//
// Only the panel windows are application windows -- when all panels are closed, the application quits. Because of this,
// the application can't own a reference to the panel windows but needs to store their window IDs. Since all other
// windows are not attached to the application object, the application can hold a reference on them.
//

struct _FoobarApplication
{
	GtkApplication              parent_instance;
	FoobarBatteryService*       battery_service;
	FoobarClockService*         clock_service;
	FoobarBrightnessService*    brightness_service;
	FoobarWorkspaceService*     workspace_service;
	FoobarNotificationService*  notification_service;
	FoobarAudioService*         audio_service;
	FoobarNetworkService*       network_service;
	FoobarBluetoothService*     bluetooth_service;
	FoobarApplicationService*   application_service;
	FoobarQuickAnswerService*   quick_answer_service;
	FoobarConfigurationService* configuration_service;
	gulong                      config_handler_id;
	FoobarServer*               server_skeleton;
	GtkCssProvider*             style_provider;
	guint                       bus_owner_id;
	GArray*                     panel_window_ids;
	gboolean                    panel_is_multi_monitor;
	GListModel*                 monitors;
	gulong                      monitors_handler_id;
	FoobarLauncher*             launcher;
	FoobarControlCenter*        control_center;
	FoobarNotificationArea*     notification_area;
	gboolean                    option_inspector;
	gboolean                    option_quit;
	gboolean                    option_toggle_launcher;
	gboolean                    option_toggle_control_center;
};

static void     foobar_application_class_init                  ( FoobarApplicationClass*  klass );
static void     foobar_application_init                        ( FoobarApplication*       self );
static void     foobar_application_activate                    ( GApplication*            app );
static int      foobar_application_command_line                ( GApplication*            app,
                                                                 GApplicationCommandLine* cmdline );
static void     foobar_application_finalize                    ( GObject*                 object );
static void     foobar_application_handle_bus_acquired         ( GDBusConnection*         connection,
                                                                 gchar const*             name,
                                                                 gpointer                 userdata );
static gboolean foobar_application_handle_inspector            ( FoobarServer*            server,
                                                                 GDBusMethodInvocation*   invocation,
                                                                 gpointer                 userdata );
static gboolean foobar_application_handle_quit                 ( FoobarServer*            server,
                                                                 GDBusMethodInvocation*   invocation,
                                                                 gpointer                 userdata );
static gboolean foobar_application_handle_toggle_launcher      ( FoobarServer*            server,
                                                                 GDBusMethodInvocation*   invocation,
                                                                 gpointer                 userdata );
static gboolean foobar_application_handle_toggle_control_center( FoobarServer*            server,
                                                                 GDBusMethodInvocation*   invocation,
                                                                 gpointer                 userdata );
static void     foobar_application_handle_config_changed       ( GObject*                 object,
                                                                 GParamSpec*              pspec,
                                                                 gpointer                 userdata );
static void     foobar_application_handle_monitors_changed     ( GListModel*              list,
                                                                 guint                    position,
                                                                 guint                    removed,
                                                                 guint                    added,
                                                                 gpointer                 userdata );
static void     foobar_application_destroy_panels              ( FoobarApplication*       self );
static void     foobar_application_create_panels               ( FoobarApplication*       self );
static guint    foobar_application_create_panel                ( FoobarApplication*       self,
                                                                 GdkMonitor*              monitor );

G_DEFINE_FINAL_TYPE( FoobarApplication, foobar_application, GTK_TYPE_APPLICATION )

// ---------------------------------------------------------------------------------------------------------------------
// Application Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the application.
//
void foobar_application_class_init( FoobarApplicationClass* klass )
{
	GApplicationClass* app_klass = G_APPLICATION_CLASS( klass );
	app_klass->activate = foobar_application_activate;
	app_klass->command_line = foobar_application_command_line;

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->finalize = foobar_application_finalize;
}

//
// Instance initialization for the application.
//
void foobar_application_init( FoobarApplication* self )
{
	g_application_set_default( G_APPLICATION( self ) );
	g_application_set_option_context_summary( G_APPLICATION( self ), "A bar." );

	GOptionEntry const options[] =
		{
			{
				.long_name = "inspector",
				.short_name = 'i',
				.flags = G_OPTION_FLAG_NONE,
				.arg = G_OPTION_ARG_NONE,
				.arg_data = &self->option_inspector,
				.description = "Open the GTK inspector for the active bar instance.",
				.arg_description = NULL,
			},
			{
				.long_name = "quit",
				.short_name = 'q',
				.flags = G_OPTION_FLAG_NONE,
				.arg = G_OPTION_ARG_NONE,
				.arg_data = &self->option_quit,
				.description = "Quit the active bar instance.",
				.arg_description = NULL,
			},
			{
				.long_name = "toggle-launcher",
				.short_name = 'l',
				.flags = G_OPTION_FLAG_NONE,
				.arg = G_OPTION_ARG_NONE,
				.arg_data = &self->option_toggle_launcher,
				.description = "Toggle the launcher visibility for the active bar instance.",
				.arg_description = NULL,
			},
			{
				.long_name = "toggle-control-center",
				.short_name = 'c',
				.flags = G_OPTION_FLAG_NONE,
				.arg = G_OPTION_ARG_NONE,
				.arg_data = &self->option_toggle_control_center,
				.description = "Toggle the control center visibility for the active bar instance.",
				.arg_description = NULL,
			},
			{ 0 },
		};
	g_application_add_main_option_entries( G_APPLICATION( self ), options );
}

//
// Called by GTK when the application is activated (only for a single instance of the application).
//
// This is where the services are created and the windows are presented.
//
void foobar_application_activate( GApplication* app )
{
	FoobarApplication* self = (FoobarApplication*)app;

	// Initialize services.

	self->battery_service = foobar_battery_service_new( );
	self->clock_service = foobar_clock_service_new( );
	self->brightness_service = foobar_brightness_service_new( );
	self->workspace_service = foobar_workspace_service_new( );
	self->notification_service = foobar_notification_service_new( );
	self->audio_service = foobar_audio_service_new( );
	self->network_service = foobar_network_service_new( );
	self->bluetooth_service = foobar_bluetooth_service_new( );
	self->application_service = foobar_application_service_new( );
	self->quick_answer_service = foobar_quick_answer_service_new( );
	self->configuration_service = foobar_configuration_service_new( );

	// Enforce a uniform style by forcing Adwaita and shipping our own icons.

	GtkSettings* settings = gtk_settings_get_default( );
	g_object_set( settings, "gtk-theme-name", "Adwaita", NULL );

	GtkIconTheme* icon_theme = gtk_icon_theme_get_for_display( gdk_display_get_default( ) );
	gtk_icon_theme_add_resource_path( icon_theme, "/foobar/icons" );

	// This application instance now becomes the "server" process, listening for commands from clients (which can be
	// invoked via the command line).

	self->bus_owner_id = g_bus_own_name(
		G_BUS_TYPE_SESSION,
		g_application_get_application_id( G_APPLICATION( self ) ),
		G_BUS_NAME_OWNER_FLAGS_NONE,
		foobar_application_handle_bus_acquired,
		NULL,
		NULL,
		self,
		NULL );

	// React to configuration file changes.

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_application_apply_configuration( self, foobar_configuration_get_general( config ) );
	self->panel_is_multi_monitor = foobar_panel_configuration_get_multi_monitor( foobar_configuration_get_panel( config ) );
	self->config_handler_id = g_signal_connect(
		self->configuration_service,
		"notify::current",
		G_CALLBACK( foobar_application_handle_config_changed ),
		self );

	// React to monitor configuration changes (for multi-monitor support in the panel).

	self->monitors = gdk_display_get_monitors( gdk_display_get_default( ) );
	g_object_ref( self->monitors );

	self->monitors_handler_id = g_signal_connect(
		self->monitors,
		"items-changed",
		G_CALLBACK( foobar_application_handle_monitors_changed ),
		self );

	// Create all the windows.

	foobar_application_create_panels( self );

	self->launcher = foobar_launcher_new(
		self->application_service,
		self->quick_answer_service,
		self->configuration_service );
	g_object_ref( self->launcher );

	self->control_center = foobar_control_center_new(
		self->brightness_service,
		self->audio_service,
		self->network_service,
		self->bluetooth_service,
		self->notification_service,
		self->configuration_service );
	g_object_ref( self->control_center );

	self->notification_area = foobar_notification_area_new(
		self->notification_service,
		self->configuration_service );
	g_object_ref( self->notification_area );

	// Only present the notification area, all other windows start out as hidden windows.

	gtk_window_present( GTK_WINDOW( self->notification_area ) );
}

//
// Called before activating the application, when GTK has parsed the command line arguments.
//
// This method decides whether the application should be activated (if it is the first instance). Otherwise it will
// handle command line flags for controlling the application.
//
int foobar_application_command_line(
	GApplication*            app,
	GApplicationCommandLine* cmdline )
{
	(void)cmdline;
	FoobarApplication* self = (FoobarApplication*)app;

	// Try to open a connection to the existing server.

	g_autoptr( GError ) error = NULL;
	g_autoptr( FoobarServer ) proxy = foobar_server_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		g_application_get_application_id( G_APPLICATION( self ) ),
		"/com/github/hannesschulze/foobar/server",
		NULL,
		&error );

	if ( !proxy )
	{
		g_printerr( "Unable to create proxy: %s\n", error->message );
		return 1;
	}

	// If there is no existing server application, the "name owner" will not be set.

	g_autofree gchar* owner = g_dbus_proxy_get_name_owner( G_DBUS_PROXY( proxy ) );
	if ( owner )
	{
		// There is an active instance -> this is a client. Handle all command line flags.

		if ( self->option_inspector && !foobar_server_call_inspector_sync( proxy, NULL, &error ) )
		{
			g_printerr( "Unable to open inspector: %s\n", error->message );
			return 2;
		}

		if ( self->option_toggle_launcher && !foobar_server_call_toggle_launcher_sync( proxy, NULL, &error ) )
		{
			g_printerr( "Unable to toggle launcher: %s\n", error->message );
			return 2;
		}

		if ( self->option_toggle_control_center && !foobar_server_call_toggle_control_center_sync( proxy, NULL, &error ) )
		{
			g_printerr( "Unable to toggle control center: %s\n", error->message );
			return 2;
		}

		if ( self->option_quit && !foobar_server_call_quit_sync( proxy, NULL, &error ) )
		{
			g_printerr( "Unable to quit server: %s\n", error->message );
			return 2;
		}
	}
	else if ( !self->option_quit )
	{
		// This is the main application -> launch the server.

		g_application_activate( G_APPLICATION( self ) );
	}

	return 0;
}

//
// Instance cleanup for the application.
//
void foobar_application_finalize( GObject* object )
{
	FoobarApplication* self = (FoobarApplication*)object;

	// Destroy the DBus server.

	if ( self->server_skeleton )
	{
		g_dbus_interface_skeleton_unexport( G_DBUS_INTERFACE_SKELETON( self->server_skeleton ) );
	}

	// Destroy non-application windows.

	if ( self->launcher ) { gtk_window_destroy( GTK_WINDOW( self->launcher ) ); }
	if ( self->control_center ) { gtk_window_destroy( GTK_WINDOW( self->control_center ) ); }
	if ( self->notification_area ) { gtk_window_destroy( GTK_WINDOW( self->notification_area ) ); }

	// Destroy all other objects.

	g_clear_handle_id( &self->bus_owner_id, g_bus_unown_name );
	g_clear_signal_handler( &self->config_handler_id, self->configuration_service );
	g_clear_signal_handler( &self->monitors_handler_id, self->monitors );
	g_clear_object( &self->battery_service );
	g_clear_object( &self->clock_service );
	g_clear_object( &self->brightness_service );
	g_clear_object( &self->workspace_service );
	g_clear_object( &self->notification_service );
	g_clear_object( &self->audio_service );
	g_clear_object( &self->network_service );
	g_clear_object( &self->bluetooth_service );
	g_clear_object( &self->application_service );
	g_clear_object( &self->quick_answer_service );
	g_clear_object( &self->configuration_service );
	g_clear_object( &self->style_provider );
	g_clear_object( &self->server_skeleton );
	g_clear_object( &self->launcher );
	g_clear_object( &self->control_center );
	g_clear_object( &self->notification_area );
	g_clear_object( &self->monitors );
	g_clear_pointer( &self->panel_window_ids, g_array_unref );

	G_OBJECT_CLASS( foobar_application_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new application instance.
//
FoobarApplication* foobar_application_new( void )
{
	return g_object_new(
		FOOBAR_TYPE_APPLICATION,
		"application-id",
		"com.github.hannesschulze.foobar",
		"flags",
		G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_NON_UNIQUE,
		NULL );
}

//
// Toggle the launcher window's visibility.
//
void foobar_application_toggle_launcher( FoobarApplication* self )
{
	g_return_if_fail( FOOBAR_IS_APPLICATION( self ) );

	gboolean previously_visible = gtk_widget_is_visible( GTK_WIDGET( self->launcher ) );
	gtk_widget_set_visible( GTK_WIDGET( self->launcher ), !previously_visible );
}

//
// Toggle the control center window's visibility.
//
void foobar_application_toggle_control_center( FoobarApplication* self )
{
	g_return_if_fail( FOOBAR_IS_APPLICATION( self ) );

	gboolean previously_visible = gtk_widget_is_visible( GTK_WIDGET( self->control_center ) );
	gtk_widget_set_visible( GTK_WIDGET( self->control_center ), !previously_visible );
}

//
// Apply the general application configuration provided by the configuration service.
//
void foobar_application_apply_configuration(
	FoobarApplication*                self,
	FoobarGeneralConfiguration const* config )
{
	g_return_if_fail( FOOBAR_IS_APPLICATION( self ) );
	g_return_if_fail( config != NULL );

	GdkDisplay* display = gdk_display_get_default( );

	// Remove the old stylesheet from the application.

	if ( self->style_provider )
	{
		gtk_style_context_remove_provider_for_display( display, GTK_STYLE_PROVIDER( self->style_provider ) );
		g_clear_object( &self->style_provider );
	}

	// Add the new stylesheet.

	g_autoptr( GFile ) file = g_file_new_for_uri( foobar_general_configuration_get_stylesheet( config ) );
	self->style_provider = gtk_css_provider_new( );
	gtk_css_provider_load_from_file( self->style_provider, file );
	gtk_style_context_add_provider_for_display(
		display,
		GTK_STYLE_PROVIDER( self->style_provider ),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called after the application server process has asynchronously acquired the bus.
//
// This is where we can export server functionality using the corresponding DBus skeleton object.
//
void foobar_application_handle_bus_acquired(
	GDBusConnection* connection,
	gchar const*     name,
	gpointer         userdata )
{
	(void)name;
	FoobarApplication* self = (FoobarApplication*)userdata;

	self->server_skeleton = foobar_server_skeleton_new( );
	g_signal_connect(
		self->server_skeleton,
		"handle-inspector",
		G_CALLBACK( foobar_application_handle_inspector ),
		self );
	g_signal_connect(
		self->server_skeleton,
		"handle-quit",
		G_CALLBACK( foobar_application_handle_quit ),
		self );
	g_signal_connect(
		self->server_skeleton,
		"handle-toggle-launcher",
		G_CALLBACK( foobar_application_handle_toggle_launcher ),
		self );
	g_signal_connect(
		self->server_skeleton,
		"handle-toggle-control-center",
		G_CALLBACK( foobar_application_handle_toggle_control_center ),
		self );

	g_autoptr( GError ) error = NULL;
	if ( !g_dbus_interface_skeleton_export(
			 G_DBUS_INTERFACE_SKELETON( self->server_skeleton ),
			 connection,
			 "/com/github/hannesschulze/foobar/server",
			 &error ) )
	{
		g_warning( "Unable to export server interface: %s", error->message );
	}
}

//
// DBus skeleton callback for the "Inspector" method.
//
gboolean foobar_application_handle_inspector(
	FoobarServer*          server,
	GDBusMethodInvocation* invocation,
	gpointer               userdata )
{
	(void)userdata;

	gtk_window_set_interactive_debugging( TRUE );

	foobar_server_complete_inspector( server, invocation );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// DBus skeleton callback for the "Quit" method.
//
gboolean foobar_application_handle_quit(
	FoobarServer*          server,
	GDBusMethodInvocation* invocation,
	gpointer               userdata )
{
	FoobarApplication* self = (FoobarApplication*)userdata;

	foobar_application_destroy_panels( self );

	foobar_server_complete_quit( server, invocation );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// DBus skeleton callback for the "ToggleLauncher" method.
//
gboolean foobar_application_handle_toggle_launcher(
	FoobarServer*          server,
	GDBusMethodInvocation* invocation,
	gpointer               userdata )
{
	FoobarApplication* self = (FoobarApplication*)userdata;

	foobar_application_toggle_launcher( self );

	foobar_server_complete_toggle_launcher( server, invocation );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// DBus skeleton callback for the "ToggleControlCenter" method.
//
gboolean foobar_application_handle_toggle_control_center(
	FoobarServer*          server,
	GDBusMethodInvocation* invocation,
	gpointer               userdata )
{
	FoobarApplication* self = (FoobarApplication*)userdata;

	foobar_application_toggle_control_center( self );

	foobar_server_complete_toggle_control_center( server, invocation );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// Signal handler called when the global configuration file has changed.
//
void foobar_application_handle_config_changed(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	(void)pspec;
	FoobarApplication* self = (FoobarApplication*)userdata;

	// Apply general settings.

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_application_apply_configuration( self, foobar_configuration_get_general( config ) );

	// Update panel instances depending on whether multi-monitor mode is enabled.

	FoobarPanelConfiguration const* panel_config = foobar_configuration_get_panel( config );
	gboolean panel_is_multi_monitor = foobar_panel_configuration_get_multi_monitor( panel_config );
	if ( self->panel_is_multi_monitor != panel_is_multi_monitor )
	{
		self->panel_is_multi_monitor = panel_is_multi_monitor;

		// Panel windows keep the application alive, so we need to "hold" it while we re-create the panel windows.

		g_application_hold( G_APPLICATION( self ) );
		foobar_application_destroy_panels( self );
		foobar_application_create_panels( self );
		g_application_release( G_APPLICATION( self ) );
	}
}

//
// Signal handler called when the list of GdkMonitor objects has changed, possibly requiring us to re-create the panel
// windows.
//
void foobar_application_handle_monitors_changed(
	GListModel* list,
	guint       position,
	guint       removed,
	guint       added,
	gpointer    userdata )
{
	(void)list;
	(void)position;
	(void)removed;
	(void)added;
	FoobarApplication* self = (FoobarApplication*)userdata;

	if ( self->panel_is_multi_monitor )
	{
		// Panel windows keep the application alive, so we need to "hold" it while we re-create the panel windows.

		g_application_hold( G_APPLICATION( self ) );
		foobar_application_destroy_panels( self );
		foobar_application_create_panels( self );
		g_application_release( G_APPLICATION( self ) );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Destroy all currently active panel windows.
//
void foobar_application_destroy_panels( FoobarApplication* self )
{
	for ( guint i = 0; i < self->panel_window_ids->len; ++i )
	{
		guint window_id = g_array_index( self->panel_window_ids, guint, i );
		GtkWindow* window = gtk_application_get_window_by_id( GTK_APPLICATION( self ), window_id );
		if ( window ) { gtk_window_destroy( window ); }
	}

	g_clear_pointer( &self->panel_window_ids, g_array_unref );
}

//
// Create and present panel windows for
// - each monitor, if multi-monitor mode is enabled
// - the current monitor, if multi-monitor mode is disabled.
//
void foobar_application_create_panels( FoobarApplication* self )
{
	self->panel_window_ids = g_array_new( FALSE, FALSE, sizeof( guint ) );

	if ( self->panel_is_multi_monitor )
	{
		for ( guint i = 0; i < g_list_model_get_n_items( self->monitors ); ++i )
		{
			GdkMonitor* monitor = g_list_model_get_item( self->monitors, i );
			guint id = foobar_application_create_panel( self, monitor );
			g_array_append_val( self->panel_window_ids, id );
		}
	}
	else
	{
		guint id = foobar_application_create_panel( self, NULL );
		g_array_append_val( self->panel_window_ids, id );
	}
}

//
// Create and present a panel for a single monitor (or NULL to use the current monitor).
//
guint foobar_application_create_panel(
	FoobarApplication* self,
	GdkMonitor*        monitor )
{
	FoobarPanel* panel = foobar_panel_new(
		GTK_APPLICATION( self ),
		monitor,
		self->battery_service,
		self->clock_service,
		self->brightness_service,
		self->workspace_service,
		self->audio_service,
		self->network_service,
		self->bluetooth_service,
		self->notification_service,
		self->configuration_service );
	gtk_window_present( GTK_WINDOW( panel ) );

	return gtk_application_window_get_id( GTK_APPLICATION_WINDOW( panel ) );
}

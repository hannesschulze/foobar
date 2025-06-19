#include "services/brightness-service.h"
#include <gio/gio.h>
#include <math.h>

//
// FoobarBrightnessService:
//
// Service managing the brightness level. This is implemented by
// - for read access: monitoring the "brightness" file in a "/sys/class/backlight" subdirectory,
// - for write access: invoking the "brightnessctl" command.
//

struct _FoobarBrightnessService
{
	GObject       parent_instance;
	gint          percentage;
	GFileMonitor* file_monitor;
	gulong        file_monitor_handler_id;
	gchar*        device_name;
	gchar*        file_path;
	gint          max_brightness;
};

enum
{
	PROP_PERCENTAGE = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void          foobar_brightness_service_class_init               ( FoobarBrightnessServiceClass* klass );
static void          foobar_brightness_service_init                     ( FoobarBrightnessService*      self );
static void          foobar_brightness_service_get_property             ( GObject*                      object,
                                                                          guint                         prop_id,
                                                                          GValue*                       value,
                                                                          GParamSpec*                   pspec );
static void          foobar_brightness_service_set_property             ( GObject*                      object,
                                                                          guint                         prop_id,
                                                                          GValue const*                 value,
                                                                          GParamSpec*                   pspec );
static void          foobar_brightness_service_finalize                 ( GObject*                      object );
static void          foobar_brightness_service_handle_changed           ( GFileMonitor*                 monitor,
                                                                          GFile*                        file,
                                                                          GFile*                        other_file,
                                                                          GFileMonitorEvent             event_type,
                                                                          gpointer                      userdata );
static gint          foobar_brightness_service_load_percentage          ( FoobarBrightnessService*      self );
static void          foobar_brightness_service_write_percentage         ( FoobarBrightnessService*      self );
static void          foobar_brightness_service_write_percentage_watch_cb( GPid                          pid,
                                                                          gint                          status,
                                                                          gpointer                      userdata );
static void          foobar_brightness_service_get_info                 ( gchar**                       out_device_name,
                                                                          gchar**                       out_file_path,
                                                                          gint*                         out_max_brightness );
static gint          foobar_brightness_service_read_value               ( gchar const*                  path );
static GFileMonitor* foobar_brightness_service_monitor                  ( gchar const*                  path );

G_DEFINE_FINAL_TYPE( FoobarBrightnessService, foobar_brightness_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the brightness service.
//
void foobar_brightness_service_class_init( FoobarBrightnessServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_brightness_service_get_property;
	object_klass->set_property = foobar_brightness_service_set_property;
	object_klass->finalize = foobar_brightness_service_finalize;

	props[PROP_PERCENTAGE] = g_param_spec_int(
		"percentage",
		"Percentage",
		"Current brightness percentage.",
		0,
		100,
		100,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the brightness service.
//
void foobar_brightness_service_init( FoobarBrightnessService* self )
{
	foobar_brightness_service_get_info( &self->device_name, &self->file_path, &self->max_brightness );
	self->percentage = foobar_brightness_service_load_percentage( self );
	if ( self->file_path )
	{
		self->file_monitor = foobar_brightness_service_monitor( self->file_path );
	}
	if ( self->file_monitor )
	{
		self->file_monitor_handler_id = g_signal_connect(
			self->file_monitor,
			"changed",
			G_CALLBACK( foobar_brightness_service_handle_changed ),
			self );
	}
	else
	{
		self->file_monitor_handler_id = 0;
	}
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_brightness_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarBrightnessService* self = (FoobarBrightnessService*)object;

	switch ( prop_id )
	{
		case PROP_PERCENTAGE:
			g_value_set_int( value, foobar_brightness_service_get_percentage( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_brightness_service_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarBrightnessService* self = (FoobarBrightnessService*)object;

	switch ( prop_id )
	{
		case PROP_PERCENTAGE:
			foobar_brightness_service_set_percentage( self, g_value_get_int( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the brightness service.
//
void foobar_brightness_service_finalize( GObject* object )
{
	FoobarBrightnessService* self = (FoobarBrightnessService*)object;

	g_clear_signal_handler( &self->file_monitor_handler_id, self->file_monitor );
	g_clear_object( &self->file_monitor );
	g_clear_pointer( &self->device_name, g_free );
	g_clear_pointer( &self->file_path, g_free );

	G_OBJECT_CLASS( foobar_brightness_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new brightness service instance.
//
FoobarBrightnessService* foobar_brightness_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_BRIGHTNESS_SERVICE, NULL );
}

//
// Get the current brightness percentage value.
//
gint foobar_brightness_service_get_percentage( FoobarBrightnessService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BRIGHTNESS_SERVICE( self ), 0 );

	return self->percentage;
}

//
// Update the current brightness percentage value.
//
void foobar_brightness_service_set_percentage(
	FoobarBrightnessService* self,
	gint                     value )
{
	g_return_if_fail( FOOBAR_IS_BRIGHTNESS_SERVICE( self ) );

	value = CLAMP( value, 0, 100 );
	if ( self->percentage != value )
	{
		self->percentage = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_PERCENTAGE] );
		foobar_brightness_service_write_percentage( self );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called by the file monitor when the contents of the "brightness" file have changed.
//
void foobar_brightness_service_handle_changed(
	GFileMonitor*     monitor,
	GFile*            file,
	GFile*            other_file,
	GFileMonitorEvent event_type,
	gpointer          userdata )
{
	(void)monitor;
	(void)file;
	(void)other_file;
	(void)event_type;
	FoobarBrightnessService* self = (FoobarBrightnessService*)userdata;

	gint new_percentage = foobar_brightness_service_load_percentage( self );
	if ( self->percentage != new_percentage )
	{
		self->percentage = new_percentage;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_PERCENTAGE] );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Load the current percentage value from the file at self->file_path.
//
// This requires file_path and max_brightness to be initialized, otherwise the brightness is assumed to be 100%.
//
gint foobar_brightness_service_load_percentage( FoobarBrightnessService* self )
{
	if ( !self->file_path || self->max_brightness == -1 ) { return 100; }

	gint value = foobar_brightness_service_read_value( self->file_path );
	if ( value == -1 ) { return 100; }

	gdouble rel = value / (gdouble)self->max_brightness;
	gint percentage = (gint)round( rel * 100. );
	return CLAMP( percentage, 0, 100 );
}

//
// Asynchronously update the current percentage value by invoking brightnessctl.
//
// This requires device_name to be initialized.
//
void foobar_brightness_service_write_percentage( FoobarBrightnessService* self )
{
	if ( !self->device_name ) { return; }

	g_autofree gchar* percentage_string = g_strdup_printf( "%d%%", self->percentage );
	gchar* args[] = { "brightnessctl", "-d", self->device_name, "s", percentage_string, NULL };

	g_autoptr( GError ) error = NULL;
	GPid child_pid;
	if ( !g_spawn_async(
		NULL,
		args,
		NULL,
		G_SPAWN_DO_NOT_REAP_CHILD
			| G_SPAWN_SEARCH_PATH
			| G_SPAWN_STDIN_FROM_DEV_NULL
			| G_SPAWN_STDOUT_TO_DEV_NULL
			| G_SPAWN_STDERR_TO_DEV_NULL,
		NULL,
		NULL,
		&child_pid,
		&error ) )
	{
		g_warning( "Unable to spawn brightnessctl: %s", error->message );
		return;
	}

	g_child_watch_add( child_pid, foobar_brightness_service_write_percentage_watch_cb, NULL );
}

//
// Called when the brightnessctl child process has finished.
//
void foobar_brightness_service_write_percentage_watch_cb(
	GPid     pid,
	gint     status,
	gpointer userdata )
{
	(void)userdata;

	g_autoptr( GError ) error = NULL;
	if ( !g_spawn_check_wait_status( status, &error ) )
	{
		g_warning( "Unable to set brightness with brightnessctl: %s", error->message );
	}

	g_spawn_close_pid( pid );
}

//
// Initialize information for the brightness device.
//
// This looks for the first directory in the /sys/class/backlight directories and uses it as the device name. It also
// populates out_file_path to the path of this directory and out_max_brightness to the contents of the corresponding
// max_brightness file.
//
void foobar_brightness_service_get_info(
	gchar** out_device_name,
	gchar** out_file_path,
	gint*   out_max_brightness )
{
	*out_device_name = NULL;
	*out_file_path = NULL;
	*out_max_brightness = -1;

	g_autoptr( GFile ) dir = g_file_new_for_path( "/sys/class/backlight" );
	if ( !dir ) { return; }

	g_autoptr( GFileEnumerator ) enumerator = g_file_enumerate_children(
		dir,
		"standard::",
		G_FILE_QUERY_INFO_NONE,
		NULL,
		NULL );
	if ( !enumerator ) { return; }

	g_autoptr( GFileInfo ) info = g_file_enumerator_next_file( enumerator, NULL, NULL );
	g_file_enumerator_close( enumerator, NULL, NULL );
	if ( !info ) { return; }

	gchar* device_name = g_strdup( g_file_info_get_name( info ) );
	gchar* current_file_path = g_strdup_printf( "/sys/class/backlight/%s/brightness", device_name );
	gchar* max_file_path = g_strdup_printf( "/sys/class/backlight/%s/max_brightness", device_name );
	*out_device_name = device_name;
	*out_file_path = current_file_path;
	*out_max_brightness = foobar_brightness_service_read_value( max_file_path );
	g_free( max_file_path );
}

//
// Read the current brightness integer (not percentage) from the given file path.
//
gint foobar_brightness_service_read_value( gchar const* path )
{
	g_autofree gchar* contents = NULL;
	g_autoptr( GError ) error = NULL;
	if ( !g_file_get_contents( path, &contents, NULL, &error ) )
	{
		g_warning( "Unable to read %s: %s", path, error->message );
		return -1;
	}

	errno = 0;
	char* endptr;
	long  result = strtol( contents, &endptr, 10 );
	if ( errno != 0 || endptr == contents || result > INT_MAX || result < 0 )
	{
		g_warning( "Expected integer in %s, but got: %s", path, contents );
		return -1;
	}

	return (gint)result;
}

//
// Set up a file monitor for the given path.
//
GFileMonitor* foobar_brightness_service_monitor( gchar const* path )
{
	g_autoptr( GFile ) file = g_file_new_for_path( path );
	if ( !file ) { return NULL; }

	g_autoptr( GError ) error = NULL;
	GFileMonitor* monitor = g_file_monitor_file( file, G_FILE_MONITOR_NONE, NULL, &error );
	if ( !monitor ) { g_warning( "Unable to monitor brightness: %s", error->message ); }

	return monitor;
}

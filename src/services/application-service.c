#include "services/application-service.h"
#include "launcher-item.h"
#include "utils.h"
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <json-glib/json-glib.h>
#include <string.h>

//
// FoobarApplicationItem:
//
// Represents a single application/desktop file.
//

struct _FoobarApplicationItem
{
	GObject                   parent_instance;
	FoobarApplicationService* service;
	GAppInfo*                 info;
};

enum
{
	APP_PROP_ID = 1,
	APP_PROP_EXECUTABLE,
	APP_PROP_CATEGORIES,
	APP_PROP_FREQUENCY,
	APP_PROP_TITLE,
	APP_PROP_DESCRIPTION,
	APP_PROP_ICON,
	N_APP_PROPS,
};

static GParamSpec* app_props[N_APP_PROPS] = { 0 };

static void                   foobar_application_item_class_init                  ( FoobarApplicationItemClass* klass );
static void                   foobar_application_item_launcher_item_interface_init( FoobarLauncherItemInterface* iface );
static void                   foobar_application_item_init                        ( FoobarApplicationItem*      self );
static void                   foobar_application_item_get_property                ( GObject*                    object,
                                                                                    guint                       prop_id,
                                                                                    GValue*                     value,
                                                                                    GParamSpec*                 pspec );
static void                   foobar_application_item_finalize                    ( GObject*                    object );
static FoobarApplicationItem* foobar_application_item_new                         ( FoobarApplicationService*   service );
static gchar const*           foobar_application_item_get_title                   ( FoobarLauncherItem*         self );
static gchar const*           foobar_application_item_get_description             ( FoobarLauncherItem*         self );
static GIcon*                 foobar_application_item_get_icon                    ( FoobarLauncherItem*         self );
static void                   foobar_application_item_activate                    ( FoobarLauncherItem*         self );
static void                   foobar_application_item_set_info                    ( FoobarApplicationItem*      self,
                                                                                    GAppInfo*                   value );

G_DEFINE_FINAL_TYPE_WITH_CODE(
	FoobarApplicationItem,
	foobar_application_item,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE( FOOBAR_TYPE_LAUNCHER_ITEM, foobar_application_item_launcher_item_interface_init ) )

//
// FoobarApplicationService:
//
// Service providing a list of all installed applications. This implemented using GLib's AppInfo API.
//

struct _FoobarApplicationService
{
	GObject           parent_instance;
	GListStore*       items;
	GtkSortListModel* sorted_items;
	GAppInfoMonitor*  monitor;
	GHashTable*       frequencies; // only modified on the main thread, but possibly read on a background thread
	GMutex            frequencies_mutex;
	GMutex            write_cache_mutex;
	gchar*            cache_path;
	gulong            changed_handler_id;
};

enum
{
	PROP_ITEMS = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void     foobar_application_service_class_init            ( FoobarApplicationServiceClass* klass );
static void     foobar_application_service_init                  ( FoobarApplicationService*      self );
static void     foobar_application_service_get_property          ( GObject*                       object,
                                                                   guint                          prop_id,
                                                                   GValue*                        value,
                                                                   GParamSpec*                    pspec );
static void     foobar_application_service_finalize              ( GObject*                       object );
static void     foobar_application_service_handle_changed        ( GAppInfoMonitor*               monitor,
                                                                   gpointer                       userdata );
static void     foobar_application_service_update                ( FoobarApplicationService*      self );
static void     foobar_application_service_read_cache            ( FoobarApplicationService*      self );
static void     foobar_application_service_read_cache_foreach_cb ( JsonObject*                    object,
                                                                   gchar const*                   member_name,
                                                                   JsonNode*                      member_node,
                                                                   gpointer                       userdata );
static void     foobar_application_service_write_cache           ( FoobarApplicationService*      self );
static void     foobar_application_service_write_cache_cb        ( GObject*                       object,
                                                                   GAsyncResult*                  result,
                                                                   gpointer                       userdata );
static void     foobar_application_service_write_cache_async     ( FoobarApplicationService*      self,
                                                                   GCancellable*                  cancellable,
                                                                   GAsyncReadyCallback            callback,
                                                                   gpointer                       userdata );
static gboolean foobar_application_service_write_cache_finish    ( FoobarApplicationService*      self,
                                                                   GAsyncResult*                  result,
                                                                   GError**                       error );
static void     foobar_application_service_write_cache_thread    ( GTask*                         task,
                                                                   gpointer                       source_object,
                                                                   gpointer                       task_data,
                                                                   GCancellable*                  cancellable );
static void     foobar_application_service_write_cache_foreach_cb( gpointer                       key,
                                                                   gpointer                       value,
                                                                   gpointer                       userdata );
static gint     foobar_application_service_sort_func             ( gconstpointer                  item_a,
                                                                   gconstpointer                  item_b,
                                                                   gpointer                       userdata );

G_DEFINE_FINAL_TYPE( FoobarApplicationService, foobar_application_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Item Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for application items.
//
void foobar_application_item_class_init( FoobarApplicationItemClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_application_item_get_property;
	object_klass->finalize = foobar_application_item_finalize;

	gpointer launcher_item_iface = g_type_default_interface_peek( FOOBAR_TYPE_LAUNCHER_ITEM );
	app_props[APP_PROP_ID] = g_param_spec_string(
		"id",
		"ID",
		"Application identifier string.",
		NULL,
		G_PARAM_READABLE );
	app_props[APP_PROP_EXECUTABLE] = g_param_spec_string(
		"executable",
		"Executable",
		"The executable name of the application.",
		NULL,
		G_PARAM_READABLE );
	app_props[APP_PROP_CATEGORIES] = g_param_spec_string(
		"categories",
		"Categories",
		"Semicolon-separated list of application categories.",
		NULL,
		G_PARAM_READABLE );
	app_props[APP_PROP_FREQUENCY] = g_param_spec_int64(
		"frequency",
		"Frequency",
		"The number of times the application was opened.",
		0,
		INT64_MAX,
		0,
		G_PARAM_READABLE );
	app_props[APP_PROP_TITLE] = g_param_spec_override(
		"title",
		g_object_interface_find_property( launcher_item_iface, "title" ) );
	app_props[APP_PROP_DESCRIPTION] = g_param_spec_override(
		"description",
		g_object_interface_find_property( launcher_item_iface, "description" ) );
	app_props[APP_PROP_ICON] = g_param_spec_override(
		"icon",
		g_object_interface_find_property( launcher_item_iface, "icon" ) );
	g_object_class_install_properties( object_klass, N_APP_PROPS, app_props );
}

//
// Static initialization of the FoobarLauncherItem interface.
//
void foobar_application_item_launcher_item_interface_init( FoobarLauncherItemInterface* iface )
{
	iface->get_title = foobar_application_item_get_title;
	iface->get_description = foobar_application_item_get_description;
	iface->get_icon = foobar_application_item_get_icon;
	iface->activate = foobar_application_item_activate;
}

//
// Instance initialization for application items.
//
void foobar_application_item_init( FoobarApplicationItem* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_application_item_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarApplicationItem* self = (FoobarApplicationItem*)object;

	switch ( prop_id )
	{
		case APP_PROP_ID:
			g_value_set_string( value, foobar_application_item_get_id( self ) );
			break;
		case APP_PROP_EXECUTABLE:
			g_value_set_string( value, foobar_application_item_get_executable( self ) );
			break;
		case APP_PROP_CATEGORIES:
			g_value_set_string( value, foobar_application_item_get_categories( self ) );
			break;
		case APP_PROP_FREQUENCY:
			g_value_set_int64( value, foobar_application_item_get_frequency( self ) );
			break;
		case APP_PROP_TITLE:
			g_value_set_string( value, foobar_launcher_item_get_title( FOOBAR_LAUNCHER_ITEM ( self ) ) );
			break;
		case APP_PROP_DESCRIPTION:
			g_value_set_string( value, foobar_launcher_item_get_description( FOOBAR_LAUNCHER_ITEM( self ) ) );
			break;
		case APP_PROP_ICON:
			g_value_set_object( value, foobar_launcher_item_get_icon( FOOBAR_LAUNCHER_ITEM( self ) ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for application items.
//
void foobar_application_item_finalize( GObject* object )
{
	FoobarApplicationItem* self = (FoobarApplicationItem*)object;

	g_clear_object( &self->info );

	G_OBJECT_CLASS( foobar_application_item_parent_class )->finalize( object );
}

//
// Create a new application item belonging to the parent service (captured as an unowned reference).
//
FoobarApplicationItem* foobar_application_item_new( FoobarApplicationService* service )
{
	FoobarApplicationItem* self = g_object_new( FOOBAR_TYPE_APPLICATION_ITEM, NULL );
	self->service = service;
	return self;
}

//
// Get the application's unique textual identifier.
//
gchar const* foobar_application_item_get_id( FoobarApplicationItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_ITEM( self ), NULL );
	return self->info ? g_app_info_get_id( self->info ) : NULL;
}

//
// Get the application's executable name.
//
gchar const* foobar_application_item_get_executable( FoobarApplicationItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_ITEM( self ), NULL );
	return self->info ? g_app_info_get_executable( self->info ) : NULL;
}

//
// Get the semicolon-separated list of categories for this application.
//
gchar const* foobar_application_item_get_categories( FoobarApplicationItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_ITEM( self ), NULL );
	return G_IS_DESKTOP_APP_INFO( self->info )
		? g_desktop_app_info_get_categories( G_DESKTOP_APP_INFO( self->info ) )
		: NULL;
}

//
// Get the number of times this application was opened using the launcher.
//
gint64 foobar_application_item_get_frequency( FoobarApplicationItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_ITEM( self ), 0 );

	if ( !self->service ) { return 0; }
	gpointer value = g_hash_table_lookup( self->service->frequencies, foobar_application_item_get_id( self ) );
	return GPOINTER_TO_SIZE( value );
}

//
// Get the application's human-readable name.
//
gchar const* foobar_application_item_get_title( FoobarLauncherItem* item )
{
	FoobarApplicationItem* self = ( FoobarApplicationItem* )item;
	return self->info ? g_app_info_get_display_name( self->info ) : NULL;
}

//
// Get an optional short description for the application.
//
gchar const* foobar_application_item_get_description( FoobarLauncherItem* item )
{
	FoobarApplicationItem* self = ( FoobarApplicationItem* )item;
	return self->info ? g_app_info_get_description( self->info ) : NULL;
}

//
// Get an icon representing the application.
//
GIcon* foobar_application_item_get_icon( FoobarLauncherItem* item )
{
	FoobarApplicationItem* self = ( FoobarApplicationItem* )item;
	return self->info ? g_app_info_get_icon( self->info ) : NULL;
}

//
// Launch the application, increasing its frequency by one.
//
void foobar_application_item_activate( FoobarLauncherItem* item )
{
	FoobarApplicationItem* self = ( FoobarApplicationItem* )item;

	if ( !self->info ) { return; }

	g_autoptr( GError ) error = NULL;
	if ( !g_app_info_launch( self->info, NULL, NULL, &error ) )
	{
		g_warning( "Unable to launch application: %s", error->message );
	}

	gchar const* id = foobar_application_item_get_id( self );
	if ( !self->service || !id ) { return; }

	gpointer value = g_hash_table_lookup( self->service->frequencies, id );
	value = GSIZE_TO_POINTER( GPOINTER_TO_SIZE( value ) + 1 );
	g_mutex_lock( &self->service->frequencies_mutex );
	g_hash_table_insert( self->service->frequencies, g_strdup( id ), value );
	g_mutex_unlock( &self->service->frequencies_mutex );

	g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_FREQUENCY] );
	gtk_sorter_changed( gtk_sort_list_model_get_sorter( self->service->sorted_items ), GTK_SORTER_CHANGE_DIFFERENT );
	foobar_application_service_write_cache( self->service );
}

//
// Update the backing GAppInfo object for the application.
//
void foobar_application_item_set_info(
	FoobarApplicationItem* self,
	GAppInfo*              value )
{
	g_return_if_fail( FOOBAR_IS_APPLICATION_ITEM( self ) );

	if ( self->info != value )
	{
		g_clear_object( &self->info );

		if ( value ) { self->info = g_object_ref( value ); }

		g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_ID] );
		g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_TITLE] );
		g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_DESCRIPTION] );
		g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_EXECUTABLE] );
		g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_ICON] );
		g_object_notify_by_pspec( G_OBJECT( self ), app_props[APP_PROP_FREQUENCY] );
	}
}

//
// Match the item against the given search terms.
//
gboolean foobar_application_item_match(
	FoobarApplicationItem* self,
	gchar const* const*    terms )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_ITEM( self ), FALSE );
	g_return_val_if_fail( terms != NULL, FALSE );

	for ( gchar const* const* it = terms; *it; ++it )
	{
		gchar const* term = *it;
		gchar const* name = foobar_launcher_item_get_title( FOOBAR_LAUNCHER_ITEM( self ) );
		gchar const* description = foobar_launcher_item_get_description( FOOBAR_LAUNCHER_ITEM( self ) );
		gchar const* executable = foobar_application_item_get_executable( self );
		gchar const* categories = foobar_application_item_get_categories( self );
		gchar const* id = foobar_application_item_get_id( self );
		if ( name && strcasestr( name, term ) != NULL ) { continue; }
		if ( description && strcasestr( description, term ) != NULL ) { continue; }
		if ( executable && strcasestr( executable, term ) != NULL ) { continue; }
		if ( categories && strcasestr( categories, term ) != NULL ) { continue; }
		if ( id && strcasestr( id, term ) != NULL ) { continue; }

		return FALSE;
	}

	return TRUE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the application service.
//
void foobar_application_service_class_init( FoobarApplicationServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_application_service_get_property;
	object_klass->finalize = foobar_application_service_finalize;

	props[PROP_ITEMS] = g_param_spec_object(
		"items",
		"Items",
		"Application items, sorted by their frequency.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the application service.
//
void foobar_application_service_init( FoobarApplicationService* self )
{
	self->items = g_list_store_new( FOOBAR_TYPE_APPLICATION_ITEM );

	GtkCustomSorter* sorter = gtk_custom_sorter_new( foobar_application_service_sort_func, NULL, NULL );
	self->sorted_items = gtk_sort_list_model_new( G_LIST_MODEL( g_object_ref( self->items ) ), GTK_SORTER( sorter ) );

	self->cache_path = foobar_get_cache_path( "application-frequencies.json" );
	self->frequencies = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, NULL );
	g_mutex_init( &self->frequencies_mutex );
	g_mutex_init( &self->write_cache_mutex );

	foobar_application_service_read_cache( self );
	foobar_application_service_update( self );

	self->monitor = g_app_info_monitor_get( );
	self->changed_handler_id = g_signal_connect(
		self->monitor,
		"changed",
		G_CALLBACK( foobar_application_service_handle_changed ),
		self );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_application_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarApplicationService* self = (FoobarApplicationService*)object;

	switch ( prop_id )
	{
		case PROP_ITEMS:
			g_value_set_object( value, foobar_application_service_get_items( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the application service.
//
void foobar_application_service_finalize( GObject* object )
{
	FoobarApplicationService* self = (FoobarApplicationService*)object;

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->items ) ); ++i )
	{
		FoobarApplicationItem* item = g_list_model_get_item( G_LIST_MODEL( self->items ), i );
		item->service = NULL;
		g_object_notify_by_pspec( G_OBJECT( item ), app_props[APP_PROP_FREQUENCY] );
	}
	g_clear_signal_handler( &self->changed_handler_id, self->monitor );
	g_clear_object( &self->sorted_items );
	g_clear_object( &self->items );
	g_clear_object( &self->monitor );
	g_clear_pointer( &self->frequencies, g_hash_table_unref );
	g_clear_pointer( &self->cache_path, g_free );

	g_mutex_clear( &self->frequencies_mutex );
	g_mutex_clear( &self->write_cache_mutex );

	G_OBJECT_CLASS( foobar_application_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new application service instance.
//
FoobarApplicationService* foobar_application_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_APPLICATION_SERVICE, NULL );
}

//
// Get a sorted list of all available applications.
//
GListModel* foobar_application_service_get_items( FoobarApplicationService* self )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_SERVICE( self ), NULL );

	return G_LIST_MODEL( self->sorted_items );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called by GLib when the list of applications has changed.
//
void foobar_application_service_handle_changed(
	GAppInfoMonitor* monitor,
	gpointer         userdata )
{
	(void)monitor;
	FoobarApplicationService* self = (FoobarApplicationService*)userdata;

	foobar_application_service_update( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Reload the list of applications.
//
void foobar_application_service_update( FoobarApplicationService* self )
{
	g_list_store_remove_all( self->items );

	g_autolist( GAppInfo ) info = g_app_info_get_all( );
	for ( GList* it = info; it; it = it->next )
	{
		if ( g_app_info_should_show( it->data ) )
		{
			g_autoptr( FoobarApplicationItem ) item = foobar_application_item_new( self );
			foobar_application_item_set_info( item, it->data );
			g_list_store_append( self->items, item );
		}
	}
}

//
// Synchronously read the frequency cache file at self->cache_path, populating self->frequencies.
//
void foobar_application_service_read_cache( FoobarApplicationService* self )
{
	g_autoptr( GError ) error = NULL;

	if ( self->cache_path && g_file_test( self->cache_path, G_FILE_TEST_EXISTS ) )
	{
		g_autoptr( JsonParser ) parser = json_parser_new( );
		if ( !json_parser_load_from_mapped_file( parser, self->cache_path, &error ) )
		{
			g_warning( "Unable to read cached application frequencies: %s", error->message );
			return;
		}

		JsonNode* root_node = json_parser_get_root( parser );
		JsonObject* root_object = json_node_get_object( root_node );
		json_object_foreach_member( root_object, foobar_application_service_read_cache_foreach_cb, self );
	}
}

//
// Callback used by foobar_application_service_read_cache to read a single entry.
//
void foobar_application_service_read_cache_foreach_cb(
	JsonObject*  object,
	gchar const* member_name,
	JsonNode*    member_node,
	gpointer     userdata )
{
	(void)object;
	FoobarApplicationService* self = (FoobarApplicationService*)userdata;

	gsize frequency = json_node_get_int( member_node );
	g_hash_table_insert( self->frequencies, g_strdup( member_name ), GSIZE_TO_POINTER( frequency ) );
}

//
// Asynchronously start writing the frequency cache file at self->cache_path.
//
void foobar_application_service_write_cache( FoobarApplicationService* self )
{
	foobar_application_service_write_cache_async( self, NULL, foobar_application_service_write_cache_cb, NULL );
}

//
// Callback invoked when the frequency cache was written successfully or failed.
//
void foobar_application_service_write_cache_cb(
	GObject*      object,
	GAsyncResult* result,
	gpointer      userdata )
{
	(void)userdata;
	FoobarApplicationService* self = (FoobarApplicationService*)object;

	g_autoptr( GError ) error = NULL;
	if ( !foobar_application_service_write_cache_finish( self, result, &error ) )
	{
		g_warning( "Unable to write cached application frequencies: %s", error->message );
	}
}

//
// Asynchronously write the frequency cache file at self->cache_path.
//
void foobar_application_service_write_cache_async(
	FoobarApplicationService* self,
	GCancellable*             cancellable,
	GAsyncReadyCallback       callback,
	gpointer                  userdata )
{
	g_autoptr( GTask ) task = g_task_new( self, cancellable, callback, userdata );
	g_task_set_name( task, "write-application-cache" );
	g_task_run_in_thread( task, foobar_application_service_write_cache_thread );
}

//
// Get the asynchronous result for writing the frequency cache file, returning TRUE on success, or FALSE on error.
//
gboolean foobar_application_service_write_cache_finish(
	FoobarApplicationService* self,
	GAsyncResult*             result,
	GError**                  error )
{
	(void)self;

	return g_task_propagate_boolean( G_TASK( result ), error );
}

//
// Task implementation for foobar_application_service_write_cache_async, invoked on a background thread.
//
void foobar_application_service_write_cache_thread(
	GTask*        task,
	gpointer      source_object,
	gpointer      task_data,
	GCancellable* cancellable )
{
	(void)cancellable;
	(void)task_data;
	FoobarApplicationService* self = (FoobarApplicationService*)source_object;

	if ( !self->cache_path )
	{
		g_task_return_boolean( task, TRUE );
		return;
	}

	g_autoptr( GError ) error = NULL;

	g_autoptr( JsonBuilder ) builder = json_builder_new( );
	json_builder_begin_object( builder );

	g_mutex_lock( &self->frequencies_mutex );
	g_hash_table_foreach( self->frequencies, foobar_application_service_write_cache_foreach_cb, builder );
	g_mutex_unlock( &self->frequencies_mutex );

	json_builder_end_object( builder );

	g_autoptr( JsonNode ) root_node = json_builder_get_root( builder );
	g_autoptr( JsonGenerator ) generator = json_generator_new( );
	json_generator_set_root( generator, root_node );

	g_mutex_lock( &self->write_cache_mutex );
	gboolean success = json_generator_to_file( generator, self->cache_path, &error );
	g_mutex_unlock( &self->write_cache_mutex );
	if ( !success )
	{
		g_task_return_error( task, g_steal_pointer( &error ) );
		return;
	}

	g_task_return_boolean( task, TRUE );
}

//
// Callback used by foobar_application_service_write_cache_thread to build a single entry.
//
void foobar_application_service_write_cache_foreach_cb(
	gpointer key,
	gpointer value,
	gpointer userdata )
{
	JsonBuilder* builder = (JsonBuilder*)userdata;

	json_builder_set_member_name( builder, key );
	json_builder_add_int_value( builder, GPOINTER_TO_SIZE( value ) );
}

//
// Sorting callback for application items. Items are sorted based on:
// 1. frequency (descending)
// 2. name (ascending)
// 3. id (ascending)
//
gint foobar_application_service_sort_func(
	gconstpointer item_a,
	gconstpointer item_b,
	gpointer      userdata )
{
	(void)userdata;

	FoobarApplicationItem* app_a = (FoobarApplicationItem*)item_a;
	FoobarApplicationItem* app_b = (FoobarApplicationItem*)item_b;

	gint64 freq_a = foobar_application_item_get_frequency( app_a );
	gint64 freq_b = foobar_application_item_get_frequency( app_b );
	if ( freq_a > freq_b ) { return -1; }
	if ( freq_a < freq_b ) { return 1; }

	gchar const* name_a = foobar_launcher_item_get_title( FOOBAR_LAUNCHER_ITEM( app_a ) );
	gchar const* name_b = foobar_launcher_item_get_title( FOOBAR_LAUNCHER_ITEM( app_b ) );
	gint name_res = g_strcmp0( name_a, name_b );
	if ( name_res ) { return name_res; }

	gchar const* id_a = foobar_application_item_get_id( app_a );
	gchar const* id_b = foobar_application_item_get_id( app_b );
	return g_strcmp0( id_a, id_b );
}

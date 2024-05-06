#include "services/workspace-service.h"
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>
#include <string.h>

typedef struct _EventData    EventData;
typedef struct _EventHandler EventHandler;

//
// FoobarWorkspaceFlags:
//
// Some information about the type/state of a workspace.
//

G_DEFINE_FLAGS_TYPE(
	FoobarWorkspaceFlags,
	foobar_workspace_flags,
	G_DEFINE_ENUM_VALUE( FOOBAR_WORKSPACE_FLAGS_NONE, "none" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_WORKSPACE_FLAGS_ACTIVE, "active" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_WORKSPACE_FLAGS_VISIBLE, "visible" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_WORKSPACE_FLAGS_SPECIAL, "special" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_WORKSPACE_FLAGS_PERSISTENT, "persistent" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_WORKSPACE_FLAGS_URGENT, "urgent" ) )

//
// FoobarWorkspace:
//
// Representation of a workspace.
//

struct _FoobarWorkspace
{
	GObject                 parent_instance;
	FoobarWorkspaceService* service;
	gint64                  id;
	gchar*                  name;
	gchar*                  monitor;
	FoobarWorkspaceFlags    flags;
};

enum
{
	WORKSPACE_PROP_ID = 1,
	WORKSPACE_PROP_NAME,
	WORKSPACE_PROP_MONITOR,
	WORKSPACE_PROP_FLAGS,
	N_WORKSPACE_PROPS,
};

static GParamSpec* workspace_props[N_WORKSPACE_PROPS] = { 0 };

static void             foobar_workspace_class_init  ( FoobarWorkspaceClass*   klass );
static void             foobar_workspace_init        ( FoobarWorkspace*        self );
static void             foobar_workspace_get_property( GObject*                object,
                                                       guint                   prop_id,
                                                       GValue*                 value,
                                                       GParamSpec*             pspec );
static void             foobar_workspace_finalize    ( GObject*                object );
static FoobarWorkspace* foobar_workspace_new         ( FoobarWorkspaceService* service );
static void             foobar_workspace_set_id      ( FoobarWorkspace*        self,
                                                       gint64                  value );
static void             foobar_workspace_set_name    ( FoobarWorkspace*        self,
                                                       gchar const*            value );
static void             foobar_workspace_set_monitor ( FoobarWorkspace*        self,
                                                       gchar const*            value );
static void             foobar_workspace_set_flags   ( FoobarWorkspace*        self,
                                                       FoobarWorkspaceFlags    value );

G_DEFINE_FINAL_TYPE( FoobarWorkspace, foobar_workspace, G_TYPE_OBJECT )

//
// FoobarWorkspaceService:
//
// Service monitoring the workspaces provided by the window manager. This is implemented by communicating directly with
// Hyprland through its IPC socket.
//
// Based on the waybar implementation:
// https://github.com/Alexays/Waybar/blob/master/src/modules/hyprland/workspaces.cpp
//

struct _FoobarWorkspaceService
{
	GObject           parent_instance;
	GListStore*       workspaces;
	GtkSortListModel* sorted_workspaces;
	GCancellable*     event_cancellable;
	GThread*          event_thread;
	gchar*            rx_path;
	gchar*            tx_path;
};

enum
{
	PROP_WORKSPACES = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

enum
{
	SIGNAL_MONITOR_CONFIGURATION_CHANGED,
	N_SIGNALS,
};
static unsigned signals[N_SIGNALS] = { 0 };

static void             foobar_workspace_service_class_init                        ( FoobarWorkspaceServiceClass* klass );
static void             foobar_workspace_service_init                              ( FoobarWorkspaceService*      self );
static void             foobar_workspace_service_get_property                      ( GObject*                     object,
                                                                                     guint                        prop_id,
                                                                                     GValue*                      value,
                                                                                     GParamSpec*                  pspec );
static void             foobar_workspace_service_dispose                           ( GObject*                     object );
static void             foobar_workspace_service_finalize                          ( GObject*                     object );
static gboolean         foobar_workspace_service_handle_workspace_created          ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_workspace_destroyed        ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_workspace_moved            ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_workspace_renamed          ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_workspace_activated        ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_special_workspace_activated( EventData*                   data );
static gboolean         foobar_workspace_service_handle_monitor_focused            ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_window_urgent              ( EventData*                   data );
static gboolean         foobar_workspace_service_handle_config_reloaded            ( EventData*                   data );
static gpointer         foobar_workspace_service_event_thread_func                 ( gpointer                     userdata );
static void             foobar_workspace_service_dispatch_event                    ( FoobarWorkspaceService*      self,
                                                                                     gchar*                       message );
static gchar*           foobar_workspace_service_send_request                      ( FoobarWorkspaceService*      self,
                                                                                     gchar const*                 request,
                                                                                     GError**                     error );
static JsonNode*        foobar_workspace_service_send_json_request                 ( FoobarWorkspaceService*      self,
                                                                                     gchar const*                 request,
                                                                                     GError**                     error );
static void             foobar_workspace_service_initialize                        ( FoobarWorkspaceService*      self );
static void             foobar_workspace_service_update_active                     ( FoobarWorkspaceService*      self,
                                                                                     GListStore*                  workspaces );
static void             foobar_workspace_service_add_workspace                     ( FoobarWorkspaceService*      self,
                                                                                     GListStore*                  workspaces,
                                                                                     JsonObject*                  workspace_object,
                                                                                     gboolean                     is_persistent );
static FoobarWorkspace* foobar_workspace_service_find_workspace                    ( GListModel*                  workspaces,
                                                                                     gint64                       id,
                                                                                     guint*                       out_index );
static gboolean         foobar_workspace_service_is_invalid_name                   ( gchar const*                 name );
static gint64           foobar_workspace_service_parse_id                          ( gchar const*                 str );
static gint             foobar_workspace_service_compare_ids                       ( gconstpointer                a,
                                                                                     gconstpointer                b );
static gint             foobar_workspace_service_sort_func                         ( gconstpointer                item_a,
                                                                                     gconstpointer                item_b,
                                                                                     gpointer                     userdata );

G_DEFINE_FINAL_TYPE( FoobarWorkspaceService, foobar_workspace_service, G_TYPE_OBJECT )

//
// EventData:
//
// Custom userdata passed to hyprland event handlers in a single pointer.
//

struct _EventData
{
	FoobarWorkspaceService* service;
	gchar*                  name;
	gchar*                  payload;
};

static EventData* event_data_new ( FoobarWorkspaceService* service,
                                   gchar const*            name,
                                   gchar const*            payload );
static void       event_data_free( EventData*              self );

//
// EventHandler:
//
// Description for the handler of a single hyprland event.
//

struct _EventHandler
{
	gchar const* name;
	gboolean ( *fn )( EventData* data );
};

//
// Static list of registered event handlers.
//
static EventHandler const event_handlers[] =
	{
		{ .name = "createworkspacev2",  .fn = foobar_workspace_service_handle_workspace_created },
		{ .name = "destroyworkspacev2", .fn = foobar_workspace_service_handle_workspace_destroyed },
		{ .name = "moveworkspacev2",    .fn = foobar_workspace_service_handle_workspace_moved },
		{ .name = "renameworkspace",    .fn = foobar_workspace_service_handle_workspace_renamed },
		{ .name = "workspace",          .fn = foobar_workspace_service_handle_workspace_activated },
		{ .name = "activespecial",      .fn = foobar_workspace_service_handle_special_workspace_activated },
		{ .name = "focusedmon",         .fn = foobar_workspace_service_handle_monitor_focused },
		{ .name = "urgent",             .fn = foobar_workspace_service_handle_window_urgent },
		{ .name = "configreloaded",     .fn = foobar_workspace_service_handle_config_reloaded },
	};

// ---------------------------------------------------------------------------------------------------------------------
// Workspace
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for workspaces.
//
void foobar_workspace_class_init( FoobarWorkspaceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_workspace_get_property;
	object_klass->finalize = foobar_workspace_finalize;

	workspace_props[WORKSPACE_PROP_ID] = g_param_spec_int64(
		"id",
		"ID",
		"Numeric ID of the workspace.",
		INT64_MIN,
		INT64_MAX,
		0,
		G_PARAM_READABLE );
	workspace_props[WORKSPACE_PROP_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Optional human-readable workspace name.",
		NULL,
		G_PARAM_READABLE );
	workspace_props[WORKSPACE_PROP_MONITOR] = g_param_spec_string(
		"monitor",
		"Monitor",
		"Connector ID of the workspace's monitor.",
		NULL,
		G_PARAM_READABLE );
	workspace_props[WORKSPACE_PROP_FLAGS] = g_param_spec_flags(
		"flags",
		"Flags",
		"Flags indicating the current state of the workspace.",
		FOOBAR_TYPE_WORKSPACE_FLAGS,
		FOOBAR_WORKSPACE_FLAGS_NONE,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_WORKSPACE_PROPS, workspace_props );
}

//
// Instance initialization for workspaces.
//
void foobar_workspace_init( FoobarWorkspace* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_workspace_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarWorkspace* self = (FoobarWorkspace*)object;

	switch ( prop_id )
	{
		case WORKSPACE_PROP_ID:
			g_value_set_int64( value, foobar_workspace_get_id( self ) );
			break;
		case WORKSPACE_PROP_NAME:
			g_value_set_string( value, foobar_workspace_get_name( self ) );
			break;
		case WORKSPACE_PROP_MONITOR:
			g_value_set_string( value, foobar_workspace_get_monitor( self ) );
			break;
		case WORKSPACE_PROP_FLAGS:
			g_value_set_flags( value, foobar_workspace_get_flags( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for workspaces.
//
void foobar_workspace_finalize( GObject* object )
{
	FoobarWorkspace* self = (FoobarWorkspace*)object;

	g_clear_pointer( &self->name, g_free );
	g_clear_pointer( &self->monitor, g_free );

	G_OBJECT_CLASS( foobar_workspace_parent_class )->finalize( object );
}

//
// Create a new workspace that is owned by the given service (captured as an unowned reference).
//
FoobarWorkspace* foobar_workspace_new( FoobarWorkspaceService* service )
{
	FoobarWorkspace* self = g_object_new( FOOBAR_TYPE_WORKSPACE, NULL );
	self->service = service;
	return self;
}

//
// Numeric ID of the workspace.
//
gint64 foobar_workspace_get_id( FoobarWorkspace* self )
{
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE( self ), 0 );
	return self->id;
}

//
// Optional human-readable workspace name.
//
gchar const* foobar_workspace_get_name( FoobarWorkspace* self )
{
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE( self ), NULL );
	return self->name;
}

//
// Connector ID of the workspace's monitor.
//
gchar const* foobar_workspace_get_monitor( FoobarWorkspace* self )
{
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE( self ), NULL );
	return self->monitor;
}

//
// Flags indicating the current state of the workspace.
//
FoobarWorkspaceFlags foobar_workspace_get_flags( FoobarWorkspace* self )
{
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE( self ), FOOBAR_WORKSPACE_FLAGS_NONE );
	return self->flags;
}

//
// Numeric ID of the workspace.
//
void foobar_workspace_set_id(
	FoobarWorkspace* self,
	gint64           value )
{
	g_return_if_fail( FOOBAR_IS_WORKSPACE( self ) );

	if ( self->id != value )
	{
		self->id = value;
		g_object_notify_by_pspec( G_OBJECT( self ), workspace_props[WORKSPACE_PROP_ID] );
	}
}

//
// Optional human-readable workspace name.
//
void foobar_workspace_set_name(
	FoobarWorkspace* self,
	gchar const*     value )
{
	g_return_if_fail( FOOBAR_IS_WORKSPACE( self ) );

	if ( g_strcmp0( self->name, value ) )
	{
		g_clear_pointer( &self->name, g_free );
		self->name = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), workspace_props[WORKSPACE_PROP_NAME] );
	}
}

//
// Connector ID of the workspace's monitor.
//
void foobar_workspace_set_monitor(
	FoobarWorkspace* self,
	gchar const*     value )
{
	g_return_if_fail( FOOBAR_IS_WORKSPACE( self ) );

	if ( g_strcmp0( self->monitor, value ) )
	{
		g_clear_pointer( &self->monitor, g_free );
		self->monitor = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), workspace_props[WORKSPACE_PROP_MONITOR] );
	}
}

//
// Flags indicating the current state of the workspace.
//
void foobar_workspace_set_flags(
	FoobarWorkspace*     self,
	FoobarWorkspaceFlags value )
{
	g_return_if_fail( FOOBAR_IS_WORKSPACE( self ) );

	if ( self->flags != value )
	{
		self->flags = value;
		g_object_notify_by_pspec( G_OBJECT( self ), workspace_props[WORKSPACE_PROP_FLAGS] );
	}
}

//
// Let this workspace become the active one.
//
void foobar_workspace_activate( FoobarWorkspace* self )
{
	g_return_if_fail( FOOBAR_IS_WORKSPACE( self ) );
	if ( !self->service ) { return; }

	g_autofree gchar* request = NULL;
	if ( foobar_workspace_get_id( self ) > 0 )
	{
		// Normal
		request = g_strdup_printf( "dispatch workspace %lld", (long long)foobar_workspace_get_id( self ) );
	}
	else if ( !( foobar_workspace_get_flags( self ) & FOOBAR_WORKSPACE_FLAGS_SPECIAL ) )
	{
		// Named (this includes persistent)
		request = g_strdup_printf( "dispatch workspace name:%s", foobar_workspace_get_name( self ) );
	}
	else if ( foobar_workspace_get_id( self ) != -99 )
	{
		// Named special
		request = g_strdup_printf( "dispatch togglespecialworkspace %s", foobar_workspace_get_name( self ) );
	}
	else
	{
		// Special
		request = g_strdup( "dispatch togglespecialworkspace" );
	}

	g_autoptr( GError ) error = NULL;
	g_free( foobar_workspace_service_send_request( self->service, request, &error ) );
	if ( error )
	{
		g_warning( "Unable to activate workspace: %s", error->message );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the workspace service.
//
void foobar_workspace_service_class_init( FoobarWorkspaceServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_workspace_service_get_property;
	object_klass->dispose = foobar_workspace_service_dispose;
	object_klass->finalize = foobar_workspace_service_finalize;

	props[PROP_WORKSPACES] = g_param_spec_object(
		"workspaces",
		"Workspaces",
		"Sorted list of all workspaces.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );

	signals[SIGNAL_MONITOR_CONFIGURATION_CHANGED] = g_signal_new(
		"monitor-configuration-changed",
		FOOBAR_TYPE_WORKSPACE_SERVICE,
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		0,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0 );
}

//
// Instance initialization for the workspace service.
//
void foobar_workspace_service_init( FoobarWorkspaceService* self )
{
	self->workspaces = g_list_store_new( FOOBAR_TYPE_WORKSPACE );

	GtkCustomSorter* sorter = gtk_custom_sorter_new( foobar_workspace_service_sort_func, NULL, NULL );
	self->sorted_workspaces = gtk_sort_list_model_new(
		G_LIST_MODEL( g_object_ref( self->workspaces ) ),
		GTK_SORTER( sorter ) );

	gchar const* instance_signature = g_getenv( "HYPRLAND_INSTANCE_SIGNATURE" );
	if ( !instance_signature )
	{
		g_warning( "Unable to connect to hyprland -- is it currently running?" );
		return;
	}

	self->tx_path = g_strdup_printf( "/tmp/hypr/%s/.socket.sock", instance_signature );
	self->rx_path = g_strdup_printf( "/tmp/hypr/%s/.socket2.sock", instance_signature );
	self->event_cancellable = g_cancellable_new( );
	self->event_thread = g_thread_new( "workspace-event-listener", foobar_workspace_service_event_thread_func, self );

	foobar_workspace_service_initialize( self );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_workspace_service_get_property( GObject* object, guint prop_id, GValue* value, GParamSpec* pspec )
{
	FoobarWorkspaceService* self = (FoobarWorkspaceService*)object;

	switch ( prop_id )
	{
		case PROP_WORKSPACES:
			g_value_set_object( value, foobar_workspace_service_get_workspaces( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for the workspace service.
//
// This is where the event thread is joined because it might still create new object references which is a bad thing to
// do in finalize.
//
void foobar_workspace_service_dispose( GObject* object )
{
	FoobarWorkspaceService* self = (FoobarWorkspaceService*)object;

	if ( self->event_thread )
	{
		g_cancellable_cancel( self->event_cancellable );
		g_thread_join( self->event_thread );
		self->event_thread = NULL;
	}

	G_OBJECT_CLASS( foobar_workspace_service_parent_class )->dispose( object );
}

//
// Instance cleanup for the workspace service.
//
void foobar_workspace_service_finalize( GObject* object )
{
	FoobarWorkspaceService* self = (FoobarWorkspaceService*)object;

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->workspaces ) ); ++i )
	{
		FoobarWorkspace* workspace = g_list_model_get_item( G_LIST_MODEL( self->workspaces ), i );
		workspace->service = NULL;
	}
	g_clear_object( &self->event_cancellable );
	g_clear_object( &self->sorted_workspaces );
	g_clear_object( &self->workspaces );
	g_clear_pointer( &self->rx_path, g_free );
	g_clear_pointer( &self->tx_path, g_free );

	G_OBJECT_CLASS( foobar_workspace_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new workspace service instance.
//
FoobarWorkspaceService* foobar_workspace_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_WORKSPACE_SERVICE, NULL );
}

//
// Get a sorted list of all workspaces.
//
GListModel* foobar_workspace_service_get_workspaces( FoobarWorkspaceService* self )
{
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->sorted_workspaces );
}

// ---------------------------------------------------------------------------------------------------------------------
// Event Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Handler for the "createworkspacev2" event.
//
// The payload has the format "<id>,<name>" where <id> is the numeric ID.
//
gboolean foobar_workspace_service_handle_workspace_created( EventData* data )
{
	gchar* save;
	gchar const* id_str = strtok_r( data->payload, ",", &save );
	gchar const* name = strtok_r( NULL, ",", &save );
	g_return_val_if_fail( id_str && name, G_SOURCE_REMOVE );

	if ( foobar_workspace_service_is_invalid_name( name ) ) { return G_SOURCE_REMOVE; }
	gint64 id = foobar_workspace_service_parse_id( id_str );

	// Get additional information about the workspace.

	g_autoptr( GError ) error = NULL;
	g_autoptr( JsonNode ) workspaces_node = foobar_workspace_service_send_json_request(
		data->service,
		"j/workspaces",
		&error );
	if ( !workspaces_node )
	{
		g_warning( "Unable to load workspaces: %s", error->message );
		return G_SOURCE_REMOVE;
	}

	JsonArray* workspaces_array = json_node_get_array( workspaces_node );
	JsonObject* workspace_object = NULL;
	for ( guint i = 0; i < json_array_get_length( workspaces_array ); ++i )
	{
		JsonObject* it = json_array_get_object_element( workspaces_array, i );
		if ( id == json_object_get_int_member( it, "id" ) )
		{
			workspace_object = it;
			break;
		}
	}

	if ( !workspace_object )
	{
		g_warning( "Unable to find new workspace." );
		return G_SOURCE_REMOVE;
	}

	// Check if the workspace is persistent by looking for a rule with its ID.

	gboolean is_persistent = FALSE;

	g_autoptr( JsonNode ) rules_node = foobar_workspace_service_send_json_request(
		data->service,
		"j/workspacerules",
		&error );
	if ( !rules_node )
	{
		g_warning( "Unable to load workspace rules: %s", error->message );
		return G_SOURCE_REMOVE;
	}

	JsonArray* rules_array = json_node_get_array( rules_node );
	for ( guint i = 0; i < json_array_get_length( rules_array ); ++i )
	{
		JsonObject* rule_object = json_array_get_object_element( rules_array, i );
		gchar const* rule_id_str = json_object_get_string_member_with_default( rule_object, "workspaceString", NULL );
		if ( id == foobar_workspace_service_parse_id( rule_id_str ) )
		{
			is_persistent = json_object_get_boolean_member_with_default( rule_object, "persistent", FALSE );
			break;
		}
	}

	foobar_workspace_service_add_workspace( data->service, data->service->workspaces, workspace_object, is_persistent );
	foobar_workspace_service_update_active( data->service, data->service->workspaces );
	return G_SOURCE_REMOVE;
}

//
// Handler for the "destroyworkspacev2" event.
//
// The payload has the format "<id>,<name>" where <id> is the numeric ID.
//
gboolean foobar_workspace_service_handle_workspace_destroyed( EventData* data )
{
	gchar* save;
	gchar const* id_str = strtok_r( data->payload, ",", &save );
	gchar const* name = strtok_r( NULL, ",", &save );
	g_return_val_if_fail( id_str && name, G_SOURCE_REMOVE );

	if ( foobar_workspace_service_is_invalid_name( name ) ) { return G_SOURCE_REMOVE; }
	gint64 id = foobar_workspace_service_parse_id( id_str );

	GListStore* workspaces = data->service->workspaces;
	guint index;
	FoobarWorkspace* workspace = foobar_workspace_service_find_workspace( G_LIST_MODEL( workspaces ), id, &index );
	if ( workspace )
	{
		workspace->service = NULL;
		g_list_store_remove( workspaces, index );
	}

	return G_SOURCE_REMOVE;
}

//
// Handler for the "moveworkspacev2" event.
//
// The payload has the format "<id>,<name>,<monitor>" where <id> is the numeric ID of the workspace.
//
gboolean foobar_workspace_service_handle_workspace_moved( EventData* data )
{
	gchar* save;
	gchar const* workspace_id_str = strtok_r( data->payload, ",", &save );
	gchar const* workspace_name = strtok_r( NULL, ",", &save );
	gchar const* monitor_name = strtok_r( NULL, ",", &save );
	g_return_val_if_fail( workspace_id_str && workspace_name && monitor_name, G_SOURCE_REMOVE );

	gint64 workspace_id = foobar_workspace_service_parse_id( workspace_id_str );
	FoobarWorkspace* workspace = foobar_workspace_service_find_workspace(
		G_LIST_MODEL( data->service->workspaces ),
		workspace_id,
		NULL );
	if ( workspace )
	{
		foobar_workspace_set_monitor( workspace, monitor_name );
		g_signal_emit( data->service, signals[SIGNAL_MONITOR_CONFIGURATION_CHANGED], 0 );
	}
	foobar_workspace_service_update_active( data->service, data->service->workspaces );

	return G_SOURCE_REMOVE;
}

//
// Handler for the "renameworkspace" event.
//
// The payload has the format "<id>,<name>" where <id> is the numeric ID of the workspace.
//
gboolean foobar_workspace_service_handle_workspace_renamed( EventData* data )
{
	gchar* save;
	gchar const* id_str = strtok_r( data->payload, ",", &save );
	gchar const* new_name = strtok_r( NULL, ",", &save );
	g_return_val_if_fail( id_str && new_name, G_SOURCE_REMOVE );

	gint64 id = foobar_workspace_service_parse_id( id_str );
	FoobarWorkspace* workspace = foobar_workspace_service_find_workspace(
		G_LIST_MODEL( data->service->workspaces ),
		id,
		NULL );
	if ( workspace )
	{
		foobar_workspace_set_name( workspace, new_name );
		gtk_sorter_changed(
			gtk_sort_list_model_get_sorter( data->service->sorted_workspaces ),
			GTK_SORTER_CHANGE_DIFFERENT );
	}

	return G_SOURCE_REMOVE;
}

//
// Handler for the "workspace" event.
//
gboolean foobar_workspace_service_handle_workspace_activated( EventData* data )
{
	foobar_workspace_service_update_active( data->service, data->service->workspaces );

	return G_SOURCE_REMOVE;
}

//
// Handler for the "activespecial" event.
//
gboolean foobar_workspace_service_handle_special_workspace_activated( EventData* data )
{
	foobar_workspace_service_update_active( data->service, data->service->workspaces );

	return G_SOURCE_REMOVE;
}

//
// Handler for the "focusedmon" event.
//
gboolean foobar_workspace_service_handle_monitor_focused( EventData* data )
{
	foobar_workspace_service_update_active( data->service, data->service->workspaces );

	return G_SOURCE_REMOVE;
}

//
// Handler for the "urgent" event.
//
// The payload has the format "<address>" where <address> is the sender window's address.
//
gboolean foobar_workspace_service_handle_window_urgent( EventData* data )
{
	gchar const* address = data->payload;

	// Find the client with the given address.

	g_autoptr( GError ) error = NULL;
	g_autoptr( JsonNode ) clients_node = foobar_workspace_service_send_json_request(
		data->service,
		"j/clients",
		&error );
	if ( !clients_node )
	{
		g_warning( "Unable to load clients: %s", error->message );
		return G_SOURCE_REMOVE;
	}

	JsonArray* clients_array = json_node_get_array( clients_node );
	JsonObject* client_object = NULL;
	for ( guint i = 0; i < json_array_get_length( clients_array ); ++i )
	{
		JsonObject*  it = json_array_get_object_element( clients_array, i );
		gchar const* it_address = json_object_get_string_member_with_default( it, "address", "" );
		if ( g_str_has_suffix( it_address, address ) )
		{
			client_object = it;
			break;
		}
	}

	if ( !client_object ) { return G_SOURCE_REMOVE; }

	// Find the workspace this window belongs to and mark it as "urgent".

	JsonObject* workspace_object = json_object_get_object_member( client_object, "workspace" );
	gint64 id = json_object_get_int_member( workspace_object, "id" );
	FoobarWorkspace* workspace = foobar_workspace_service_find_workspace(
		G_LIST_MODEL( data->service->workspaces ),
		id,
		NULL );
	if ( workspace )
	{
		FoobarWorkspaceFlags flags = foobar_workspace_get_flags( workspace ) | FOOBAR_WORKSPACE_FLAGS_URGENT;
		foobar_workspace_set_flags( workspace, flags );
	}

	return G_SOURCE_REMOVE;
}

//
// Handler for the "configreloaded" event.
//
gboolean foobar_workspace_service_handle_config_reloaded( EventData* data )
{
	foobar_workspace_service_initialize( data->service );

	return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Implementation of the event thread.
//
// This thread listens on hyprland's event socket (socket.2.sock) for incoming event notifications and then submits
// their registered handlers (if any) to the main loop.
//
gpointer foobar_workspace_service_event_thread_func( gpointer userdata )
{
	FoobarWorkspaceService* self = (FoobarWorkspaceService*)userdata;

	g_autoptr( GError ) error = NULL;
	g_autoptr( GSocket ) sock = g_socket_new(
		G_SOCKET_FAMILY_UNIX,
		G_SOCKET_TYPE_STREAM,
		G_SOCKET_PROTOCOL_DEFAULT,
		&error );
	if ( !sock )
	{
		g_warning( "Unable to create socket for hyprland communication: %s", error->message );
		return NULL;
	}

	g_autoptr( GSocketAddress ) addr = g_unix_socket_address_new( self->rx_path );
	if ( !g_socket_connect( sock, addr, self->event_cancellable, &error ) &&
			!g_cancellable_is_cancelled( self->event_cancellable ) )
	{
		g_warning( "Unable to connect socket to hyprland: %s", error->message );
		return NULL;
	}

	// Each event is sent as a complete line, separated by a newline character.

	GString* prev_buf = NULL; // for longer, incomplete messages
	gchar buf[1024];
	while ( !g_cancellable_is_cancelled( self->event_cancellable ) )
	{
		// Synchronously receive data from the socket, but support cancellation using Gio.

		gssize received = g_socket_receive( sock, buf, sizeof( buf ), self->event_cancellable, &error );
		if ( error && !g_cancellable_is_cancelled( self->event_cancellable ) )
		{
			g_warning( "Unable to receive events from hyprland: %s", error->message );
		}
		if ( received <= 0 ) { break; }

		gchar* start = buf;
		gchar* end = start + received;
		while ( start < end )
		{
			gchar* next_newline = g_strstr_len( start, end - start, "\n" );
			if ( !next_newline )
			{
				// Incomplete message (missing newline) -> store it in the buffer

				prev_buf = ( prev_buf != NULL )
					? g_string_append_len( prev_buf, start, end - start )
					: g_string_new_len( start, end - start );
				break;
			}

			if ( prev_buf )
			{
				// Prepend buffered data before the actual data.

				prev_buf = g_string_append_len( prev_buf, start, next_newline - start );
				foobar_workspace_service_dispatch_event( self, prev_buf->str );

				g_string_free( prev_buf, TRUE );
				prev_buf = NULL;
			}
			else
			{
				// Skip allocation and use the buffer contents on the stack directly.

				*next_newline = '\0';
				foobar_workspace_service_dispatch_event( self, start );
			}

			start = next_newline + 1;
		}
	}

	if ( prev_buf ) { g_string_free( prev_buf, TRUE ); }

	return NULL;
}

//
// Called when an entire event has been received on the event thread.
//
// This will look through the registered handlers and (if found) invoke the event's handler on the main thread.
//
void foobar_workspace_service_dispatch_event(
	FoobarWorkspaceService* self,
	gchar*                  message )
{
	gchar* delimiter = strstr( message, ">>" );
	if ( !delimiter )
	{
		g_warning( "Received an invalid message from hyprland: %s", message );
		return;
	}

	*delimiter = '\0';
	gchar* name = message;
	gchar* payload = delimiter + 2;
	for ( gsize i = 0; i < G_N_ELEMENTS( event_handlers ); ++i )
	{
		EventHandler handler = event_handlers[i];
		if ( !strcmp( name, handler.name ) )
		{
			g_idle_add_full(
				G_PRIORITY_DEFAULT,
				G_SOURCE_FUNC( handler.fn ),
				event_data_new( self, name, payload ),
				(GDestroyNotify)event_data_free );
			break;
		}
	}
}

//
// Synchronously send a command to hyprland's control socket (socket.sock), returning the entire response.
//
gchar* foobar_workspace_service_send_request(
	FoobarWorkspaceService* self,
	gchar const*            request,
	GError**                error )
{
	g_autoptr( GSocket ) sock = g_socket_new(
		G_SOCKET_FAMILY_UNIX,
		G_SOCKET_TYPE_STREAM,
		G_SOCKET_PROTOCOL_DEFAULT,
		error );
	if ( !sock ) { return NULL; }

	g_autoptr( GSocketAddress ) addr = g_unix_socket_address_new( self->tx_path );
	if ( !g_socket_connect( sock, addr, NULL, error ) ) { return NULL; }

	if ( g_socket_send( sock, request, strlen( request ), NULL, error ) == -1 ) { return NULL; }

	GString* str = g_string_sized_new( 0 );
	gchar    buf[8192];
	while ( TRUE )
	{
		gssize received = g_socket_receive( sock, buf, sizeof( buf ), self->event_cancellable, error );
		if ( received < 0 ) { return g_string_free( str, TRUE ); }

		if ( received == 0 ) { break; }
		str = g_string_append_len( str, buf, received );
	}

	return g_string_free( str, FALSE );
}

//
// Synchronously send a command to hyprland's control socket (socket.sock), returning the parsed the JSON response.
//
JsonNode* foobar_workspace_service_send_json_request(
	FoobarWorkspaceService* self,
	gchar const*            request,
	GError**                error )
{
	g_autofree gchar* json = foobar_workspace_service_send_request( self, request, error );
	if ( !json ) { return NULL; }

	g_autoptr( JsonParser ) parser = json_parser_new( );
	if ( !json_parser_load_from_data( parser, json, (gssize)strlen( json ), error ) ) { return NULL; }

	return json_parser_steal_root( parser );
}

//
// Refresh the list of workspaces by actively sending a request to hyprland.
//
void foobar_workspace_service_initialize( FoobarWorkspaceService* self )
{
	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->workspaces ) ); ++i )
	{
		FoobarWorkspace* workspace = g_list_model_get_item( G_LIST_MODEL( self->workspaces ), i );
		workspace->service = NULL;
	}
	g_list_store_remove_all( self->workspaces );
	g_autoptr( GListStore ) workspaces = g_list_store_new( FOOBAR_TYPE_WORKSPACE );

	{
		g_autoptr( GError ) error = NULL;
		g_autoptr( JsonNode ) workspaces_node = foobar_workspace_service_send_json_request(
			self,
			"j/workspaces",
			&error );
		if ( !workspaces_node )
		{
			g_warning( "Unable to load workspaces: %s", error->message );
			return;
		}

		JsonArray* workspaces_array = json_node_get_array( workspaces_node );
		for ( guint i = 0; i < json_array_get_length( workspaces_array ); ++i )
		{
			JsonObject* workspace_object = json_array_get_object_element( workspaces_array, i );
			foobar_workspace_service_add_workspace( self, workspaces, workspace_object, FALSE );
		}
	}

	{
		// Load configured persistent workspaces.

		g_autoptr( GError ) error = NULL;
		g_autoptr( JsonNode ) rules_node = foobar_workspace_service_send_json_request(
			self,
			"j/workspacerules",
			&error );
		if ( rules_node )
		{
			JsonArray* rules_array = json_node_get_array( rules_node );
			for ( guint i = 0; i < json_array_get_length( rules_array ); ++i )
			{
				JsonObject*  rule_object = json_array_get_object_element( rules_array, i );
				gchar const* name = json_object_get_string_member_with_default( rule_object, "workspaceString", NULL );
				if ( !name )
				{
					g_warning( "Found an invalid workspaceString value, skipping." );
					continue;
				}

				if ( !json_object_get_boolean_member_with_default( rule_object, "persistent", FALSE ) ) { continue; }

				gint64 id = foobar_workspace_service_parse_id( name );
				json_object_set_int_member( rule_object, "id", id );
				json_object_set_string_member( rule_object, "name", name );

				foobar_workspace_service_add_workspace( self, workspaces, rule_object, TRUE );
			}
		}
		else { g_warning( "Unable to load workspace rules: %s", error->message ); }
	}

	foobar_workspace_service_update_active( self, workspaces );

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( workspaces ) ); ++i )
	{
		FoobarWorkspace* workspace = g_list_model_get_item( G_LIST_MODEL( workspaces ), i );
		g_list_store_append( self->workspaces, workspace );
	}
}

//
// Update the currently active/visible workspaces by actively sending a request to hyprland.
//
void foobar_workspace_service_update_active(
	FoobarWorkspaceService* self,
	GListStore*             workspaces )
{
	g_autoptr( GArray ) visible_workspace_ids = g_array_new( FALSE, FALSE, sizeof( gint64 ) );
	gint64 active_workspace_id = 0;
	gint64 active_special_workspace_id = 0;

	{
		// Get all active workspaces

		g_autoptr( GError ) error = NULL;
		g_autoptr( JsonNode ) monitors_node = foobar_workspace_service_send_json_request(
			self,
			"j/monitors",
			&error );
		if ( monitors_node )
		{
			JsonArray* monitors_array = json_node_get_array( monitors_node );
			for ( guint i = 0; i < json_array_get_length( monitors_array ); ++i )
			{
				JsonObject* monitor_object = json_array_get_object_element( monitors_array, i );
				gint64 workspace_id = 0;
				gint64 special_workspace_id = 0;

				if ( json_object_has_member( monitor_object, "activeWorkspace" ) )
				{
					JsonObject* workspace_object = json_object_get_object_member( monitor_object, "activeWorkspace" );
					workspace_id = json_object_get_int_member_with_default( workspace_object, "id", 0 );
					if ( workspace_id ) { g_array_append_val( visible_workspace_ids, workspace_id ); }
				}

				if ( json_object_has_member( monitor_object, "specialWorkspace" ) )
				{
					JsonObject* workspace_object = json_object_get_object_member( monitor_object, "specialWorkspace" );
					special_workspace_id = json_object_get_int_member_with_default( workspace_object, "id", 0 );
					if ( special_workspace_id ) { g_array_append_val( visible_workspace_ids, special_workspace_id ); }
				}

				// If this is the focused monitor, then the visible workspace is also the active one.

				if ( json_object_get_boolean_member_with_default( monitor_object, "focused", FALSE ) )
				{
					active_workspace_id = workspace_id;
					active_special_workspace_id = special_workspace_id;
				}
			}
		}
		else { g_warning( "Unable to load monitors: %s", error->message ); }
	}

	// Sort visible workspaces to allow binary search.

	g_array_sort( visible_workspace_ids, foobar_workspace_service_compare_ids );

	// Go through all workspaces and update their state.

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( workspaces ) ); ++i )
	{
		FoobarWorkspace* workspace = g_list_model_get_item( G_LIST_MODEL( workspaces ), i );
		FoobarWorkspaceFlags flags = foobar_workspace_get_flags( workspace );
		gint64 id = foobar_workspace_get_id( workspace );

		// Active
		gboolean is_active = id == active_workspace_id || id == active_special_workspace_id;
		flags = is_active ? flags | FOOBAR_WORKSPACE_FLAGS_ACTIVE : flags & ~FOOBAR_WORKSPACE_FLAGS_ACTIVE;

		// Disable urgency if workspace is active
		if ( is_active ) { flags = flags & ~FOOBAR_WORKSPACE_FLAGS_URGENT; }

		// Visible
		gboolean is_visible = g_array_binary_search(
			visible_workspace_ids,
			&id,
			foobar_workspace_service_compare_ids,
			NULL );
		flags = is_visible ? flags | FOOBAR_WORKSPACE_FLAGS_VISIBLE : flags & ~FOOBAR_WORKSPACE_FLAGS_VISIBLE;

		foobar_workspace_set_flags( workspace, flags );
	}
}

//
// Add or update a workspace described by a JSON object.
//
void foobar_workspace_service_add_workspace(
	FoobarWorkspaceService* self,
	GListStore*             workspaces,
	JsonObject*             workspace_object,
	gboolean                is_persistent )
{
	gchar const* name = json_object_get_string_member( workspace_object, "name" );
	gint64 id = json_object_get_int_member( workspace_object, "id" );
	gboolean is_special = g_str_has_prefix( name, "special" );
	if ( g_str_has_prefix( name, "name:" ) ) { name += strlen( "name:" ); }
	else if ( g_str_has_prefix( name, "special:" ) ) { name += strlen( "special:" ); }

	FoobarWorkspace* existing = foobar_workspace_service_find_workspace( G_LIST_MODEL( workspaces ), id, NULL );
	if ( existing )
	{
		// Only update persistence.

		FoobarWorkspaceFlags flags = foobar_workspace_get_flags( existing );
		flags = is_persistent ? flags | FOOBAR_WORKSPACE_FLAGS_PERSISTENT : flags & ~FOOBAR_WORKSPACE_FLAGS_PERSISTENT;
		foobar_workspace_set_flags( existing, flags );
	}
	else
	{
		// Found a new workspace -> create a new object for it.

		FoobarWorkspaceFlags flags = FOOBAR_WORKSPACE_FLAGS_NONE;
		flags = is_persistent ? flags | FOOBAR_WORKSPACE_FLAGS_PERSISTENT : flags;
		flags = is_special ? flags | FOOBAR_WORKSPACE_FLAGS_SPECIAL : flags;

		g_autoptr( FoobarWorkspace ) workspace = foobar_workspace_new( self );
		foobar_workspace_set_id( workspace, id );
		foobar_workspace_set_name( workspace, name );
		foobar_workspace_set_monitor( workspace, json_object_get_string_member( workspace_object, "monitor" ) );
		foobar_workspace_set_flags( workspace, flags );
		g_list_store_append( workspaces, workspace );
	}
}

//
// Find an existing workspace object by its ID.
//
FoobarWorkspace* foobar_workspace_service_find_workspace(
	GListModel* workspaces,
	gint64      id,
	guint*      out_index )
{
	for ( guint i = 0; i < g_list_model_get_n_items( workspaces ); ++i )
	{
		FoobarWorkspace* workspace = g_list_model_get_item( workspaces, i );
		if ( foobar_workspace_get_id( workspace ) == id )
		{
			if ( out_index ) { *out_index = i; }
			return workspace;
		}
	}

	return NULL;
}

//
// Hyprland's IPC sometimes reports the creation of workspaces strangely named `special:special:<some_name>`. This
// function checks for that and is used to avoid creating (and then removing) such workspaces.
//
// See hyprwm/Hyprland#3424 for more info.
//
gboolean foobar_workspace_service_is_invalid_name( gchar const* name )
{
	return g_str_has_prefix( name, "special:special:" );
}

//
// Parse an ID string and return its numeric code.
//
gint64 foobar_workspace_service_parse_id( gchar const* str )
{
	if ( !g_strcmp0( str, "special" ) ) { return -99; }

	return strtoll( str, NULL, 10 );
}

//
// Comparison function for two workspace IDs, used for binary search.
//
gint foobar_workspace_service_compare_ids(
	gconstpointer a,
	gconstpointer b )
{
	gint64 const* a_int = a;
	gint64 const* b_int = b;

	if ( *a_int < *b_int ) { return -1; }
	if ( *a_int > *b_int ) { return 1; }
	return 0;
}

//
// Sorting callback for workspaces. Items are sorted to be in the following order:
// 1. normal
// 2. named persistent
// 3. named
// 4. special
// 5. named special
//
gint foobar_workspace_service_sort_func(
	gconstpointer item_a,
	gconstpointer item_b,
	gpointer      userdata )
{
	(void)userdata;

	FoobarWorkspace* workspace_a = (FoobarWorkspace*)item_a;
	FoobarWorkspace* workspace_b = (FoobarWorkspace*)item_b;
	gint64 id_a = foobar_workspace_get_id( workspace_a );
	gint64 id_b = foobar_workspace_get_id( workspace_b );
	gboolean special_a = ( foobar_workspace_get_flags( workspace_a ) & FOOBAR_WORKSPACE_FLAGS_SPECIAL ) != 0;
	gboolean special_b = ( foobar_workspace_get_flags( workspace_b ) & FOOBAR_WORKSPACE_FLAGS_SPECIAL ) != 0;
	gboolean named_a = id_a < 0;
	gboolean named_b = id_b < 0;

	// both normal (includes numbered persistent) => sort by ID
	if ( !named_a && !named_b )
	{
		if ( id_a < id_b ) { return -1; }
		if ( id_a > id_b ) { return 1; }
		return 0;
	}

	// one normal, one special => normal first
	if ( special_a && !special_b ) { return 1; }
	if ( !special_a && special_b ) { return -1; }

	// only one normal, one named
	if ( named_a && !named_b ) { return -1; }
	if ( !named_a && named_b ) { return 1; }

	// both special
	if ( special_a && special_b )
	{
		// if one is -99 => put it last
		if ( id_a == -99 && id_b != -99 ) { return 1; }
		if ( id_a != -99 && id_b == -99 ) { return -1; }
	}

	return g_strcmp0( foobar_workspace_get_name( workspace_a ), foobar_workspace_get_name( workspace_b ) );
}

//
// Create a new EventData structure.
//
static EventData* event_data_new(
	FoobarWorkspaceService* service,
	gchar const*            name,
	gchar const*            payload )
{
	EventData* self = g_new0( EventData, 1 );
	self->service = g_object_ref( service );
	self->name = g_strdup( name );
	self->payload = g_strdup( payload );
	return self;
}

//
// Destroy an EventData structure.
//
static void event_data_free( EventData* self )
{
	if ( self )
	{
		g_object_unref( self->service );
		g_free( self->name );
		g_free( self->payload );
		g_free( self );
	}
}
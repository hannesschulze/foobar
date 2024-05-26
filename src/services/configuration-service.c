#include "services/configuration-service.h"
#include <gio/gio.h>

#define UPDATE_APPLY_DELAY           250
#define CONFIGURATION_LOG_DOMAIN     "foobar.conf"
#define CONFIGURATION_WARNING( ... ) g_log( CONFIGURATION_LOG_DOMAIN, G_LOG_LEVEL_WARNING, __VA_ARGS__ )

//
// FoobarScreenEdge:
//
// An edge of the screen.
//

G_STATIC_ASSERT( sizeof( FoobarScreenEdge ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarScreenEdge,
	foobar_screen_edge,
	G_DEFINE_ENUM_VALUE( FOOBAR_SCREEN_EDGE_LEFT, "left" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_SCREEN_EDGE_RIGHT, "right" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_SCREEN_EDGE_TOP, "top" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_SCREEN_EDGE_BOTTOM, "bottom" ) )

//
// FoobarOrientation:
//
// An orientation value.
//

G_STATIC_ASSERT( sizeof( FoobarOrientation ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarOrientation,
	foobar_orientation,
	G_DEFINE_ENUM_VALUE( FOOBAR_ORIENTATION_HORIZONTAL, "horizontal" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_ORIENTATION_VERTICAL, "vertical" ) )

//
// FoobarStatusItem:
//
// An item to display in a status panel item.
//

G_STATIC_ASSERT( sizeof( FoobarStatusItem ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarStatusItem,
	foobar_status_item,
	G_DEFINE_ENUM_VALUE( FOOBAR_STATUS_ITEM_NETWORK, "network" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_STATUS_ITEM_BLUETOOTH, "bluetooth" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_STATUS_ITEM_BATTERY, "battery" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_STATUS_ITEM_BRIGHTNESS, "brightness" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_STATUS_ITEM_AUDIO, "audio" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_STATUS_ITEM_NOTIFICATIONS, "notifications" ) )

//
// FoobarPanelItemKind:
//
// Enum representing the various panel item types.
//

G_STATIC_ASSERT( sizeof( FoobarPanelItemKind ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarPanelItemKind,
	foobar_panel_item_kind,
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_KIND_ICON, "icon" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_KIND_CLOCK, "clock" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_KIND_WORKSPACES, "workspaces" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_KIND_STATUS, "status" ) )

//
// FoobarPanelItemAction:
//
// The action to be executed when a panel item is clicked (if supported by the item).
//

G_STATIC_ASSERT( sizeof( FoobarPanelItemAction ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarPanelItemAction,
	foobar_panel_item_action,
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_ACTION_NONE, "none" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_ACTION_LAUNCHER, "launcher" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_ACTION_CONTROL_CENTER, "control-center" ) )

//
// FoobarPanelItemPosition:
//
// Position of a panel item in the panel.
//

G_STATIC_ASSERT( sizeof( FoobarPanelItemPosition ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarPanelItemPosition,
	foobar_panel_item_position,
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_POSITION_START, "start" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_POSITION_CENTER, "center" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_PANEL_ITEM_POSITION_END, "end" ) )

//
// FoobarControlCenterRow:
//
// A row in the "controls" section of the control center.
//

G_STATIC_ASSERT( sizeof( FoobarControlCenterRow ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarControlCenterRow,
	foobar_control_center_row,
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ROW_CONNECTIVITY, "connectivity" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ROW_AUDIO_OUTPUT, "audio-output" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ROW_AUDIO_INPUT, "audio-input" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ROW_BRIGHTNESS, "brightness" ) )

//
// FoobarControlCenterAlignment:
//
// Alignment of the control center along its screen edge.
//

G_STATIC_ASSERT( sizeof( FoobarControlCenterAlignment ) == sizeof( gint ) );
G_DEFINE_ENUM_TYPE(
	FoobarControlCenterAlignment,
	foobar_control_center_alignment,
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ALIGNMENT_START, "start" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ALIGNMENT_CENTER, "center" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ALIGNMENT_END, "end" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_CENTER_ALIGNMENT_FILL, "fill" ) )

//
// FoobarGeneralConfiguration:
//
// General application settings not specific to a single component.
//

struct _FoobarGeneralConfiguration
{
	gchar* stylesheet;
};

static void foobar_general_configuration_load ( FoobarGeneralConfiguration*       self,
                                                GKeyFile*                         file );
static void foobar_general_configuration_store( FoobarGeneralConfiguration const* self,
                                                GKeyFile*                         file );

G_DEFINE_BOXED_TYPE(
	FoobarGeneralConfiguration,
	foobar_general_configuration,
	foobar_general_configuration_copy,
	foobar_general_configuration_free )

//
// FoobarPanelItemConfiguration:
//
// Configuration for an item in the panel. Structures of this type have a specific item kind with more properties.
//

struct _FoobarPanelItemConfiguration
{
	FoobarPanelItemKind     kind;
	gchar*                  name;
	FoobarPanelItemPosition position;
	FoobarPanelItemAction   action;
	union
	{
		struct
		{
			gchar* icon_name;
		} icon;

		struct
		{
			gchar* format;
		} clock;

		struct
		{
			gint button_size;
			gint spacing;
		} workspaces;

		struct
		{
			FoobarStatusItem* items;
			gsize             items_count;
			gint              spacing;
			gboolean          show_labels;
			gboolean          enable_scrolling;
		} status;
	};
};

static FoobarPanelItemConfiguration* foobar_panel_item_configuration_load ( GKeyFile*                           file,
                                                                            gchar const*                        group );
static void                          foobar_panel_item_configuration_store( FoobarPanelItemConfiguration const* self,
                                                                            GKeyFile*                           file );

G_DEFINE_BOXED_TYPE(
	FoobarPanelItemConfiguration,
	foobar_panel_item_configuration,
	foobar_panel_item_configuration_copy,
	foobar_panel_item_configuration_free )

//
// FoobarPanelConfiguration:
//
// Configuration for the panel shown at the edge of a screen.
//

struct _FoobarPanelConfiguration
{
	FoobarScreenEdge               position;
	gint                           margin;
	gint                           padding;
	gint                           size;
	gint                           spacing;
	gboolean                       multi_monitor;
	FoobarPanelItemConfiguration** items;
	gsize                          items_count;
};

static void foobar_panel_configuration_load ( FoobarPanelConfiguration*       self,
                                              GKeyFile*                       file );
static void foobar_panel_configuration_store( FoobarPanelConfiguration const* self,
                                              GKeyFile*                       file );

G_DEFINE_BOXED_TYPE(
	FoobarPanelConfiguration,
	foobar_panel_configuration,
	foobar_panel_configuration_copy,
	foobar_panel_configuration_free )

//
// FoobarLauncherConfiguration:
//
// Configuration for the application launcher.
//

struct _FoobarLauncherConfiguration
{
	gint width;
	gint position;
	gint max_height;
};

static void foobar_launcher_configuration_load ( FoobarLauncherConfiguration*       self,
                                                 GKeyFile*                          file );
static void foobar_launcher_configuration_store( FoobarLauncherConfiguration const* self,
                                                 GKeyFile*                          file );

G_DEFINE_BOXED_TYPE(
	FoobarLauncherConfiguration,
	foobar_launcher_configuration,
	foobar_launcher_configuration_copy,
	foobar_launcher_configuration_free )

//
// FoobarControlCenterConfiguration:
//
// Configuration for the control center.
//

struct _FoobarControlCenterConfiguration
{
	gint                         width;
	gint                         height;
	FoobarScreenEdge             position;
	gint                         offset;
	gint                         padding;
	gint                         spacing;
	FoobarOrientation            orientation;
	FoobarControlCenterAlignment alignment;
	FoobarControlCenterRow*      rows;
	gsize                        rows_count;
};

static void foobar_control_center_configuration_load ( FoobarControlCenterConfiguration*       self,
                                                       GKeyFile*                               file );
static void foobar_control_center_configuration_store( FoobarControlCenterConfiguration const* self,
                                                       GKeyFile*                               file );

G_DEFINE_BOXED_TYPE(
	FoobarControlCenterConfiguration,
	foobar_control_center_configuration,
	foobar_control_center_configuration_copy,
	foobar_control_center_configuration_free )

//
// FoobarNotificationConfiguration:
//
// Configuration for notifications and the notification area shown in the corner of the screen.
//

struct _FoobarNotificationConfiguration
{
	gint   width;
	gint   min_height;
	gint   spacing;
	gint   close_button_inset;
	gchar* time_format;
};

static void foobar_notification_configuration_load ( FoobarNotificationConfiguration*       self,
                                                     GKeyFile*                              file );
static void foobar_notification_configuration_store( FoobarNotificationConfiguration const* self,
                                                     GKeyFile*                              file );

G_DEFINE_BOXED_TYPE(
	FoobarNotificationConfiguration,
	foobar_notification_configuration,
	foobar_notification_configuration_copy,
	foobar_notification_configuration_free )

//
// FoobarConfiguration:
//
// Main configuration structure containing the configuration state for all components of the application.
//

struct _FoobarConfiguration
{
	FoobarGeneralConfiguration*       general;
	FoobarPanelConfiguration*         panel;
	FoobarLauncherConfiguration*      launcher;
	FoobarControlCenterConfiguration* control_center;
	FoobarNotificationConfiguration*  notifications;
};

static void foobar_configuration_load          ( FoobarConfiguration*       self,
                                                 GKeyFile*                  file );
static void foobar_configuration_load_from_file( FoobarConfiguration*       self,
                                                 gchar const*               path );
static void foobar_configuration_store         ( FoobarConfiguration const* self, 
                                                 GKeyFile*                  file );

G_DEFINE_BOXED_TYPE( FoobarConfiguration, foobar_configuration, foobar_configuration_copy, foobar_configuration_free )

//
// FoobarConfigurationService:
//
// Service monitoring the configuration state of the application specified in its config file.
//

struct _FoobarConfigurationService
{
	GObject              parent_instance;
	gchar*               path;
	char*                actual_path;
	FoobarConfiguration* current;
	GFileMonitor*        monitor;
	GFileMonitor*        actual_monitor;
	gulong               changed_handler_id;
	gulong               actual_changed_handler_id;
	guint                update_source_id;
};

enum
{
	PROP_CURRENT = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void          foobar_configuration_service_class_init           ( FoobarConfigurationServiceClass* klass );
static void          foobar_configuration_service_init                 ( FoobarConfigurationService*      self );
static void          foobar_configuration_service_get_property         ( GObject*                         object,
                                                                         guint                            prop_id,
                                                                         GValue*                          value,
                                                                         GParamSpec*                      pspec );
static void          foobar_configuration_service_finalize             ( GObject*                         object );
static void          foobar_configuration_service_handle_changed       ( GFileMonitor*                    monitor,
                                                                         GFile*                           file,
                                                                         GFile*                           other_file,
                                                                         GFileMonitorEvent                event_type,
                                                                         gpointer                         userdata );
static void          foobar_configuration_service_handle_actual_changed( GFileMonitor*                    monitor,
                                                                         GFile*                           file,
                                                                         GFile*                           other_file,
                                                                         GFileMonitorEvent                event_type,
                                                                         gpointer                         userdata );
static void          foobar_configuration_service_update               ( FoobarConfigurationService*      self );
static gboolean      foobar_configuration_service_update_cb            ( gpointer                         userdata );
static GFileMonitor* foobar_configuration_service_create_monitor       ( gchar const*                     path );

G_DEFINE_FINAL_TYPE( FoobarConfigurationService, foobar_configuration_service, G_TYPE_OBJECT )

//
// Parsing/Serialization Helpers:
//
// These functions make it easier to parse, validate and serialize various data types.
//

typedef enum
{
	VALIDATE_NONE = 0,
	VALIDATE_NON_NEGATIVE = 1 << 0,
	VALIDATE_NON_ZERO = 1 << 1,
	VALIDATE_POSITIVE = VALIDATE_NON_NEGATIVE | VALIDATE_NON_ZERO,
	VALIDATE_FILE_URL = 1 << 2,
	VALIDATE_DISTINCT = 1 << 3,
} ValidationFlags;

static gboolean try_get_string_value     ( GKeyFile*       file,
                                           gchar const*    group,
                                           gchar const*    key,
                                           ValidationFlags validation,
                                           gchar**         out );
static gboolean try_get_string_list_value( GKeyFile*       file,
                                           gchar const*    group,
                                           gchar const*    key,
                                           ValidationFlags validation,
                                           GStrv*          out,
                                           gsize*          out_count );
static gboolean try_get_int_value        ( GKeyFile*       file,
                                           gchar const*    group,
                                           gchar const*    key,
                                           ValidationFlags validation,
                                           gint*           out );
static gboolean try_get_enum_value       ( GKeyFile*       file,
                                           gchar const*    group,
                                           gchar const*    key,
                                           GType           enum_type,
                                           ValidationFlags validation,
                                           gint*           out );
static gboolean try_get_enum_list_value  ( GKeyFile*       file,
                                           gchar const*    group,
                                           gchar const*    key,
                                           GType           enum_type,
                                           ValidationFlags validation,
                                           gpointer*       out,
                                           gsize*          out_count );
static gboolean try_get_boolean_value    ( GKeyFile*       file,
                                           gchar const*    group,
                                           gchar const*    key,
                                           ValidationFlags validation,
                                           gboolean*       out );

static gchar const*  enum_value_to_string ( GType         enum_type,
                                            gint          value );
static gchar const** enum_list_to_string  ( GType         enum_type,
                                            gconstpointer values,
                                            gsize         values_count );
static gchar*        enum_format_help_text( GType         enum_type );

// ---------------------------------------------------------------------------------------------------------------------
// Default Configuration
// ---------------------------------------------------------------------------------------------------------------------

static FoobarGeneralConfiguration default_general_configuration =
	{
		.stylesheet = "resource:///foobar/styles/default.css",
	};

static FoobarPanelItemConfiguration default_panel_item_icon_configuration =
	{
		.kind = FOOBAR_PANEL_ITEM_KIND_ICON,
		.name = "launcher",
		.position = FOOBAR_PANEL_ITEM_POSITION_START,
		.action = FOOBAR_PANEL_ITEM_ACTION_LAUNCHER,
		.icon.icon_name = "fluent-grid-dots-symbolic",
	};

static FoobarPanelItemConfiguration default_panel_item_clock_configuration =
	{
		.kind = FOOBAR_PANEL_ITEM_KIND_CLOCK,
		.name = "clock",
		.position = FOOBAR_PANEL_ITEM_POSITION_CENTER,
		.action = FOOBAR_PANEL_ITEM_ACTION_NONE,
		.clock.format = "%H\n%M",
	};

static FoobarPanelItemConfiguration default_panel_item_workspaces_configuration =
	{
		.kind = FOOBAR_PANEL_ITEM_KIND_WORKSPACES,
		.name = "workspaces",
		.position = FOOBAR_PANEL_ITEM_POSITION_START,
		.action = FOOBAR_PANEL_ITEM_ACTION_NONE,
		.workspaces.button_size = 20,
		.workspaces.spacing = 6,
	};

static FoobarStatusItem default_panel_item_status_configuration_items[] =
	{
		FOOBAR_STATUS_ITEM_BATTERY,
		FOOBAR_STATUS_ITEM_BRIGHTNESS,
		FOOBAR_STATUS_ITEM_AUDIO,
		FOOBAR_STATUS_ITEM_NETWORK,
		FOOBAR_STATUS_ITEM_BLUETOOTH,
		FOOBAR_STATUS_ITEM_NOTIFICATIONS
	};

static FoobarPanelItemConfiguration default_panel_item_status_configuration =
	{
		.kind = FOOBAR_PANEL_ITEM_KIND_STATUS,
		.name = "status",
		.position = FOOBAR_PANEL_ITEM_POSITION_END,
		.action = FOOBAR_PANEL_ITEM_ACTION_CONTROL_CENTER,
		.status.items = default_panel_item_status_configuration_items,
		.status.items_count = G_N_ELEMENTS( default_panel_item_status_configuration_items ),
		.status.spacing = 6,
		.status.show_labels = FALSE,
		.status.enable_scrolling = FALSE,
	};

static FoobarPanelItemConfiguration *default_panel_configuration_items[] =
	{
		&default_panel_item_icon_configuration,
		&default_panel_item_workspaces_configuration,
		&default_panel_item_clock_configuration,
		&default_panel_item_status_configuration,
		NULL,
	};

static FoobarPanelConfiguration default_panel_configuration =
	{
		.position = FOOBAR_SCREEN_EDGE_LEFT,
		.margin = 16,
		.padding = 12,
		.size = 48,
		.spacing = 12,
		.multi_monitor = TRUE,
		.items = default_panel_configuration_items,
		.items_count = G_N_ELEMENTS( default_panel_configuration_items ) - 1,
	};

static FoobarLauncherConfiguration default_launcher_configuration =
	{
		.width = 600,
		.position = 300,
		.max_height = 400,
	};

static FoobarControlCenterRow default_control_center_configuration_rows[] =
	{
		FOOBAR_CONTROL_CENTER_ROW_CONNECTIVITY,
		FOOBAR_CONTROL_CENTER_ROW_AUDIO_OUTPUT,
		FOOBAR_CONTROL_CENTER_ROW_BRIGHTNESS,
	};

static FoobarControlCenterConfiguration default_control_center_configuration =
	{
		.position = FOOBAR_SCREEN_EDGE_LEFT,
		.width = 400,
		.height = 600,
		.offset = 16,
		.padding = 24,
		.spacing = 12,
		.orientation = FOOBAR_ORIENTATION_VERTICAL,
		.alignment = FOOBAR_CONTROL_CENTER_ALIGNMENT_CENTER,
		.rows = default_control_center_configuration_rows,
		.rows_count = G_N_ELEMENTS( default_control_center_configuration_rows ),
	};

static FoobarNotificationConfiguration default_notification_configuration =
	{
		.width = 400,
		.min_height = 48,
		.spacing = 16,
		.close_button_inset = -6,
		.time_format = "%H:%M",
	};

static FoobarConfiguration default_configuration =
	{
		.general = &default_general_configuration,
		.panel = &default_panel_configuration,
		.launcher = &default_launcher_configuration,
		.control_center = &default_control_center_configuration,
		.notifications = &default_notification_configuration,
	};

// ---------------------------------------------------------------------------------------------------------------------
// Default Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default general configuration structure.
//
FoobarGeneralConfiguration* foobar_general_configuration_new( void )
{
	return foobar_general_configuration_copy( &default_general_configuration );
}

//
// Create a mutable copy of another general configuration structure.
//
FoobarGeneralConfiguration* foobar_general_configuration_copy( FoobarGeneralConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarGeneralConfiguration* copy = g_new0( FoobarGeneralConfiguration, 1 );
	copy->stylesheet = g_strdup( self->stylesheet );
	return copy;
}

//
// Release resources associated with a general configuration structure.
//
void foobar_general_configuration_free( FoobarGeneralConfiguration* self )
{
	g_return_if_fail( self != NULL );

	g_free( self->stylesheet );
	g_free( self );
}

//
// Check if two general configuration structures are equal.
//
gboolean foobar_general_configuration_equal(
	FoobarGeneralConfiguration const* a,
	FoobarGeneralConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( g_strcmp0( a->stylesheet, b->stylesheet ) ) { return FALSE; }

	return TRUE;
}

//
// URI to the CSS stylesheet to use -- this may be a resource that's bundled with Foobar or a "file:" URI.
//
gchar const* foobar_general_configuration_get_stylesheet( FoobarGeneralConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->stylesheet;
}

//
// URI to the CSS stylesheet to use -- this may be a resource that's bundled with Foobar or a "file:" URI.
//
void foobar_general_configuration_set_stylesheet(
	FoobarGeneralConfiguration* self,
	gchar const*                value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( value != NULL );

	g_free( self->stylesheet );
	self->stylesheet = g_strdup( value );
}

//
// Populate a general configuration structure from the "general" section of a keyfile.
//
void foobar_general_configuration_load(
	FoobarGeneralConfiguration* self,
	GKeyFile*                   file )
{
	g_autofree gchar* stylesheet = NULL;
	if ( try_get_string_value( file, "general", "stylesheet", VALIDATE_FILE_URL, &stylesheet ) )
	{
		foobar_general_configuration_set_stylesheet( self, stylesheet );
	}
}

//
// Store a general configuration structure in the "general" section of a keyfile.
//
void foobar_general_configuration_store(
	FoobarGeneralConfiguration const* self,
	GKeyFile*                         file )
{
	gchar const* stylesheet = foobar_general_configuration_get_stylesheet( self );
	g_key_file_set_string( file, "general", "stylesheet", stylesheet );
	g_key_file_set_comment(
		file,
		"general",
		"stylesheet",
		" URI to the CSS stylesheet to use -- this may be a resource that's bundled with Foobar or a \"file:\" URI.",
		NULL );
}

// ---------------------------------------------------------------------------------------------------------------------
// Panel Item Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default icon panel item configuration structure.
//
FoobarPanelItemConfiguration* foobar_panel_item_icon_configuration_new( void )
{
	return foobar_panel_item_configuration_copy( &default_panel_item_icon_configuration );
}

//
// Create a default clock panel item configuration structure.
//
FoobarPanelItemConfiguration* foobar_panel_item_clock_configuration_new( void )
{
	return foobar_panel_item_configuration_copy( &default_panel_item_clock_configuration );
}

//
// Create a default workspaces panel item configuration structure.
//
FoobarPanelItemConfiguration* foobar_panel_item_workspaces_configuration_new( void )
{
	return foobar_panel_item_configuration_copy( &default_panel_item_workspaces_configuration );
}

//
// Create a default status panel item configuration structure.
//
FoobarPanelItemConfiguration* foobar_panel_item_status_configuration_new( void )
{
	return foobar_panel_item_configuration_copy( &default_panel_item_status_configuration );
}

//
// Create a mutable copy of another panel item configuration structure, preserving its item type.
//
FoobarPanelItemConfiguration* foobar_panel_item_configuration_copy( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarPanelItemConfiguration* copy = g_new0( FoobarPanelItemConfiguration, 1 );
	copy->kind = self->kind;
	copy->name = g_strdup( self->name );
	copy->position = self->position;
	copy->action = self->action;
	switch ( copy->kind )
	{
		case FOOBAR_PANEL_ITEM_KIND_ICON:
			copy->icon.icon_name = g_strdup( self->icon.icon_name );
			break;
		case FOOBAR_PANEL_ITEM_KIND_CLOCK:
			copy->clock.format = g_strdup( self->clock.format );
			break;
		case FOOBAR_PANEL_ITEM_KIND_WORKSPACES:
			copy->workspaces.button_size = self->workspaces.button_size;
			copy->workspaces.spacing = self->workspaces.spacing;
			break;
		case FOOBAR_PANEL_ITEM_KIND_STATUS:
			copy->status.items = g_new( FoobarStatusItem, self->status.items_count );
			copy->status.items_count = self->status.items_count;
			memcpy( copy->status.items, self->status.items, sizeof( *copy->status.items ) * copy->status.items_count );
			copy->status.spacing = self->status.spacing;
			copy->status.show_labels = self->status.show_labels;
			copy->status.enable_scrolling = self->status.enable_scrolling;
			break;
		default:
			g_warn_if_reached( );
	}
	return copy;
}

//
// Release resources associated with a panel item configuration structure.
//
void foobar_panel_item_configuration_free( FoobarPanelItemConfiguration* self )
{
	g_return_if_fail( self != NULL );

	switch ( self->kind )
	{
		case FOOBAR_PANEL_ITEM_KIND_ICON:
			g_free( self->icon.icon_name );
			break;
		case FOOBAR_PANEL_ITEM_KIND_CLOCK:
			g_free( self->clock.format );
			break;
		case FOOBAR_PANEL_ITEM_KIND_WORKSPACES:
			break;
		case FOOBAR_PANEL_ITEM_KIND_STATUS:
			g_free( self->status.items );
			break;
		default:
			g_warn_if_reached( );
			break;
	}

	g_free( self->name );
	g_free( self );
}

//
// Check if two panel item configuration structures are equal.
//
gboolean foobar_panel_item_configuration_equal(
	FoobarPanelItemConfiguration const* a,
	FoobarPanelItemConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( a->kind != b->kind ) { return FALSE; }
	if ( a->position != b->position ) { return FALSE; }
	if ( a->action != b->action ) { return FALSE; }
	if ( g_strcmp0( a->name, b->name ) ) { return FALSE; }

	switch ( a->kind )
	{
		case FOOBAR_PANEL_ITEM_KIND_ICON:
			if ( g_strcmp0( a->icon.icon_name, b->icon.icon_name ) ) { return FALSE; }
			break;
		case FOOBAR_PANEL_ITEM_KIND_CLOCK:
			if ( g_strcmp0( a->clock.format, b->clock.format ) ) { return FALSE; }
			break;
		case FOOBAR_PANEL_ITEM_KIND_WORKSPACES:
			if ( a->workspaces.button_size != b->workspaces.button_size ) { return FALSE; }
			if ( a->workspaces.spacing != b->workspaces.spacing ) { return FALSE; }
			break;
		case FOOBAR_PANEL_ITEM_KIND_STATUS:
			if ( a->status.items_count != b->status.items_count ) { return FALSE; }
			if ( memcmp( a->status.items, b->status.items, sizeof( *a->status.items ) * a->status.items_count ) ) { return FALSE; }
			if ( a->status.spacing != b->status.spacing ) { return FALSE; }
			if ( a->status.show_labels != b->status.show_labels ) { return FALSE; }
			if ( a->status.enable_scrolling != b->status.enable_scrolling ) { return FALSE; }
			break;
		default:
			g_warn_if_reached( );
			break;
	}

	return TRUE;
}

//
// The type of panel item to be configured by this structure.
//
FoobarPanelItemKind foobar_panel_item_configuration_get_kind( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->kind;
}

//
// Name of the panel item (this is derived from the section name, which has the form "panel.[name]").
//
gchar const* foobar_panel_item_configuration_get_name( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->name;
}

//
// Position where the item should be placed item within the panel.
//
FoobarPanelItemPosition foobar_panel_item_configuration_get_position( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->position;
}

//
// Action invoked when the user clicks the item (if supported by the item).
//
FoobarPanelItemAction foobar_panel_item_configuration_get_action( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->action;
}

//
// The GTK icon to use for the item.
//
gchar const* foobar_panel_item_icon_configuration_get_icon_name( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_ICON, NULL );

	return self->icon.icon_name;
}

//
// The time format string as used by g_date_time_format.
//
gchar const* foobar_panel_item_clock_configuration_get_format( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_CLOCK, NULL );

	return self->clock.format;
}

//
// Size of each workspace button.
//
gint foobar_panel_item_workspaces_configuration_get_button_size( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_WORKSPACES, 0 );

	return self->workspaces.button_size;
}

//
// Inner spacing between the workspace buttons.
//
gint foobar_panel_item_workspaces_configuration_get_spacing( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_WORKSPACES, 0 );

	return self->workspaces.spacing;
}

//
// Ordered list of status items to display in the item.
//
FoobarStatusItem const* foobar_panel_item_status_configuration_get_items(
	FoobarPanelItemConfiguration const* self,
	gsize*                              out_count )
{
	g_return_val_if_fail( self != NULL, NULL );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS, NULL );
	g_return_val_if_fail( out_count != NULL, NULL );

	*out_count = self->status.items_count;
	return self->status.items;
}

//
// Inner spacing between the status items.
//
gint foobar_panel_item_status_configuration_get_spacing( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS, 0 );

	return self->status.spacing;
}

//
// Indicates whether text labels should be shown next to the status icons.
//
gboolean foobar_panel_item_status_configuration_get_show_labels( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, FALSE );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS, FALSE );

	return self->status.show_labels;
}

//
// If set to true, some settings like volume or brightness can be adjusted by scrolling while hovering over the status
// item.
//
gboolean foobar_panel_item_status_configuration_get_enable_scrolling( FoobarPanelItemConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, FALSE );
	g_return_val_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS, FALSE );

	return self->status.enable_scrolling;
}

//
// Name of the panel item (this is derived from the section name, which has the form "panel.[name]").
//
void foobar_panel_item_configuration_set_name(
	FoobarPanelItemConfiguration* self,
	gchar const*                  value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( value != NULL );

	g_free( self->name );
	self->name = g_strdup( value );
}

//
// Position where the item should be placed item within the panel.
//
void foobar_panel_item_configuration_set_position(
	FoobarPanelItemConfiguration* self,
	FoobarPanelItemPosition       value )
{
	g_return_if_fail( self != NULL );
	self->position = value;
}

//
// Action invoked when the user clicks the item (if supported by the item).
//
void foobar_panel_item_configuration_set_action(
	FoobarPanelItemConfiguration* self,
	FoobarPanelItemAction         value )
{
	g_return_if_fail( self != NULL );
	self->action = value;
}

//
// The GTK icon to use for the item.
//
void foobar_panel_item_icon_configuration_set_icon_name(
	FoobarPanelItemConfiguration* self,
	gchar const*                  value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_ICON );
	g_return_if_fail( value != NULL );

	g_free( self->icon.icon_name );
	self->icon.icon_name = g_strdup( value );
}

//
// The time format string as used by g_date_time_format.
//
void foobar_panel_item_clock_configuration_set_format(
	FoobarPanelItemConfiguration* self,
	gchar const*                  value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_CLOCK );
	g_return_if_fail( value != NULL );

	g_free( self->clock.format );
	self->clock.format = g_strdup( value );
}

//
// Size of each workspace button.
//
void foobar_panel_item_workspaces_configuration_set_button_size(
	FoobarPanelItemConfiguration* self,
	gint                          value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_WORKSPACES );

	self->workspaces.button_size = value;
}

//
// Inner spacing between the workspace buttons.
//
void foobar_panel_item_workspaces_configuration_set_spacing(
	FoobarPanelItemConfiguration* self,
	gint                          value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_WORKSPACES );

	self->workspaces.spacing = value;
}

//
// Ordered list of status items to display in the item.
//
void foobar_panel_item_status_configuration_set_items(
	FoobarPanelItemConfiguration* self,
	FoobarStatusItem const*       value,
	gsize                         value_count )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS );
	g_return_if_fail( value != NULL || value_count == 0 );

	g_free( self->status.items );
	self->status.items = g_new( FoobarStatusItem, value_count );
	self->status.items_count = value_count;
	memcpy( self->status.items, value, sizeof( *self->status.items ) * self->status.items_count );
}

//
// Inner spacing between the status items.
//
void foobar_panel_item_status_configuration_set_spacing(
	FoobarPanelItemConfiguration* self,
	gint                          value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS );

	self->status.spacing = value;
}

//
// Indicates whether text labels should be shown next to the status icons.
//
void foobar_panel_item_status_configuration_set_show_labels(
	FoobarPanelItemConfiguration* self,
	gboolean                      value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS );

	self->status.show_labels = value;
}

//
// If set to true, some settings like volume or brightness can be adjusted by scrolling while hovering over the status
// item.
//
void foobar_panel_item_status_configuration_set_enable_scrolling(
	FoobarPanelItemConfiguration* self,
	gboolean                      value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( self->kind == FOOBAR_PANEL_ITEM_KIND_STATUS );

	self->status.enable_scrolling = value;
}

//
// Create a panel item configuration structure from the given section of a keyfile, returning NULL if it is not a panel
// item configuration section.
//
FoobarPanelItemConfiguration* foobar_panel_item_configuration_load(
	GKeyFile*    file,
	gchar const* group )
{
	if ( !g_str_has_prefix( group, "panel." ) )
	{
		return NULL;
	}

	gint kind;
	if ( !try_get_enum_value( file, group, "kind", FOOBAR_TYPE_PANEL_ITEM_KIND, VALIDATE_NONE, &kind ) )
	{
		return NULL;
	}

	FoobarPanelItemConfiguration* result;
	switch ( (FoobarPanelItemKind)kind )
	{
		case FOOBAR_PANEL_ITEM_KIND_ICON:
		{
			result = foobar_panel_item_icon_configuration_new( );

			g_autofree gchar* icon_name = NULL;
			if ( try_get_string_value( file, group, "icon-name", VALIDATE_NONE, &icon_name ) )
			{
				foobar_panel_item_icon_configuration_set_icon_name( result, icon_name );
			}

			break;
		}
		case FOOBAR_PANEL_ITEM_KIND_CLOCK:
		{
			result = foobar_panel_item_clock_configuration_new( );

			g_autofree gchar* format = NULL;
			if ( try_get_string_value( file, group, "format", VALIDATE_NONE, &format ) )
			{
				foobar_panel_item_clock_configuration_set_format( result, format );
			}

			break;
		}
		case FOOBAR_PANEL_ITEM_KIND_WORKSPACES:
		{
			result = foobar_panel_item_workspaces_configuration_new( );

			gint button_size;
			if ( try_get_int_value( file, group, "button-size", VALIDATE_POSITIVE, &button_size ) )
			{
				foobar_panel_item_workspaces_configuration_set_button_size( result, button_size );
			}

			gint spacing;
			if ( try_get_int_value( file, group, "spacing", VALIDATE_NON_NEGATIVE, &spacing ) )
			{
				foobar_panel_item_workspaces_configuration_set_spacing( result, spacing );
			}

			break;
		}
		case FOOBAR_PANEL_ITEM_KIND_STATUS:
		{
			result = foobar_panel_item_status_configuration_new( );

			g_autofree gpointer items = NULL;
			gsize items_count;
			if ( try_get_enum_list_value( file, group, "items", FOOBAR_TYPE_STATUS_ITEM, VALIDATE_DISTINCT, &items, &items_count ) )
			{
				foobar_panel_item_status_configuration_set_items( result, items, items_count );
			}

			gint spacing;
			if ( try_get_int_value( file, group, "spacing", VALIDATE_NON_NEGATIVE, &spacing ) )
			{
				foobar_panel_item_status_configuration_set_spacing( result, spacing );
			}

			gboolean show_labels;
			if ( try_get_boolean_value( file, group, "show-labels", VALIDATE_NONE, &show_labels ) )
			{
				foobar_panel_item_status_configuration_set_show_labels( result, show_labels );
			}

			gboolean enable_scrolling;
			if ( try_get_boolean_value( file, group, "enable-scrolling", VALIDATE_NONE, &enable_scrolling ) )
			{
				foobar_panel_item_status_configuration_set_enable_scrolling( result, enable_scrolling );
			}

			break;
		}
		default:
		{
			g_warn_if_reached( );
			return NULL;
		}
	}

	gchar const* name = group + strlen( "panel." );
	foobar_panel_item_configuration_set_name( result, name );

	gint position;
	if ( try_get_enum_value( file, group, "position", FOOBAR_TYPE_PANEL_ITEM_POSITION, VALIDATE_NONE, &position ) )
	{
		foobar_panel_item_configuration_set_position( result, position );
	}

	gint action;
	if ( try_get_enum_value( file, group, "action", FOOBAR_TYPE_PANEL_ITEM_ACTION, VALIDATE_NONE, &action ) )
	{
		foobar_panel_item_configuration_set_action( result, action );
	}

	return result;
}

//
// Store a panel item configuration structure in the "panel.[name]" section of a keyfile.
//
void foobar_panel_item_configuration_store(
	FoobarPanelItemConfiguration const* self,
	GKeyFile* file )
{
	gchar const* name = foobar_panel_item_configuration_get_name( self );
	g_autofree gchar* group = g_strdup_printf( "panel.%s", name );

	FoobarPanelItemKind kind = foobar_panel_item_configuration_get_kind( self );
	g_key_file_set_string( file, group, "kind", enum_value_to_string( FOOBAR_TYPE_PANEL_ITEM_KIND, kind ) );

	FoobarPanelItemPosition position = foobar_panel_item_configuration_get_position( self );
	g_key_file_set_string( file, group, "position", enum_value_to_string( FOOBAR_TYPE_PANEL_ITEM_POSITION, position ) );

	FoobarPanelItemAction action = foobar_panel_item_configuration_get_action( self );
	g_key_file_set_string( file, group, "action", enum_value_to_string( FOOBAR_TYPE_PANEL_ITEM_ACTION, action ) );

	switch ( kind )
	{
		case FOOBAR_PANEL_ITEM_KIND_ICON:
		{
			gchar const* icon_name = foobar_panel_item_icon_configuration_get_icon_name( self );
			g_key_file_set_string( file, group, "icon-name", icon_name );
			g_key_file_set_comment(
				file,
				group,
				"icon-name",
				" The GTK icon to use for the item.",
				NULL );

			break;
		}
		case FOOBAR_PANEL_ITEM_KIND_CLOCK:
		{
			gchar const* format = foobar_panel_item_clock_configuration_get_format( self );
			g_key_file_set_string( file, group, "format", format );
			g_key_file_set_comment(
				file,
				group,
				"format",
				" The time format string as used by g_date_time_format.",
				NULL );

			break;
		}
		case FOOBAR_PANEL_ITEM_KIND_WORKSPACES:
		{
			gint button_size = foobar_panel_item_workspaces_configuration_get_button_size( self );
			g_key_file_set_integer( file, group, "button-size", button_size );
			g_key_file_set_comment(
				file,
				group,
				"button-size",
				" Size of each workspace button.",
				NULL );

			gint spacing = foobar_panel_item_workspaces_configuration_get_spacing( self );
			g_key_file_set_integer( file, group, "spacing", spacing );
			g_key_file_set_comment(
				file,
				group,
				"spacing",
				" Inner spacing between the workspace buttons.",
				NULL );

			break;
		}
		case FOOBAR_PANEL_ITEM_KIND_STATUS:
		{
			gsize items_count;
			FoobarStatusItem const* items = foobar_panel_item_status_configuration_get_items( self, &items_count );
			g_autofree gchar const** items_str = enum_list_to_string( FOOBAR_TYPE_STATUS_ITEM, items, items_count );
			g_key_file_set_string_list( file, group, "items", items_str, items_count );
			g_key_file_set_comment(
				file,
				group,
				"items",
				" Ordered list of status items to display in the item.",
				NULL );

			gint spacing = foobar_panel_item_status_configuration_get_spacing( self );
			g_key_file_set_integer( file, group, "spacing", spacing );
			g_key_file_set_comment(
				file,
				group,
				"spacing",
				" Inner spacing between the status items.",
				NULL );

			gboolean show_labels = foobar_panel_item_status_configuration_get_show_labels( self );
			g_key_file_set_boolean( file, group, "show-labels", show_labels );
			g_key_file_set_comment(
				file,
				group,
				"show-labels",
				" Indicates whether text labels should be shown next to the status icons.",
				NULL );

			gboolean enable_scrolling = foobar_panel_item_status_configuration_get_enable_scrolling( self );
			g_key_file_set_boolean( file, group, "enable-scrolling", enable_scrolling );
			g_key_file_set_comment(
				file,
				group,
				"enable-scrolling",
				" If set to true, some settings like volume or brightness can be adjusted by scrolling while hovering"
				" over the status item.",
				NULL );

			break;
		}
		default:
		{
			g_warn_if_reached( );
			break;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Panel Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default panel configuration structure.
//
FoobarPanelConfiguration* foobar_panel_configuration_new( void )
{
	return foobar_panel_configuration_copy( &default_panel_configuration );
}

//
// Create a mutable copy of another panel configuration structure.
//
FoobarPanelConfiguration* foobar_panel_configuration_copy( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarPanelConfiguration* copy = g_new0( FoobarPanelConfiguration, 1 );
	copy->position = self->position;
	copy->margin = self->margin;
	copy->padding = self->padding;
	copy->size = self->size;
	copy->spacing = self->spacing;
	copy->multi_monitor = self->multi_monitor;
	copy->items = g_new( FoobarPanelItemConfiguration*, self->items_count + 1 );
	copy->items_count = self->items_count;
	for ( gsize i = 0; i < copy->items_count; ++i )
	{
		copy->items[i] = foobar_panel_item_configuration_copy( self->items[i] );
	}
	return copy;
}

//
// Release resources associated with a panel configuration structure.
//
void foobar_panel_configuration_free( FoobarPanelConfiguration* self )
{
	g_return_if_fail( self != NULL );

	for ( gsize i = 0; i < self->items_count; ++i )
	{
		foobar_panel_item_configuration_free( self->items[i] );
	}
	g_free( self->items );
	g_free( self );
}

//
// Check if two panel configuration structures are equal.
//
gboolean foobar_panel_configuration_equal(
	FoobarPanelConfiguration const* a,
	FoobarPanelConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( a->position != b->position ) { return FALSE; }
	if ( a->margin != b->margin ) { return FALSE; }
	if ( a->padding != b->padding ) { return FALSE; }
	if ( a->size != b->size ) { return FALSE; }
	if ( a->spacing != b->spacing ) { return FALSE; }
	if ( a->multi_monitor != b->multi_monitor ) { return FALSE; }
	if ( a->items_count != b->items_count ) { return FALSE; }

	for ( gsize i = 0; i < a->items_count; ++i )
	{
		if ( !foobar_panel_item_configuration_equal( a->items[i], b->items[i] ) ) { return FALSE; }
	}

	return TRUE;
}

//
// The screen edge which the panel should appear on.
//
FoobarScreenEdge foobar_panel_configuration_get_position( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->position;
}

//
// Offset of the panel from all screen edges.
//
gint foobar_panel_configuration_get_margin( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->margin;
}

//
// Inset of the panel along its orientation.
//
gint foobar_panel_configuration_get_padding( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->padding;
}

//
// Size of the panel (depending on the orientation, this can be either the width or the height).
//
gint foobar_panel_configuration_get_size( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->size;
}

//
// Spacing between panel items.
//
gint foobar_panel_configuration_get_spacing( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->spacing;
}

//
// Flag to enable the panel on all monitors.
//
gboolean foobar_panel_configuration_get_multi_monitor( FoobarPanelConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, FALSE );
	return self->multi_monitor;
}

//
// Items configured to be displayed in the panel.
//
FoobarPanelItemConfiguration const* const* foobar_panel_configuration_get_items(
	FoobarPanelConfiguration const* self,
	gsize*                          out_count )
{
	g_return_val_if_fail( self != NULL, NULL );

	if ( out_count ) { *out_count = self->items_count; }
	return (FoobarPanelItemConfiguration const* const*)self->items;
}

//
// Mutable items configured to be displayed in the panel.
//
FoobarPanelItemConfiguration* const* foobar_panel_configuration_get_items_mut(
	FoobarPanelConfiguration* self,
	gsize*                    out_count )
{
	g_return_val_if_fail( self != NULL, NULL );

	if ( out_count ) { *out_count = self->items_count; }
	return self->items;
}

//
// The screen edge which the panel should appear on.
//
void foobar_panel_configuration_set_position(
	FoobarPanelConfiguration* self,
	FoobarScreenEdge          value )
{
	g_return_if_fail( self != NULL );
	self->position = value;
}

//
// Offset of the panel from all screen edges.
//
void foobar_panel_configuration_set_margin(
	FoobarPanelConfiguration* self,
	gint                      value )
{
	g_return_if_fail( self != NULL );
	self->margin = value;
}

//
// Inset of the panel along its orientation.
//
void foobar_panel_configuration_set_padding(
	FoobarPanelConfiguration* self,
	gint                      value )
{
	g_return_if_fail( self != NULL );
	self->padding = value;
}

//
// Size of the panel (depending on the orientation, this can be either the width or the height).
//
void foobar_panel_configuration_set_size(
	FoobarPanelConfiguration* self,
	gint                      value )
{
	g_return_if_fail( self != NULL );
	self->size = value;
}

//
// Spacing between panel items.
//
void foobar_panel_configuration_set_spacing(
	FoobarPanelConfiguration* self,
	gint                      value )
{
	g_return_if_fail( self != NULL );
	self->spacing = value;
}

//
// Flag to enable the panel on all monitors.
//
void foobar_panel_configuration_set_multi_monitor(
	FoobarPanelConfiguration* self,
	gboolean                  value )
{
	g_return_if_fail( self != NULL );
	self->multi_monitor = value;
}

//
// Items configured to be displayed in the panel.
//
void foobar_panel_configuration_set_items(
	FoobarPanelConfiguration*                  self,
	FoobarPanelItemConfiguration const* const* value,
	gsize                                      value_count )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( value != NULL || value_count == 0 );

	for ( gsize i = 0; i < self->items_count; ++i )
	{
		foobar_panel_item_configuration_free( self->items[i] );
	}
	g_free( self->items );
	self->items = g_new0( FoobarPanelItemConfiguration*, value_count + 1 );
	self->items_count = 0;
	for ( gsize i = 0; i < value_count; ++i )
	{
		g_warn_if_fail( value[i] != NULL );
		if ( value[i] )
		{
			self->items[self->items_count++] = foobar_panel_item_configuration_copy( value[i] );
		}
	}
}

//
// Populate a panel configuration structure from the "panel" section of a keyfile.
//
void foobar_panel_configuration_load(
	FoobarPanelConfiguration* self,
	GKeyFile*                 file )
{
	gint position;
	if ( try_get_enum_value( file, "panel", "position", FOOBAR_TYPE_SCREEN_EDGE, VALIDATE_NONE, &position ) )
	{
		foobar_panel_configuration_set_position( self, position );
	}

	gint margin;
	if ( try_get_int_value( file, "panel", "margin", VALIDATE_NON_NEGATIVE, &margin ) )
	{
		foobar_panel_configuration_set_margin( self, margin );
	}

	gint padding;
	if ( try_get_int_value( file, "panel", "padding", VALIDATE_NON_NEGATIVE, &padding ) )
	{
		foobar_panel_configuration_set_padding( self, padding );
	}

	gint size;
	if ( try_get_int_value( file, "panel", "size", VALIDATE_POSITIVE, &size ) )
	{
		foobar_panel_configuration_set_size( self, size );
	}

	gint spacing;
	if ( try_get_int_value( file, "panel", "spacing", VALIDATE_NON_NEGATIVE, &spacing ) )
	{
		foobar_panel_configuration_set_spacing( self, spacing );
	}

	gboolean multi_monitor;
	if ( try_get_boolean_value( file, "panel", "multi-monitor", VALIDATE_NONE, &multi_monitor ) )
	{
		foobar_panel_configuration_set_multi_monitor( self, multi_monitor );
	}

	g_autoptr( GPtrArray ) items = g_ptr_array_new_with_free_func( (GDestroyNotify)foobar_panel_item_configuration_free );
	g_auto( GStrv ) groups = g_key_file_get_groups( file, NULL );
	for ( gchar** it = groups; *it; ++it )
	{
		FoobarPanelItemConfiguration* item = foobar_panel_item_configuration_load( file, *it );
		if ( item ) { g_ptr_array_add( items, item ); }
	}
	foobar_panel_configuration_set_items( self, (FoobarPanelItemConfiguration const* const*)items->pdata, items->len );
}

//
// Store a panel configuration structure in the "panel" section of a keyfile.
//
void foobar_panel_configuration_store(
	FoobarPanelConfiguration const* self,
	GKeyFile*                       file )
{
	FoobarScreenEdge position = foobar_panel_configuration_get_position( self );
	g_key_file_set_string( file, "panel", "position", enum_value_to_string( FOOBAR_TYPE_SCREEN_EDGE, position ) );
	g_key_file_set_comment(
		file,
		"panel",
		"position",
		" The screen edge which the panel should appear on.",
		NULL );

	gint margin = foobar_panel_configuration_get_margin( self );
	g_key_file_set_integer( file, "panel", "margin", margin );
	g_key_file_set_comment(
		file,
		"panel",
		"margin",
		" Offset of the panel from all screen edges.",
		NULL );

	gint padding = foobar_panel_configuration_get_padding( self );
	g_key_file_set_integer( file, "panel", "padding", padding );
	g_key_file_set_comment(
		file,
		"panel",
		"padding",
		" Inset of the panel along its orientation.",
		NULL );

	gint size = foobar_panel_configuration_get_size( self );
	g_key_file_set_integer( file, "panel", "size", size );
	g_key_file_set_comment(
		file,
		"panel",
		"size",
		" Size of the panel (depending on the orientation, this can be either the width or the height).",
		NULL );

	gint spacing = foobar_panel_configuration_get_spacing( self );
	g_key_file_set_integer( file, "panel", "spacing", spacing );
	g_key_file_set_comment(
		file,
		"panel",
		"spacing",
		" Spacing between panel items.",
		NULL );

	gboolean multi_monitor = foobar_panel_configuration_get_multi_monitor( self );
	g_key_file_set_boolean( file, "panel", "multi-monitor", multi_monitor );
	g_key_file_set_comment(
		file,
		"panel",
		"multi-monitor",
		" Flag to enable the panel on all monitors.",
		NULL );

	gsize items_count;
	FoobarPanelItemConfiguration const* const* items = foobar_panel_configuration_get_items( self, &items_count );
	for ( gsize i = 0; i < items_count; ++i )
	{
		foobar_panel_item_configuration_store( items[i], file );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Launcher Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default launcher configuration structure.
//
FoobarLauncherConfiguration* foobar_launcher_configuration_new( void )
{
	return foobar_launcher_configuration_copy( &default_launcher_configuration );
}

//
// Create a mutable copy of another launcher configuration structure.
//
FoobarLauncherConfiguration* foobar_launcher_configuration_copy( FoobarLauncherConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarLauncherConfiguration* copy = g_new0( FoobarLauncherConfiguration, 1 );
	copy->width = self->width;
	copy->position = self->position;
	copy->max_height = self->max_height;
	return copy;
}

//
// Release resources associated with a launcher configuration structure.
//
void foobar_launcher_configuration_free( FoobarLauncherConfiguration* self )
{
	g_return_if_fail( self != NULL );

	g_free( self );
}

//
// Check if two launcher configuration structures are equal.
//
gboolean foobar_launcher_configuration_equal(
	FoobarLauncherConfiguration const* a,
	FoobarLauncherConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( a->width != b->width ) { return FALSE; }
	if ( a->position != b->position ) { return FALSE; }
	if ( a->max_height != b->max_height ) { return FALSE; }

	return TRUE;
}

//
// Horizontal size of the launcher.
//
gint foobar_launcher_configuration_get_width( FoobarLauncherConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->width;
}

//
// Offset from the top of the screen.
//
gint foobar_launcher_configuration_get_position( FoobarLauncherConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->position;
}

//
// Maximum allowed height for the launcher before scrolling is enabled.
//
gint foobar_launcher_configuration_get_max_height( FoobarLauncherConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->max_height;
}

//
// Horizontal size of the launcher.
//
void foobar_launcher_configuration_set_width(
	FoobarLauncherConfiguration* self,
	gint                         value )
{
	g_return_if_fail( self != NULL );
	self->width = value;
}

//
// Offset from the top of the screen.
//
void foobar_launcher_configuration_set_position(
	FoobarLauncherConfiguration* self,
	gint                         value )
{
	g_return_if_fail( self != NULL );
	self->position = value;
}

//
// Maximum allowed height for the launcher before scrolling is enabled.
//
void foobar_launcher_configuration_set_max_height(
	FoobarLauncherConfiguration* self,
	gint                         value )
{
	g_return_if_fail( self != NULL );
	self->max_height = value;
}

//
// Populate a launcher configuration structure from the "launcher" section of a keyfile.
//
void foobar_launcher_configuration_load(
	FoobarLauncherConfiguration* self,
	GKeyFile*                    file )
{
	gint width;
	if ( try_get_int_value( file, "launcher", "width", VALIDATE_POSITIVE, &width ) )
	{
		foobar_launcher_configuration_set_width( self, width );
	}

	gint position;
	if ( try_get_int_value( file, "launcher", "position", VALIDATE_NON_NEGATIVE, &position ) )
	{
		foobar_launcher_configuration_set_position( self, position );
	}

	gint max_height;
	if ( try_get_int_value( file, "launcher", "max-height", VALIDATE_POSITIVE, &max_height ) )
	{
		foobar_launcher_configuration_set_position( self, max_height );
	}
}

//
// Store a launcher configuration structure in the "launcher" section of a keyfile.
//
void foobar_launcher_configuration_store(
	FoobarLauncherConfiguration const* self,
	GKeyFile*                          file )
{
	gint width = foobar_launcher_configuration_get_width( self );
	g_key_file_set_integer( file, "launcher", "width", width );

	gint position = foobar_launcher_configuration_get_position( self );
	g_key_file_set_integer( file, "launcher", "position", position );
	g_key_file_set_comment(
		file,
		"launcher",
		"position",
		" Offset from the top of the screen.",
		NULL );

	gint max_height = foobar_launcher_configuration_get_max_height( self );
	g_key_file_set_integer( file, "launcher", "max-height", max_height );
	g_key_file_set_comment(
		file,
		"launcher",
		"max-height",
		" Maximum allowed height for the launcher before scrolling is enabled.",
		NULL );
}

// ---------------------------------------------------------------------------------------------------------------------
// Control Center Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default control center configuration structure.
//
FoobarControlCenterConfiguration* foobar_control_center_configuration_new( void )
{
	return foobar_control_center_configuration_copy( &default_control_center_configuration );
}

//
// Create a mutable copy of another control center configuration structure.
//
FoobarControlCenterConfiguration* foobar_control_center_configuration_copy( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarControlCenterConfiguration* copy = g_new0( FoobarControlCenterConfiguration, 1 );
	copy->width = self->width;
	copy->height = self->height;
	copy->position = self->position;
	copy->offset = self->offset;
	copy->padding = self->padding;
	copy->spacing = self->spacing;
	copy->orientation = self->orientation;
	copy->alignment = self->alignment;
	copy->rows = g_new( FoobarControlCenterRow, self->rows_count );
	copy->rows_count = self->rows_count;
	memcpy( copy->rows, self->rows, sizeof( *copy->rows ) * copy->rows_count );
	return copy;
}

//
// Release resources associated with a control center configuration structure.
//
void foobar_control_center_configuration_free( FoobarControlCenterConfiguration* self )
{
	g_return_if_fail( self != NULL );

	g_free( self->rows );
	g_free( self );
}

//
// Check if two control center configuration structures are equal.
//
gboolean foobar_control_center_configuration_equal(
	FoobarControlCenterConfiguration const* a,
	FoobarControlCenterConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( a->width != b->width ) { return FALSE; }
	if ( a->height != b->height ) { return FALSE; }
	if ( a->position != b->position ) { return FALSE; }
	if ( a->offset != b->offset ) { return FALSE; }
	if ( a->padding != b->padding ) { return FALSE; }
	if ( a->spacing != b->spacing ) { return FALSE; }
	if ( a->orientation != b->orientation ) { return FALSE; }
	if ( a->alignment != b->alignment ) { return FALSE; }
	if ( a->rows_count != b->rows_count ) { return FALSE; }
	if ( memcmp( a->rows, b->rows, sizeof( *a->rows ) * a->rows_count ) ) { return FALSE; }

	return TRUE;
}

//
// Horizontal size of the control center.
//
gint foobar_control_center_configuration_get_width( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->width;
}

//
// Vertical size of the control center.
//
gint foobar_control_center_configuration_get_height( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->height;
}

//
// The screen edge which the control center should be attached to.
//
FoobarScreenEdge foobar_control_center_configuration_get_position( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->position;
}

//
// Offset from the attached screen edge.
//
gint foobar_control_center_configuration_get_offset( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->offset;
}

//
// Spacing between the edges of the control center and its items.
//
gint foobar_control_center_configuration_get_padding( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->padding;
}

//
// Spacing between the items.
//
gint foobar_control_center_configuration_get_spacing( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->spacing;
}

//
// The orientation used to arrange the controls and notifications sections.
//
FoobarOrientation foobar_control_center_configuration_get_orientation( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->orientation;
}

//
// Alignment along the attached screen edge, or "fill".
//
FoobarControlCenterAlignment foobar_control_center_configuration_get_alignment( FoobarControlCenterConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->alignment;
}

//
// Ordered list of rows to display in the controls section.
//
FoobarControlCenterRow const* foobar_control_center_configuration_get_rows(
	FoobarControlCenterConfiguration const* self,
	gsize*                                  out_count )
{
	g_return_val_if_fail( self != NULL, 0 );
	g_return_val_if_fail( out_count != NULL, NULL );

	*out_count = self->rows_count;
	return self->rows;
}

//
// Horizontal size of the control center.
//
void foobar_control_center_configuration_set_width(
	FoobarControlCenterConfiguration* self,
	gint                              value )
{
	g_return_if_fail( self != NULL );
	self->width = value;
}

//
// Vertical size of the control center.
//
void foobar_control_center_configuration_set_height(
	FoobarControlCenterConfiguration* self,
	gint                              value )
{
	g_return_if_fail( self != NULL );
	self->height = value;
}

//
// The screen edge which the control center should be attached to.
//
void foobar_control_center_configuration_set_position(
	FoobarControlCenterConfiguration* self,
	FoobarScreenEdge                  value )
{
	g_return_if_fail( self != NULL );
	self->position = value;
}

//
// Offset from the attached screen edge.
//
void foobar_control_center_configuration_set_offset(
	FoobarControlCenterConfiguration* self,
	gint                              value )
{
	g_return_if_fail( self != NULL );
	self->offset = value;
}

//
// Spacing between the edges of the control center and its items.
//
void foobar_control_center_configuration_set_padding(
	FoobarControlCenterConfiguration* self,
	gint                              value )
{
	g_return_if_fail( self != NULL );
	self->padding = value;
}

//
// Spacing between the items.
//
void foobar_control_center_configuration_set_spacing(
	FoobarControlCenterConfiguration* self,
	gint                              value )
{
	g_return_if_fail( self != NULL );
	self->spacing = value;
}

//
// The orientation used to arrange the controls and notifications sections.
//
void foobar_control_center_configuration_set_orientation(
	FoobarControlCenterConfiguration* self,
	FoobarOrientation                 value )
{
	g_return_if_fail( self != NULL );
	self->orientation = value;
}

//
// Alignment along the attached screen edge, or "fill".
//
void foobar_control_center_configuration_set_alignment(
	FoobarControlCenterConfiguration* self,
	FoobarControlCenterAlignment      value )
{
	g_return_if_fail( self != NULL );
	self->alignment = value;
}

//
// Ordered list of rows to display in the controls section.
//
void foobar_control_center_configuration_set_rows(
	FoobarControlCenterConfiguration* self,
	FoobarControlCenterRow const*     value,
	gsize                             value_count )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( value != NULL || value_count == 0 );

	g_free( self->rows );
	self->rows = g_new( FoobarControlCenterRow, value_count );
	self->rows_count = value_count;
	memcpy( self->rows, value, sizeof( *self->rows ) * self->rows_count );
}

//
// Populate a control center configuration structure from the "control-center" section of a keyfile.
//
void foobar_control_center_configuration_load(
	FoobarControlCenterConfiguration* self,
	GKeyFile*                         file )
{
	gint width;
	if ( try_get_int_value( file, "control-center", "width", VALIDATE_POSITIVE, &width ) )
	{
		foobar_control_center_configuration_set_width( self, width );
	}

	gint height;
	if ( try_get_int_value( file, "control-center", "height", VALIDATE_POSITIVE, &height ) )
	{
		foobar_control_center_configuration_set_height( self, height );
	}

	gint position;
	if ( try_get_enum_value( file, "control-center", "position", FOOBAR_TYPE_SCREEN_EDGE, VALIDATE_NONE, &position ) )
	{
		foobar_control_center_configuration_set_position( self, position );
	}

	gint offset;
	if ( try_get_int_value( file, "control-center", "offset", VALIDATE_NON_NEGATIVE, &offset ) )
	{
		foobar_control_center_configuration_set_offset( self, offset );
	}

	gint padding;
	if ( try_get_int_value( file, "control-center", "padding", VALIDATE_NON_NEGATIVE, &padding ) )
	{
		foobar_control_center_configuration_set_padding( self, padding );
	}

	gint spacing;
	if ( try_get_int_value( file, "control-center", "spacing", VALIDATE_NON_NEGATIVE, &spacing ) )
	{
		foobar_control_center_configuration_set_spacing( self, spacing );
	}

	gint orientation;
	if ( try_get_enum_value( file, "control-center", "orientation", FOOBAR_TYPE_ORIENTATION, VALIDATE_NONE, &orientation ) )
	{
		foobar_control_center_configuration_set_orientation( self, orientation );
	}

	gint alignment;
	if ( try_get_enum_value( file, "control-center", "alignment", FOOBAR_TYPE_CONTROL_CENTER_ALIGNMENT, VALIDATE_NONE, &alignment ) )
	{
		foobar_control_center_configuration_set_alignment( self, alignment );
	}

	g_autofree gpointer rows = NULL;
	gsize rows_count;
	if ( try_get_enum_list_value( file, "control-center", "rows", FOOBAR_TYPE_CONTROL_CENTER_ROW, VALIDATE_DISTINCT, &rows, &rows_count ) )
	{
		foobar_control_center_configuration_set_rows( self, rows, rows_count );
	}
}

//
// Store a control center configuration structure in the "control-center" section of a keyfile.
//
void foobar_control_center_configuration_store(
	FoobarControlCenterConfiguration const* self,
	GKeyFile*                               file )
{
	gint width = foobar_control_center_configuration_get_width( self );
	g_key_file_set_integer( file, "control-center", "width", width );

	gint height = foobar_control_center_configuration_get_height( self );
	g_key_file_set_integer( file, "control-center", "height", height );

	FoobarScreenEdge position = foobar_control_center_configuration_get_position( self );
	g_key_file_set_string( file, "control-center", "position", enum_value_to_string( FOOBAR_TYPE_SCREEN_EDGE, position ) );
	g_key_file_set_comment(
		file,
		"control-center",
		"position",
		" The screen edge which the control center should be attached to.",
		NULL );

	gint offset = foobar_control_center_configuration_get_offset( self );
	g_key_file_set_integer( file, "control-center", "offset", offset );
	g_key_file_set_comment(
		file,
		"control-center",
		"offset",
		" Offset from the attached screen edge.",
		NULL );

	gint padding = foobar_control_center_configuration_get_padding( self );
	g_key_file_set_integer( file, "control-center", "padding", padding );
	g_key_file_set_comment(
		file,
		"control-center",
		"padding",
		" Spacing between the edges of the control center and its items.",
		NULL );

	gint spacing = foobar_control_center_configuration_get_spacing( self );
	g_key_file_set_integer( file, "control-center", "spacing", spacing );
	g_key_file_set_comment(
		file,
		"control-center",
		"spacing",
		" Spacing between the items.",
		NULL );

	FoobarOrientation orientation = foobar_control_center_configuration_get_orientation( self );
	g_key_file_set_string( file, "control-center", "orientation", enum_value_to_string( FOOBAR_TYPE_ORIENTATION, orientation ) );
	g_key_file_set_comment(
		file,
		"control-center",
		"orientation",
		" The orientation used to arrange the controls and notifications sections.",
		NULL );

	FoobarControlCenterAlignment alignment = foobar_control_center_configuration_get_alignment( self );
	g_key_file_set_string( file, "control-center", "alignment", enum_value_to_string( FOOBAR_TYPE_CONTROL_CENTER_ALIGNMENT, alignment ) );
	g_key_file_set_comment(
		file,
		"control-center",
		"alignment",
		" Alignment along the attached screen edge, or \"fill\".",
		NULL );

	gsize rows_count;
	FoobarControlCenterRow const* rows = foobar_control_center_configuration_get_rows( self, &rows_count );
	g_autofree gchar const** rows_str = enum_list_to_string( FOOBAR_TYPE_CONTROL_CENTER_ROW, rows, rows_count );
	g_key_file_set_string_list( file, "control-center", "rows", rows_str, rows_count );
	g_key_file_set_comment(
		file,
		"control-center",
		"rows",
		" Ordered list of rows to display in the controls section.",
		NULL );
}

// ---------------------------------------------------------------------------------------------------------------------
// Notification Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default notification configuration structure.
//
FoobarNotificationConfiguration* foobar_notification_configuration_new( void )
{
	return foobar_notification_configuration_copy( &default_notification_configuration );
}

//
// Create a mutable copy of another notification configuration structure.
//
FoobarNotificationConfiguration* foobar_notification_configuration_copy( FoobarNotificationConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarNotificationConfiguration* copy = g_new0( FoobarNotificationConfiguration, 1 );
	copy->width = self->width;
	copy->min_height = self->min_height;
	copy->spacing = self->spacing;
	copy->close_button_inset = self->close_button_inset;
	copy->time_format = g_strdup( self->time_format );
	return copy;
}

//
// Release resources associated with a notification configuration structure.
//
void foobar_notification_configuration_free( FoobarNotificationConfiguration* self )
{
	g_return_if_fail( self != NULL );

	g_free( self->time_format );
	g_free( self );
}

//
// Check if two notification configuration structures are equal.
//
gboolean foobar_notification_configuration_equal(
	FoobarNotificationConfiguration const* a,
	FoobarNotificationConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( a->width != b->width ) { return FALSE; }
	if ( a->min_height != b->min_height ) { return FALSE; }
	if ( a->spacing != b->spacing ) { return FALSE; }
	if ( a->close_button_inset != b->close_button_inset ) { return FALSE; }
	if ( g_strcmp0( a->time_format, b->time_format ) ) { return FALSE; }

	return TRUE;
}

//
// Horizontal size of notifications in the notification area.
//
gint foobar_notification_configuration_get_width( FoobarNotificationConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->width;
}

//
// Minimum height for each notification.
//
gint foobar_notification_configuration_get_min_height( FoobarNotificationConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->min_height;
}

//
// Spacing between notifications and from the screen edges.
//
gint foobar_notification_configuration_get_spacing( FoobarNotificationConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->spacing;
}

//
// Inset of the close button within the notification (may be negative).
//
gint foobar_notification_configuration_get_close_button_inset( FoobarNotificationConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->close_button_inset;
}

//
// The time format string as used by g_date_time_format.
//
gchar const* foobar_notification_configuration_get_time_format( FoobarNotificationConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->time_format;
}

//
// Horizontal size of notifications in the notification area.
//
void foobar_notification_configuration_set_width(
	FoobarNotificationConfiguration* self,
	gint                             value )
{
	g_return_if_fail( self != NULL );
	self->width = value;
}

//
// Minimum height for each notification.
//
void foobar_notification_configuration_set_min_height(
	FoobarNotificationConfiguration* self,
	gint                             value )
{
	g_return_if_fail( self != NULL );
	self->min_height = value;
}

//
// Spacing between notifications and from the screen edges.
//
void foobar_notification_configuration_set_spacing(
	FoobarNotificationConfiguration* self,
	gint                             value )
{
	g_return_if_fail( self != NULL );
	self->spacing = value;
}

//
// Inset of the close button within the notification (may be negative).
//
void foobar_notification_configuration_set_close_button_inset(
	FoobarNotificationConfiguration* self,
	gint                             value )
{
	g_return_if_fail( self != NULL );
	self->close_button_inset = value;
}

//
// The time format string as used by g_date_time_format.
//
void foobar_notification_configuration_set_time_format(
	FoobarNotificationConfiguration* self,
	gchar const*                     value )
{
	g_return_if_fail( self != NULL );
	g_return_if_fail( value != NULL );

	g_free( self->time_format );
	self->time_format = g_strdup( value );
}

//
// Populate a notification configuration structure from the "notifications" section of a keyfile.
//
void foobar_notification_configuration_load(
	FoobarNotificationConfiguration* self,
	GKeyFile*                        file )
{
	gint width;
	if ( try_get_int_value( file, "notifications", "width", VALIDATE_POSITIVE, &width ) )
	{
		foobar_notification_configuration_set_width( self, width );
	}

	gint min_height;
	if ( try_get_int_value( file, "notifications", "min-height", VALIDATE_NON_NEGATIVE, &min_height ) )
	{
		foobar_notification_configuration_set_min_height( self, min_height );
	}

	gint spacing;
	if ( try_get_int_value( file, "notifications", "spacing", VALIDATE_NON_NEGATIVE, &spacing ) )
	{
		foobar_notification_configuration_set_spacing( self, spacing );
	}

	gint close_button_inset;
	if ( try_get_int_value( file, "notifications", "close-button-inset", VALIDATE_NONE, &close_button_inset ) )
	{
		foobar_notification_configuration_set_close_button_inset( self, close_button_inset );
	}

	g_autofree gchar* time_format = NULL;
	if ( try_get_string_value( file, "notifications", "time-format", VALIDATE_NONE, &time_format ) )
	{
		foobar_notification_configuration_set_time_format( self, time_format );
	}
}

//
// Store a notification configuration structure in the "notifications" section of a keyfile.
//
void foobar_notification_configuration_store(
	FoobarNotificationConfiguration const* self,
	GKeyFile*                              file )
{
	gint width = foobar_notification_configuration_get_width( self );
	g_key_file_set_integer( file, "notifications", "width", width );

	gint min_height = foobar_notification_configuration_get_min_height( self );
	g_key_file_set_integer( file, "notifications", "min-height", min_height );
	g_key_file_set_comment(
		file,
		"notifications",
		"min-height",
		" Minimum height for each notification.",
		NULL );

	gint spacing = foobar_notification_configuration_get_spacing( self );
	g_key_file_set_integer( file, "notifications", "spacing", spacing );
	g_key_file_set_comment(
		file,
		"notifications",
		"spacing",
		" Spacing between notifications and from the screen edges.",
		NULL );

	gint close_button_inset = foobar_notification_configuration_get_close_button_inset( self );
	g_key_file_set_integer( file, "notifications", "close-button-inset", close_button_inset );
	g_key_file_set_comment(
		file,
		"notifications",
		"close-button-inset",
		" Inset of the close button within the notification (may be negative).",
		NULL );

	gchar const* time_format = foobar_notification_configuration_get_time_format( self );
	g_key_file_set_string( file, "notifications", "time-format", time_format );
	g_key_file_set_comment(
		file,
		"notifications",
		"time-format",
		" The time format string as used by g_date_time_format.",
		NULL );
}

// ---------------------------------------------------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a default configuration structure.
//
FoobarConfiguration* foobar_configuration_new( void )
{
	return foobar_configuration_copy( &default_configuration );
}

//
// Create a mutable copy of another configuration structure.
//
FoobarConfiguration* foobar_configuration_copy( FoobarConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );

	FoobarConfiguration* copy = g_new0( FoobarConfiguration, 1 );
	copy->general = foobar_general_configuration_copy( self->general );
	copy->panel = foobar_panel_configuration_copy( self->panel );
	copy->launcher = foobar_launcher_configuration_copy( self->launcher );
	copy->control_center = foobar_control_center_configuration_copy( self->control_center );
	copy->notifications = foobar_notification_configuration_copy( self->notifications );
	return copy;
}

//
// Release resources associated with a configuration structure.
//
void foobar_configuration_free( FoobarConfiguration* self )
{
	g_return_if_fail( self != NULL );

	foobar_general_configuration_free( self->general );
	foobar_panel_configuration_free( self->panel );
	foobar_launcher_configuration_free( self->launcher );
	foobar_control_center_configuration_free( self->control_center );
	foobar_notification_configuration_free( self->notifications );
	g_free( self );
}

//
// Check if two configuration structures are equal.
//
gboolean foobar_configuration_equal(
	FoobarConfiguration const* a,
	FoobarConfiguration const* b )
{
	g_return_val_if_fail( a != NULL, FALSE );
	g_return_val_if_fail( b != NULL, FALSE );

	if ( !foobar_general_configuration_equal( a->general, b->general ) ) { return FALSE; }
	if ( !foobar_panel_configuration_equal( a->panel, b->panel ) ) { return FALSE; }
	if ( !foobar_launcher_configuration_equal( a->launcher, b->launcher ) ) { return FALSE; }
	if ( !foobar_control_center_configuration_equal( a->control_center, b->control_center ) ) { return FALSE; }
	if ( !foobar_notification_configuration_equal( a->notifications, b->notifications ) ) { return FALSE; }

	return TRUE;
}

//
// Get the general application configuration.
//
FoobarGeneralConfiguration const* foobar_configuration_get_general( FoobarConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->general;
}

//
// Get a mutable reference to the general application configuration.
//
FoobarGeneralConfiguration* foobar_configuration_get_general_mut( FoobarConfiguration* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->general;
}

//
// Get the panel configuration.
//
FoobarPanelConfiguration const* foobar_configuration_get_panel( FoobarConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->panel;
}

//
// Get a mutable reference to the panel configuration.
//
FoobarPanelConfiguration* foobar_configuration_get_panel_mut( FoobarConfiguration* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->panel;
}

//
// Get the launcher configuration.
//
FoobarLauncherConfiguration const* foobar_configuration_get_launcher( FoobarConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->launcher;
}

//
// Get a mutable reference to the launcher configuration.
//
FoobarLauncherConfiguration* foobar_configuration_get_launcher_mut( FoobarConfiguration* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->launcher;
}

//
// Get the control center configuration.
//
FoobarControlCenterConfiguration const* foobar_configuration_get_control_center( FoobarConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->control_center;
}

//
// Get a mutable reference to the control center configuration.
//
FoobarControlCenterConfiguration* foobar_configuration_get_control_center_mut( FoobarConfiguration* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->control_center;
}

//
// Get the notification configuration.
//
FoobarNotificationConfiguration const* foobar_configuration_get_notifications( FoobarConfiguration const* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->notifications;
}

//
// Get a mutable reference to the notification configuration.
//
FoobarNotificationConfiguration* foobar_configuration_get_notifications_mut( FoobarConfiguration* self )
{
	g_return_val_if_fail( self != NULL, NULL );
	return self->notifications;
}

//
// Populate a configuration structure from a keyfile.
//
void foobar_configuration_load(
	FoobarConfiguration* self,
	GKeyFile*            file )
{
	foobar_general_configuration_load( foobar_configuration_get_general_mut( self ), file );
	foobar_panel_configuration_load( foobar_configuration_get_panel_mut( self ), file );
	foobar_launcher_configuration_load( foobar_configuration_get_launcher_mut( self ), file );
	foobar_control_center_configuration_load( foobar_configuration_get_control_center_mut( self ), file );
	foobar_notification_configuration_load( foobar_configuration_get_notifications_mut( self ), file );
}

//
// Store a configuration structure in a keyfile.
//
void foobar_configuration_store(
	FoobarConfiguration const* self,
	GKeyFile*                  file )
{
	foobar_general_configuration_store( foobar_configuration_get_general( self ), file );
	foobar_panel_configuration_store( foobar_configuration_get_panel( self ), file );
	foobar_launcher_configuration_store( foobar_configuration_get_launcher( self ), file );
	foobar_control_center_configuration_store( foobar_configuration_get_control_center( self ), file );
	foobar_notification_configuration_store( foobar_configuration_get_notifications( self ), file );
}

//
// Populate a configuration structure from a keyfile at the given path.
//
void foobar_configuration_load_from_file(
	FoobarConfiguration* self,
	gchar const*         path )
{
	g_autoptr( GKeyFile ) file = g_key_file_new( );
	g_autoptr( GError ) error = NULL;
	if ( !g_key_file_load_from_file( file, path, G_KEY_FILE_NONE, &error ) )
	{
		CONFIGURATION_WARNING( "%s", error->message );
		return;
	}
	foobar_configuration_load( self, file );
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the configuration service.
//
void foobar_configuration_service_class_init( FoobarConfigurationServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_configuration_service_get_property;
	object_klass->finalize = foobar_configuration_service_finalize;

	props[PROP_CURRENT] = g_param_spec_boxed(
		"current",
		"Current",
		"The current configuration state.",
		FOOBAR_TYPE_CONFIGURATION,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the configuration service.
//
void foobar_configuration_service_init( FoobarConfigurationService* self )
{
	self->path = g_strdup_printf( "%s/foobar.conf", g_get_user_config_dir( ) );
	self->current = foobar_configuration_new( );
	g_return_if_fail( self->path != NULL );

	// Ensure that the file at path exists.

	if ( g_file_test( self->path, G_FILE_TEST_EXISTS ) )
	{
		foobar_configuration_load_from_file( self->current, self->path );
	}
	else
	{
		g_info( "Config file does not exist -- falling back to default config and creating a copy of it." );
		g_autoptr( GKeyFile ) file = g_key_file_new( );
		foobar_configuration_store( self->current, file );
		g_key_file_save_to_file( file, self->path, NULL );
	}

	// Monitor the config file.

	self->monitor = foobar_configuration_service_create_monitor( self->path );
	if ( self->monitor )
	{
		self->changed_handler_id = g_signal_connect(
			self->monitor,
			"changed",
			G_CALLBACK( foobar_configuration_service_handle_changed ),
			self );
	}

	// In case the config is symlinked, also watch the actual file for content changes.

	self->actual_path = realpath( self->path, NULL );
	if ( self->actual_path && g_strcmp0( self->actual_path, self->path ) )
	{
		self->actual_monitor = foobar_configuration_service_create_monitor( self->actual_path );
		if ( self->actual_monitor )
		{
			self->actual_changed_handler_id = g_signal_connect(
				self->actual_monitor,
				"changed",
				G_CALLBACK( foobar_configuration_service_handle_actual_changed ),
				self );
		}
	}
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_configuration_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarConfigurationService* self = (FoobarConfigurationService*)object;

	switch ( prop_id )
	{
		case PROP_CURRENT:
			g_value_set_boxed( value, foobar_configuration_service_get_current( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the configuration service.
//
void foobar_configuration_service_finalize( GObject* object )
{
	FoobarConfigurationService* self = (FoobarConfigurationService*)object;

	g_clear_handle_id( &self->update_source_id, g_source_remove );
	g_clear_signal_handler( &self->changed_handler_id, self->monitor );
	g_clear_signal_handler( &self->actual_changed_handler_id, self->actual_monitor );
	g_clear_object( &self->monitor );
	g_clear_object( &self->actual_monitor );
	g_clear_pointer( &self->current, foobar_configuration_free );
	g_clear_pointer( &self->path, g_free );
	g_clear_pointer( &self->actual_path, free );

	G_OBJECT_CLASS( foobar_configuration_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new configuration service instance.
//
FoobarConfigurationService* foobar_configuration_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_CONFIGURATION_SERVICE, NULL );
}

//
// Get the current configuration state.
//
FoobarConfiguration const* foobar_configuration_service_get_current( FoobarConfigurationService* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONFIGURATION_SERVICE( self ), NULL );
	return self->current;
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the file at self->path has changed.
//
void foobar_configuration_service_handle_changed(
	GFileMonitor*     monitor,
	GFile*            file,
	GFile*            other_file,
	GFileMonitorEvent event_type,
	gpointer          userdata )
{
	(void)monitor;
	(void)file;
	(void)other_file;
	FoobarConfigurationService* self = (FoobarConfigurationService*)userdata;

	// If this file is a symlink, the actual path might have changed.

	char* actual_path = realpath( self->path, NULL );
	if ( g_strcmp0( self->actual_path, actual_path ) )
	{
		free(self->actual_path);
		self->actual_path = g_steal_pointer( &actual_path );

		// Recreate the file monitor.

		g_clear_signal_handler( &self->actual_changed_handler_id, self->actual_monitor );
		g_clear_object( &self->actual_monitor );

		if ( self->actual_path && g_strcmp0( self->actual_path, self->path ) )
		{
			self->actual_monitor = foobar_configuration_service_create_monitor( self->actual_path );
			if ( self->actual_monitor )
			{
				self->actual_changed_handler_id = g_signal_connect(
					self->actual_monitor,
					"changed",
					G_CALLBACK( foobar_configuration_service_handle_actual_changed ),
					self );
			}
		}
	}
	free(actual_path);

	// Handle actual file changes.

	if ( event_type == G_FILE_MONITOR_EVENT_CREATED || event_type == G_FILE_MONITOR_EVENT_CHANGED )
	{
		foobar_configuration_service_update( self );
	}
}

//
// Called when the actual file (the one at self->actual_path) behind the symlink has changed.
//
void foobar_configuration_service_handle_actual_changed(
	GFileMonitor*     monitor,
	GFile*            file,
	GFile*            other_file,
	GFileMonitorEvent event_type,
	gpointer          userdata )
{
	(void)monitor;
	(void)file;
	(void)other_file;
	FoobarConfigurationService* self = (FoobarConfigurationService*)userdata;

	if ( event_type == G_FILE_MONITOR_EVENT_CREATED || event_type == G_FILE_MONITOR_EVENT_CHANGED )
	{
		foobar_configuration_service_update( self );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Schedule a configuration file reload, used to coalesce change events.
//
void foobar_configuration_service_update( FoobarConfigurationService* self )
{
	if ( !self->update_source_id )
	{
		self->update_source_id = g_timeout_add(
			UPDATE_APPLY_DELAY,
			foobar_configuration_service_update_cb,
			self );
	}
}

//
// Reload the configuration file.
//
gboolean foobar_configuration_service_update_cb( gpointer userdata )
{
	FoobarConfigurationService* self = (FoobarConfigurationService*)userdata;

	FoobarConfiguration* updated = foobar_configuration_new( );
	foobar_configuration_load_from_file( updated, self->path );
	if ( !foobar_configuration_equal( self->current, updated ) )
	{
		foobar_configuration_free( self->current );
		self->current = g_steal_pointer( &updated );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CURRENT] );
		g_info( "Config reloaded." );
	}
	g_clear_pointer( &updated, foobar_configuration_free );

	self->update_source_id = 0;
	return G_SOURCE_REMOVE;
}

//
// Create a new file monitor, logging a warning on error.
//
GFileMonitor* foobar_configuration_service_create_monitor( gchar const* path )
{
	g_autoptr( GError ) error = NULL;
	g_autoptr( GFile ) file = g_file_new_for_path( path );
	GFileMonitor* monitor = g_file_monitor( file, G_FILE_MONITOR_NONE, NULL, &error );
	if ( !monitor )
	{
		g_warning( "Unable to monitor config file: %s", error->message );
	}

	return monitor;
}

// ---------------------------------------------------------------------------------------------------------------------
// Parsing Helpers
// ---------------------------------------------------------------------------------------------------------------------

//
// Get and validate a string value from a keyfile. *out should always be freed, even if FALSE is returned.
//
gboolean try_get_string_value(
	GKeyFile*       file,
	gchar const*    group,
	gchar const*    key,
	ValidationFlags validation,
	gchar**         out )
{
	if ( !g_key_file_has_key( file, group, key, NULL ) ) { return FALSE; }

	g_autoptr( GError ) error = NULL;
	*out = g_key_file_get_string( file, group, key, &error );
	if ( !*out )
	{
		CONFIGURATION_WARNING( "%s:%s: %s", group, key, error->message );
		return FALSE;
	}

	if ( validation & VALIDATE_FILE_URL )
	{
		g_autoptr( GFile ) test_file = g_file_new_for_uri( *out );
		if ( !g_file_query_exists( test_file, NULL ) )
		{
			CONFIGURATION_WARNING( "%s:%s: A file with the URL \"%s\" does not exist.", group, key, *out );
			return FALSE;
		}
	}

	return TRUE;
}

//
// Get and validate a list of string values from a keyfile. *out should always be freed, even if FALSE is returned.
//
gboolean try_get_string_list_value(
	GKeyFile*       file,
	gchar const*    group,
	gchar const*    key,
	ValidationFlags validation,
	GStrv*          out,
	gsize*          out_count )
{
	if ( !g_key_file_has_key( file, group, key, NULL ) ) { return FALSE; }

	g_autoptr( GError ) error = NULL;
	*out = g_key_file_get_string_list( file, group, key, out_count, &error );
	if ( !*out )
	{
		CONFIGURATION_WARNING( "%s:%s: %s", group, key, error->message );
		return FALSE;
	}

	if ( validation & VALIDATE_DISTINCT )
	{
		g_autoptr( GHashTable ) items = g_hash_table_new( g_str_hash, g_str_equal );
		for ( gchar** it = *out; *it; ++it )
		{
			if ( !g_hash_table_add( items, *it ) )
			{
				CONFIGURATION_WARNING( "%s:%s: Duplicate value \"%s\" not allowed.", group, key, *it );
				return FALSE;
			}
		}
	}

	return TRUE;
}

//
// Get and validate an integer value from a keyfile.
//
gboolean try_get_int_value(
	GKeyFile*       file,
	gchar const*    group,
	gchar const*    key,
	ValidationFlags validation,
	gint*           out )
{
	if ( !g_key_file_has_key( file, group, key, NULL ) ) { return FALSE; }

	g_autoptr( GError ) error = NULL;
	*out = g_key_file_get_integer( file, group, key, &error );
	if ( error )
	{
		CONFIGURATION_WARNING( "%s:%s: %s", group, key, error->message );
		return FALSE;
	}

	if ( validation & VALIDATE_NON_NEGATIVE )
	{
		if ( *out < 0 )
		{
			CONFIGURATION_WARNING( "%s:%s: Value is not allowed to be negative.", group, key );
			return FALSE;
		}
	}

	if ( validation & VALIDATE_NON_ZERO )
	{
		if ( *out == 0 )
		{
			CONFIGURATION_WARNING( "%s:%s: Value is not allowed to be 0.", group, key );
			return FALSE;
		}
	}

	return TRUE;
}

//
// Get and validate an enum value (encoded as a string) from a keyfile.
//
gboolean try_get_enum_value(
	GKeyFile*       file,
	gchar const*    group,
	gchar const*    key,
	GType           enum_type,
	ValidationFlags validation,
	gint*           out )
{
	(void)validation;

	g_autofree gchar* str = NULL;
	if ( !try_get_string_value( file, group, key, VALIDATE_NONE, &str ) ) { return FALSE; }

	g_autoptr( GEnumClass ) enum_klass = g_type_class_ref( enum_type );
	GEnumValue* val = g_enum_get_value_by_nick( enum_klass, str );
	if ( !val )
	{
		g_autofree gchar* help_text = enum_format_help_text( enum_type );
		CONFIGURATION_WARNING( "%s:%s: Invalid enum value \"%s\" (allowed values: %s).", group, key, str, help_text );
		return FALSE;
	}

	*out = val->value;
	return TRUE;
}

//
// Get and validate a list of enum values (encoded as strings) from a keyfile. *out should always be freed, even if
// FALSE is returned.
//
gboolean try_get_enum_list_value(
	GKeyFile*       file,
	gchar const*    group,
	gchar const*    key,
	GType           enum_type,
	ValidationFlags validation,
	gpointer*       out,
	gsize*          out_count )
{
	g_auto( GStrv ) str_values = NULL;
	if ( !try_get_string_list_value( file, group, key, validation, &str_values, out_count ) ) { return FALSE; }

	gint* untyped_out = g_new0( gint, *out_count );
	*out = untyped_out;

	g_autoptr( GEnumClass ) enum_klass = g_type_class_ref( enum_type );
	for ( gsize i = 0; i < *out_count; ++i )
	{
		GEnumValue* val = g_enum_get_value_by_nick( enum_klass, str_values[i] );
		if ( !val )
		{
			g_autofree gchar* help_text = enum_format_help_text( enum_type );
			CONFIGURATION_WARNING( "%s:%s: Invalid enum value \"%s\" (allowed values: %s).", group, key, str_values[i], help_text );
			return FALSE;
		}

		untyped_out[i] = val->value;
	}

	return TRUE;
}

//
// Get and validate a boolean value from a keyfile.
//
gboolean try_get_boolean_value(
	GKeyFile*       file,
	gchar const*    group,
	gchar const*    key,
	ValidationFlags validation,
	gboolean*       out )
{
	(void)validation;

	if ( !g_key_file_has_key( file, group, key, NULL ) ) { return FALSE; }

	g_autoptr( GError ) error = NULL;
	*out = g_key_file_get_boolean( file, group, key, &error );
	if ( error )
	{
		CONFIGURATION_WARNING( "%s:%s: %s", group, key, error->message );
		return FALSE;
	}

	return TRUE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------------------------------------------------

//
// Get the string constant for an enum value.
//
gchar const* enum_value_to_string(
	GType enum_type,
	gint  value )
{
	g_autoptr( GEnumClass ) enum_klass = g_type_class_ref( enum_type );
	GEnumValue* val = g_enum_get_value( enum_klass, value );
	return val->value_nick;
}

//
// Get a list of string constants for a list of enum values. Only the list itself needs to be freed.
//
gchar const** enum_list_to_string(
	GType         enum_type,
	gconstpointer values,
	gsize         values_count )
{
	gint const* untyped_values = values;
	gchar const** result = g_new0( gchar const*, values_count );

	g_autoptr( GEnumClass ) enum_klass = g_type_class_ref( enum_type );
	for ( gsize i = 0; i < values_count; ++i )
	{
		GEnumValue* val = g_enum_get_value( enum_klass, untyped_values[i] );
		result[i] = val->value_nick;
	}

	return result;
}

//
// Return a comma-separated list of all allowed values for an enum type. The result needs to be freed.
//
gchar* enum_format_help_text( GType enum_type )
{
	GString* result = g_string_new( NULL );
	g_autoptr( GEnumClass ) enum_klass = g_type_class_ref( enum_type );

	gboolean is_first = TRUE;
	for ( guint i = 0; i < enum_klass->n_values; ++i )
	{
		g_string_append_printf( result, is_first ? "\"%s\"" : ", \"%s\"", enum_klass->values[i].value_nick );
		is_first = FALSE;
	}

	return g_string_free_and_steal( result );
}

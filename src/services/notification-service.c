#include "services/notification-service.h"
#include "dbus/notifications.h"
#include "utils.h"
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>

#define DEFAULT_TIMEOUT 3000

//
// FoobarNotificationUrgency:
//
// The urgency of a notification as provided by the application that sent it.
//

G_DEFINE_ENUM_TYPE(
	FoobarNotificationUrgency,
	foobar_notification_urgency,
	G_DEFINE_ENUM_VALUE( FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_COMFORT, "loss-of-comfort" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_MONEY, "loss-of-money" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_LIFE, "loss-of-life" ) )

//
// FoobarNotificationAction:
//
// A labeled action for a notification that can be invoked by the notification daemon (us).
//

struct _FoobarNotificationAction
{
	GObject             parent_instance;
	FoobarNotification* notification;
	gchar*              id;
	gchar*              label;
};

enum
{
	ACTION_PROP_ID = 1,
	ACTION_PROP_LABEL,
	N_ACTION_PROPS,
};

static GParamSpec* action_props[N_ACTION_PROPS] = { 0 };

static void                      foobar_notification_action_class_init  ( FoobarNotificationActionClass* klass );
static void                      foobar_notification_action_init        ( FoobarNotificationAction*      self );
static void                      foobar_notification_action_get_property( GObject*                       object,
                                                                          guint                          prop_id,
                                                                          GValue*                        value,
                                                                          GParamSpec*                    pspec );
static void                      foobar_notification_action_finalize    ( GObject*                       object );
static FoobarNotificationAction* foobar_notification_action_new         ( FoobarNotification*            notification );
static void                      foobar_notification_action_set_id      ( FoobarNotificationAction*      self,
                                                                          gchar const*                   value );
static void                      foobar_notification_action_set_label   ( FoobarNotificationAction*      self,
                                                                          gchar const*                   value );

G_DEFINE_FINAL_TYPE( FoobarNotificationAction, foobar_notification_action, G_TYPE_OBJECT )

//
// FoobarNotification:
//
// A notification received by the notification daemon (us).
//

struct _FoobarNotification
{
	GObject                    parent_instance;
	guint                      timeout_id;
	FoobarNotificationService* service;
	guint                      id;
	GPtrArray*                 actions;
	gchar*                     app_entry;
	gchar*                     app_name;
	gchar*                     body;
	gchar*                     summary;
	GdkPixbuf*                 image;
	gchar*                     image_path;
	gchar*                     image_data;
	gboolean                   is_dismissed;
	gboolean                   is_resident;
	gboolean                   is_transient;
	GDateTime*                 time;
	gint64                     timeout;
	FoobarNotificationUrgency  urgency;
};

enum
{
	NOTIFICATION_PROP_ID = 1,
	NOTIFICATION_PROP_APP_ENTRY,
	NOTIFICATION_PROP_APP_NAME,
	NOTIFICATION_PROP_BODY,
	NOTIFICATION_PROP_SUMMARY,
	NOTIFICATION_PROP_IMAGE,
	NOTIFICATION_PROP_IS_DISMISSED,
	NOTIFICATION_PROP_IS_RESIDENT,
	NOTIFICATION_PROP_IS_TRANSIENT,
	NOTIFICATION_PROP_TIME,
	NOTIFICATION_PROP_TIMEOUT,
	NOTIFICATION_PROP_URGENCY,
	N_NOTIFICATION_PROPS,
};

static GParamSpec* notification_props[N_NOTIFICATION_PROPS] = { 0 };

static void                foobar_notification_class_init         ( FoobarNotificationClass*   klass );
static void                foobar_notification_init               ( FoobarNotification*        self );
static void                foobar_notification_get_property       ( GObject*                   object,
                                                                    guint                      prop_id,
                                                                    GValue*                    value,
                                                                    GParamSpec*                pspec );
static void                foobar_notification_finalize           ( GObject*                   object );
static FoobarNotification* foobar_notification_new                ( FoobarNotificationService* service );
static gchar const*        foobar_notification_get_image_path     ( FoobarNotification*        self );
static gchar const*        foobar_notification_get_image_data     ( FoobarNotification*        self );
static void                foobar_notification_set_id             ( FoobarNotification*        self,
                                                                    guint                      value );
static void                foobar_notification_set_app_entry      ( FoobarNotification*        self,
                                                                    gchar const*               value );
static void                foobar_notification_set_app_name       ( FoobarNotification*        self,
                                                                    gchar const*               value );
static void                foobar_notification_set_body           ( FoobarNotification*        self,
                                                                    gchar const*               value );
static void                foobar_notification_set_summary        ( FoobarNotification*        self,
                                                                    gchar const*               value );
static void                foobar_notification_set_image_from_path( FoobarNotification*        self,
                                                                    gchar const*               value );
static void                foobar_notification_set_image_from_data( FoobarNotification*        self,
                                                                    gchar const*               value );
static void                foobar_notification_set_image          ( FoobarNotification*        self,
                                                                    GdkPixbuf*                 value );
static void                foobar_notification_set_dismissed      ( FoobarNotification*        self,
                                                                    gboolean                   value );
static void                foobar_notification_set_resident       ( FoobarNotification*        self,
                                                                    gboolean                   value );
static void                foobar_notification_set_transient      ( FoobarNotification*        self,
                                                                    gboolean                   value );
static void                foobar_notification_set_time           ( FoobarNotification*        self,
                                                                    GDateTime*                 value );
static void                foobar_notification_set_timeout        ( FoobarNotification*        self,
                                                                    gint64                     value );
static void                foobar_notification_set_urgency        ( FoobarNotification*        self,
                                                                    FoobarNotificationUrgency  value );
static void                foobar_notification_add_action         ( FoobarNotification*        self,
                                                                    FoobarNotificationAction*  action );
static void                foobar_notification_free_action        ( gpointer                   action );
static gboolean            foobar_notification_handle_timeout     ( gpointer                   userdata );

G_DEFINE_FINAL_TYPE( FoobarNotification, foobar_notification, G_TYPE_OBJECT )

//
// FoobarNotificationService:
//
// Service acting as the notification daemon, receiving incoming notifications from other applications. The service also
// persistently saves previously received notifications until they are closed.
//

struct _FoobarNotificationService
{
	GObject              parent_instance;
	GListStore*          notifications;
	GtkSortListModel*    sorted_notifications;
	GtkFilterListModel*  popup_notifications;
	FoobarNotifications* skeleton;
	guint                bus_owner_id;
	guint                next_id;
	gchar*               cache_path;
	GMutex               write_cache_mutex;
};

enum
{
	PROP_NOTIFICATIONS = 1,
	PROP_POPUP_NOTIFICATIONS,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void     foobar_notification_service_class_init                   ( FoobarNotificationServiceClass* klass );
static void     foobar_notification_service_init                         ( FoobarNotificationService*      self );
static void     foobar_notification_service_get_property                 ( GObject*                        object,
                                                                           guint                           prop_id,
                                                                           GValue*                         value,
                                                                           GParamSpec*                     pspec );
static void     foobar_notification_service_finalize                     ( GObject*                        object );
static void     foobar_notification_service_read_cache                   ( FoobarNotificationService*      self );
static void     foobar_notification_service_write_cache                  ( FoobarNotificationService*      self );
static void     foobar_notification_service_write_cache_cb               ( GObject*                        object,
                                                                           GAsyncResult*                   result,
                                                                           gpointer                        userdata );
static void     foobar_notification_service_write_cache_async            ( FoobarNotificationService*      self,
                                                                           GPtrArray*                      notifications,
                                                                           GCancellable*                   cancellable,
                                                                           GAsyncReadyCallback             callback,
                                                                           gpointer                        userdata );
static gboolean foobar_notification_service_write_cache_finish           ( FoobarNotificationService*      self,
                                                                           GAsyncResult*                   result,
                                                                           GError**                        error );
static void     foobar_notification_service_write_cache_thread           ( GTask*                          task,
                                                                           gpointer                        source_object,
                                                                           gpointer                        task_data,
                                                                           GCancellable*                   cancellable );
static void     foobar_notification_service_handle_bus_acquired          ( GDBusConnection*                connection,
                                                                           gchar const*                    name,
                                                                           gpointer                        userdata );
static void     foobar_notification_service_handle_bus_lost              ( GDBusConnection*                connection,
                                                                           gchar const*                    name,
                                                                           gpointer                        userdata );
static gboolean foobar_notification_service_handle_notify                ( FoobarNotifications*            iface,
                                                                           GDBusMethodInvocation*          invocation,
                                                                           gchar const*                    app_name,
                                                                           guint                           replaces_id,
                                                                           gchar const*                    image,
                                                                           gchar const*                    summary,
                                                                           gchar const*                    body,
                                                                           gchar const* const*             actions,
                                                                           GVariant*                       hints,
                                                                           gint                            expiration,
                                                                           gpointer                        userdata );
static gboolean foobar_notification_service_handle_close_notification    ( FoobarNotifications*            iface,
                                                                           GDBusMethodInvocation*          invocation,
                                                                           guint                           id,
                                                                           gpointer                        userdata );
static gboolean foobar_notification_service_handle_get_capabilities      ( FoobarNotifications*            iface,
                                                                           GDBusMethodInvocation*          invocation,
                                                                           gpointer                        userdata );
static gboolean foobar_notification_service_handle_get_server_information( FoobarNotifications*            iface,
                                                                           GDBusMethodInvocation*          invocation,
                                                                           gpointer                        userdata );
static gboolean foobar_notification_service_popup_filter_func            ( gpointer                        item,
                                                                           gpointer                        userdata );
static gint     foobar_notification_service_sort_func                    ( gconstpointer                   item_a,
                                                                           gconstpointer                   item_b,
                                                                           gpointer                        userdata );

G_DEFINE_FINAL_TYPE( FoobarNotificationService, foobar_notification_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Notification Action
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for notification actions.
//
void foobar_notification_action_class_init( FoobarNotificationActionClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_notification_action_get_property;
	object_klass->finalize = foobar_notification_action_finalize;

	action_props[ACTION_PROP_ID] = g_param_spec_string(
		"id",
		"ID",
		"Textual ID of the action within the notification.",
		NULL,
		G_PARAM_READABLE );
	action_props[ACTION_PROP_LABEL] = g_param_spec_string(
		"label",
		"Label",
		"Human-readable title for the action.",
		NULL,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_ACTION_PROPS, action_props );
}

//
// Instance initialization for notification actions.
//
void foobar_notification_action_init( FoobarNotificationAction* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_notification_action_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNotificationAction* self = (FoobarNotificationAction*)object;

	switch ( prop_id )
	{
		case ACTION_PROP_ID:
			g_value_set_string( value, foobar_notification_action_get_id( self ) );
			break;
		case ACTION_PROP_LABEL:
			g_value_set_string( value, foobar_notification_action_get_label( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for notification actions.
//
void foobar_notification_action_finalize( GObject* object )
{
	FoobarNotificationAction* self = (FoobarNotificationAction*)object;

	g_clear_pointer( &self->id, g_free );
	g_clear_pointer( &self->label, g_free );

	G_OBJECT_CLASS( foobar_notification_action_parent_class )->finalize( object );
}

//
// Create a new notification action that is owned by the given notification (captured as an unowned reference).
//
FoobarNotificationAction* foobar_notification_action_new( FoobarNotification* notification )
{
	FoobarNotificationAction* self = g_object_new( FOOBAR_TYPE_NOTIFICATION_ACTION, NULL );
	self->notification = notification;
	return self;
}

//
// Get the textual ID of the action within the notification.
//
gchar const* foobar_notification_action_get_id( FoobarNotificationAction* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_ACTION( self ), NULL );
	return self->id;
}

//
// Get a human-readable title for the action.
//
gchar const* foobar_notification_action_get_label( FoobarNotificationAction* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_ACTION( self ), NULL );
	return self->label;
}

//
// Update the textual ID of the action within the notification.
//
void foobar_notification_action_set_id(
	FoobarNotificationAction* self,
	gchar const*              value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_ACTION( self ) );

	if ( g_strcmp0( self->id, value ) )
	{
		g_clear_pointer( &self->id, g_free );
		self->id = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), action_props[ACTION_PROP_ID] );
	}
}

//
// Set a human-readable title for the action.
//
void foobar_notification_action_set_label(
	FoobarNotificationAction* self,
	gchar const*              value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_ACTION( self ) );

	if ( g_strcmp0( self->label, value ) )
	{
		g_clear_pointer( &self->label, g_free );
		self->label = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), action_props[ACTION_PROP_LABEL] );
	}
}

//
// Invoke this action by emitting a signal that can be handled by its sender application.
//
void foobar_notification_action_invoke( FoobarNotificationAction* self )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_ACTION( self ) );

	if ( self->notification && self->notification->service )
	{
		FoobarNotification* notification = self->notification;
		FoobarNotificationService* service = notification->service;

		foobar_notifications_emit_action_invoked(
			service->skeleton,
			foobar_notification_get_id( notification ),
			foobar_notification_action_get_id( self ) );

		if ( !foobar_notification_is_resident( notification ) )
		{
			foobar_notification_close( notification );
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for notifications.
//
void foobar_notification_class_init( FoobarNotificationClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_notification_get_property;
	object_klass->finalize = foobar_notification_finalize;

	notification_props[NOTIFICATION_PROP_ID] = g_param_spec_uint(
		"id",
		"ID",
		"Numeric ID of the notification.",
		0,
		UINT_MAX,
		0,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_APP_ENTRY] = g_param_spec_string(
		"app-entry",
		"App Entry",
		"The desktop file of the notification's application.",
		NULL,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_APP_NAME] = g_param_spec_string(
		"app-name",
		"App Name",
		"Human-readable name of the application.",
		NULL,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_BODY] = g_param_spec_string(
		"body",
		"Body",
		"Main content of the notification.",
		NULL,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_SUMMARY] = g_param_spec_string(
		"summary",
		"Summary",
		"Brief summary of the notification's content.",
		NULL,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_IMAGE] = g_param_spec_object(
		"image",
		"Image",
		"An image icon for the notification.",
		GDK_TYPE_PIXBUF,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_IS_DISMISSED] = g_param_spec_boolean(
		"is-dismissed",
		"Is Dismissed",
		"Indicates that the notification was dismissed or the timeout has passed.",
		FALSE,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_IS_RESIDENT] = g_param_spec_boolean(
		"is-resident",
		"Is Resident",
		"Indicates that the notification is not automatically removed once an action is invoked.",
		FALSE,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_IS_TRANSIENT] = g_param_spec_boolean(
		"is-transient",
		"Is Transient",
		"Indicates that the persistence capability should be bypassed.",
		FALSE,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_TIME] = g_param_spec_boxed(
		"time",
		"Time",
		"The timestamp at which the notification was received.",
		G_TYPE_DATE_TIME,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_TIMEOUT] = g_param_spec_int64(
		"timeout",
		"Timeout",
		"Timeout after which the notification auto-closes in milliseconds.",
		0,
		INT64_MAX,
		0,
		G_PARAM_READABLE );
	notification_props[NOTIFICATION_PROP_URGENCY] = g_param_spec_enum(
		"urgency",
		"Urgency",
		"The urgency state of the notification.",
		FOOBAR_TYPE_NOTIFICATION_URGENCY,
		FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_MONEY,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_NOTIFICATION_PROPS, notification_props );
}

//
// Instance initialization for notifications.
//
void foobar_notification_init( FoobarNotification* self )
{
	self->actions = g_ptr_array_new_with_free_func( foobar_notification_free_action );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_notification_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNotification* self = (FoobarNotification*)object;

	switch ( prop_id )
	{
		case NOTIFICATION_PROP_ID:
			g_value_set_int64( value, foobar_notification_get_id( self ) );
			break;
		case NOTIFICATION_PROP_APP_ENTRY:
			g_value_set_string( value, foobar_notification_get_app_entry( self ) );
			break;
		case NOTIFICATION_PROP_APP_NAME:
			g_value_set_string( value, foobar_notification_get_app_name( self ) );
			break;
		case NOTIFICATION_PROP_BODY:
			g_value_set_string( value, foobar_notification_get_body( self ) );
			break;
		case NOTIFICATION_PROP_SUMMARY:
			g_value_set_string( value, foobar_notification_get_summary( self ) );
			break;
		case NOTIFICATION_PROP_IMAGE:
			g_value_set_object( value, foobar_notification_get_image( self ) );
			break;
		case NOTIFICATION_PROP_IS_DISMISSED:
			g_value_set_boolean( value, foobar_notification_is_dismissed( self ) );
			break;
		case NOTIFICATION_PROP_IS_RESIDENT:
			g_value_set_boolean( value, foobar_notification_is_resident( self ) );
			break;
		case NOTIFICATION_PROP_IS_TRANSIENT:
			g_value_set_boolean( value, foobar_notification_is_transient( self ) );
			break;
		case NOTIFICATION_PROP_TIME:
			g_value_set_boxed( value, foobar_notification_get_time( self ) );
			break;
		case NOTIFICATION_PROP_TIMEOUT:
			g_value_set_int64( value, foobar_notification_get_timeout( self ) );
			break;
		case NOTIFICATION_PROP_URGENCY:
			g_value_set_enum( value, foobar_notification_get_urgency( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance initialization for notifications.
//
void foobar_notification_finalize( GObject* object )
{
	FoobarNotification* self = (FoobarNotification*)object;

	g_clear_object( &self->image );
	g_clear_pointer( &self->actions, g_ptr_array_unref );
	g_clear_pointer( &self->app_entry, g_free );
	g_clear_pointer( &self->app_name, g_free );
	g_clear_pointer( &self->body, g_free );
	g_clear_pointer( &self->summary, g_free );
	g_clear_pointer( &self->image_path, g_free );
	g_clear_pointer( &self->image_data, g_free );
	g_clear_pointer( &self->time, g_date_time_unref );

	G_OBJECT_CLASS( foobar_notification_parent_class )->finalize( object );
}

//
// Create a new notification that is owned by the given service (captured as an unowned reference).
//
FoobarNotification* foobar_notification_new( FoobarNotificationService* service )
{
	FoobarNotification* self = g_object_new( FOOBAR_TYPE_NOTIFICATION, NULL );
	self->service = service;
	return self;
}

//
// Numeric ID of the notification.
//
guint foobar_notification_get_id( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), 0 );
	return self->id;
}

//
// A list of actions which can be invoked by notifying the notification's sender.
//
FoobarNotificationAction** foobar_notification_get_actions(
	FoobarNotification* self,
	guint*              out_count )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	if ( out_count ) { *out_count = self->actions->len; }
	return (FoobarNotificationAction**)self->actions->pdata;
}

//
// The desktop file of the notification's application.
//
gchar const* foobar_notification_get_app_entry( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->app_entry;
}

//
// Human-readable name of the application.
//
gchar const* foobar_notification_get_app_name( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->app_name;
}

//
// Main content of the notification.
//
gchar const* foobar_notification_get_body( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->body;
}

//
// Brief summary of the notification's content.
//
gchar const* foobar_notification_get_summary( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->summary;
}

//
// The source path for the notification's image if it was loaded from a file.
//
gchar const* foobar_notification_get_image_path( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->image_path;
}

//
// The base64-encoded data for the notification's image if it was loaded from data.
//
gchar const* foobar_notification_get_image_data( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->image_data;
}

//
// An image icon for the notification.
//
GdkPixbuf* foobar_notification_get_image( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->image;
}

//
// Indicates that the notification was dismissed or the timeout has passed.
//
gboolean foobar_notification_is_dismissed( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), FALSE );
	return self->is_dismissed;
}

//
// Indicates that the notification is not automatically removed once an action is invoked.
//
gboolean foobar_notification_is_resident( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), FALSE );
	return self->is_resident;
}

//
// Indicates that the persistence capability should be bypassed.
//
gboolean foobar_notification_is_transient( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), FALSE );
	return self->is_transient;
}

//
// The timestamp at which the notification was received.
//
GDateTime* foobar_notification_get_time( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), NULL );
	return self->time;
}

//
// Timeout after which the notification auto-closes in milliseconds.
//
gint64 foobar_notification_get_timeout( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), 0 );
	return self->timeout;
}

//
// The urgency state of the notification.
//
FoobarNotificationUrgency foobar_notification_get_urgency( FoobarNotification* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION( self ), FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_MONEY );
	return self->urgency;
}

//
// Numeric ID of the notification.
//
void foobar_notification_set_id(
	FoobarNotification* self,
	guint               value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( self->id != value )
	{
		self->id = value;
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_ID] );
	}
}

//
// The desktop file of the notification's application.
//
void foobar_notification_set_app_entry(
	FoobarNotification* self,
	gchar const*        value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( g_strcmp0( self->app_entry, value ) )
	{
		g_clear_pointer( &self->app_entry, g_free );
		self->app_entry = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_APP_ENTRY] );
	}
}

//
// Human-readable name of the application.
//
void foobar_notification_set_app_name(
	FoobarNotification* self,
	gchar const*        value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( g_strcmp0( self->app_name, value ) )
	{
		g_clear_pointer( &self->app_name, g_free );
		self->app_name = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_APP_NAME] );
	}
}

//
// Main content of the notification.
//
void foobar_notification_set_body(
	FoobarNotification* self,
	gchar const*        value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( g_strcmp0( self->body, value ) )
	{
		g_clear_pointer( &self->body, g_free );
		self->body = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_BODY] );
	}
}

//
// Brief summary of the notification's content.
//
void foobar_notification_set_summary(
	FoobarNotification* self,
	gchar const*        value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( g_strcmp0( self->summary, value ) )
	{
		g_clear_pointer( &self->summary, g_free );
		self->summary = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_SUMMARY] );
	}
}

//
// Update the notification's image by trying to load a file at the provided path.
//
void foobar_notification_set_image_from_path(
	FoobarNotification* self,
	gchar const*        value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( g_strcmp0( self->image_path, value ) )
	{
		g_clear_pointer( &self->image_path, g_free );
		g_clear_pointer( &self->image_data, g_free );
		g_clear_object( &self->image );
		self->image_path = g_strdup( value );

		g_autoptr( GError ) error = NULL;
		self->image = gdk_pixbuf_new_from_file( self->image_path, &error );
		if ( !self->image )
		{
			g_warning( "Invalid image path for notification: %s", error->message );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_IMAGE] );
	}
}

//
// Update the notification's image by trying to load base64-encoded data.
//
void foobar_notification_set_image_from_data(
	FoobarNotification* self,
	gchar const*        value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( g_strcmp0( self->image_data, value ) )
	{
		g_clear_pointer( &self->image_path, g_free );
		g_clear_pointer( &self->image_data, g_free );
		g_clear_object( &self->image );
		self->image_data = g_strdup( value );

		gsize   data_len;
		guchar* data = g_base64_decode( self->image_data, &data_len );
		g_autoptr( GError ) error = NULL;
		g_autoptr( GInputStream ) input = g_memory_input_stream_new_from_data( data, (gssize)data_len, g_free );
		self->image = gdk_pixbuf_new_from_stream( input, NULL, &error );
		if ( !self->image )
		{
			g_warning( "Invalid image data for notification: %s", error->message );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_IMAGE] );
	}
}

//
// Update the notification's image to a custom pixbuf.
//
static void foobar_notification_set_image(
	FoobarNotification* self,
	GdkPixbuf*          value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( self->image != value )
	{
		g_clear_pointer( &self->image_path, g_free );
		g_clear_pointer( &self->image_data, g_free );
		g_clear_object( &self->image );

		if ( value )
		{
			self->image = g_object_ref( value );
			g_autoptr( GError ) error = NULL;
			gchar* data;
			gsize  data_len;
			if ( gdk_pixbuf_save_to_buffer( self->image, &data, &data_len, "png", &error, NULL ) )
			{
				self->image_data = g_base64_encode( (guchar const*)data, data_len );
			}
			else
			{
				g_warning( "Unable to save image for notification: %s", error->message );
				g_clear_object( &self->image );
			}
		}

		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_IMAGE] );
	}
}

//
// Indicates that the notification was dismissed or the timeout has passed.
//
void foobar_notification_set_dismissed(
	FoobarNotification* self,
	gboolean            value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	value = !!value;
	if ( self->is_dismissed != value )
	{
		self->is_dismissed = value;
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_IS_DISMISSED] );

		if ( self->service )
		{
			gtk_filter_changed(
				gtk_filter_list_model_get_filter( self->service->popup_notifications ),
				value ? GTK_FILTER_CHANGE_MORE_STRICT : GTK_FILTER_CHANGE_LESS_STRICT );
		}
	}
}

//
// Indicates that the notification is not automatically removed once an action is invoked.
//
void foobar_notification_set_resident(
	FoobarNotification* self,
	gboolean            value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	value = !!value;
	if ( self->is_resident != value )
	{
		self->is_resident = value;
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_IS_RESIDENT] );
	}
}

//
// Indicates that the persistence capability should be bypassed.
//
void foobar_notification_set_transient(
	FoobarNotification* self,
	gboolean            value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	value = !!value;
	if ( self->is_transient != value )
	{
		self->is_transient = value;
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_IS_TRANSIENT] );
	}
}

//
// The timestamp at which the notification was received.
//
void foobar_notification_set_time(
	FoobarNotification* self,
	GDateTime*          value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( self->time != value )
	{
		if ( self->time ) { g_date_time_unref( self->time ); }
		self->time = g_date_time_ref( value );
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_TIME] );
	}
}

//
// Timeout after which the notification auto-closes in milliseconds.
//
void foobar_notification_set_timeout(
	FoobarNotification* self,
	gint64              value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	value = MAX( value, 0 );
	if ( self->timeout != value )
	{
		self->timeout = value;
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_TIMEOUT] );
	}
}

//
// The urgency state of the notification.
//
void foobar_notification_set_urgency(
	FoobarNotification*       self,
	FoobarNotificationUrgency value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( self->urgency != value )
	{
		self->urgency = value;
		g_object_notify_by_pspec( G_OBJECT( self ), notification_props[NOTIFICATION_PROP_URGENCY] );
	}
}

//
// Register an action for the notification.
//
void foobar_notification_add_action(
	FoobarNotification*       self,
	FoobarNotificationAction* action )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );
	g_return_if_fail( action->notification == self );

	g_ptr_array_add( self->actions, g_object_ref( action ) );
}

//
// Block the notification from automatically being dismissed.
//
void foobar_notification_block_timeout( FoobarNotification* self )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	g_clear_handle_id( &self->timeout_id, g_source_remove );
}

//
// If not already started, start the timeout to automatically dismiss the notification after it's timeout.
//
void foobar_notification_resume_timeout( FoobarNotification* self )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( !self->is_dismissed && !self->timeout_id )
	{
		self->timeout_id = g_timeout_add_full(
			G_NOTIFICATION_PRIORITY_NORMAL,
			self->timeout,
			foobar_notification_handle_timeout,
			g_object_ref( self ),
			g_object_unref );
	}
}

//
// Manually dismiss the notification, setting "is-dismissed" to TRUE.
//
void foobar_notification_dismiss( FoobarNotification* self )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	foobar_notification_set_dismissed( self, TRUE );
}

//
// Close the notification, removing it from the parent service's list.
//
void foobar_notification_close( FoobarNotification* self )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION( self ) );

	if ( self->service )
	{
		FoobarNotificationService* service = self->service;
		self->service = NULL;

		guint id = foobar_notification_get_id( self );

		for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( service->notifications ) ); ++i )
		{
			if ( g_list_model_get_item( G_LIST_MODEL( service->notifications ), i ) == self )
			{
				g_list_store_remove( service->notifications, i );
				foobar_notification_service_write_cache( service );
				break;
			}
		}

		foobar_notifications_emit_notification_closed( service->skeleton, id, 3 );
	}
}

//
// Called when an action is removed from a notification, resetting its weak reference to the notification.
//
void foobar_notification_free_action( gpointer action )
{
	FoobarNotificationAction* object = (FoobarNotificationAction*)action;
	object->notification = NULL;
	g_object_unref( object );
}

//
// Called after the notification's timeout has elapsed.
//
gboolean foobar_notification_handle_timeout( gpointer userdata )
{
	FoobarNotification* notification = (FoobarNotification*)userdata;

	foobar_notification_dismiss( notification );
	notification->timeout_id = 0;

	return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the notification service.
//
void foobar_notification_service_class_init( FoobarNotificationServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_notification_service_get_property;
	object_klass->finalize = foobar_notification_service_finalize;

	props[PROP_NOTIFICATIONS] = g_param_spec_object(
		"notifications",
		"Notifications",
		"Sorted list of all notifications.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	props[PROP_POPUP_NOTIFICATIONS] = g_param_spec_object(
		"popup-notifications",
		"Popup Notifications",
		"Sorted list of all visible notifications (i.e. notifications that are not dismissed).",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the notification service.
//
void foobar_notification_service_init( FoobarNotificationService* self )
{
	self->notifications = g_list_store_new( FOOBAR_TYPE_NOTIFICATION );
	self->cache_path = foobar_get_cache_path( "notifications.json" );
	g_mutex_init( &self->write_cache_mutex );

	GtkCustomSorter* sorter = gtk_custom_sorter_new( foobar_notification_service_sort_func, NULL, NULL );
	self->sorted_notifications = gtk_sort_list_model_new(
		G_LIST_MODEL( g_object_ref( self->notifications ) ),
		GTK_SORTER( sorter ) );

	GtkCustomFilter* popup_filter = gtk_custom_filter_new( foobar_notification_service_popup_filter_func, NULL, NULL );
	self->popup_notifications = gtk_filter_list_model_new(
		G_LIST_MODEL( g_object_ref( self->sorted_notifications ) ),
		GTK_FILTER( popup_filter ) );

	foobar_notification_service_read_cache( self );

	self->bus_owner_id = g_bus_own_name(
		G_BUS_TYPE_SESSION,
		"org.freedesktop.Notifications",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		foobar_notification_service_handle_bus_acquired,
		NULL,
		foobar_notification_service_handle_bus_lost,
		self,
		NULL );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_notification_service_get_property( GObject* object, guint prop_id, GValue* value, GParamSpec* pspec )
{
	FoobarNotificationService* self = (FoobarNotificationService*)object;

	switch ( prop_id )
	{
		case PROP_NOTIFICATIONS:
			g_value_set_object( value, foobar_notification_service_get_notifications( self ) );
			break;
		case PROP_POPUP_NOTIFICATIONS:
			g_value_set_object( value, foobar_notification_service_get_popup_notifications( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the notification service.
//
void foobar_notification_service_finalize( GObject* object )
{
	FoobarNotificationService* self = (FoobarNotificationService*)object;

	if ( self->skeleton ) { g_dbus_interface_skeleton_unexport( G_DBUS_INTERFACE_SKELETON( self->skeleton ) ); }

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->notifications ) ); ++i )
	{
		FoobarNotification* notification = g_list_model_get_item( G_LIST_MODEL( self->notifications ), i );
		notification->service = NULL;
	}

	g_clear_object( &self->popup_notifications );
	g_clear_object( &self->sorted_notifications );
	g_clear_object( &self->notifications );
	g_clear_object( &self->skeleton );
	g_clear_handle_id( &self->bus_owner_id, g_bus_unown_name );
	g_clear_pointer( &self->cache_path, g_free );

	g_mutex_clear( &self->write_cache_mutex );

	G_OBJECT_CLASS( foobar_notification_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new notification service instance.
//
FoobarNotificationService* foobar_notification_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_NOTIFICATION_SERVICE, NULL );
}

//
// Get a sorted list of all notifications.
//
GListModel* foobar_notification_service_get_notifications( FoobarNotificationService* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->sorted_notifications );
}

//
// Get a sorted list of all visible notifications (i.e. notifications that are not dismissed).
//
GListModel* foobar_notification_service_get_popup_notifications( FoobarNotificationService* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->popup_notifications );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called after the process has asynchronously acquired the notification bus.
//
// This is where we can export notification daemon functionality using the corresponding DBus skeleton object.
//
void foobar_notification_service_handle_bus_acquired(
	GDBusConnection* connection,
	gchar const*     name,
	gpointer         userdata )
{
	(void)name;
	FoobarNotificationService* self = (FoobarNotificationService*)userdata;

	self->skeleton = foobar_notifications_skeleton_new( );
	g_signal_connect(
		self->skeleton,
		"handle-notify",
		G_CALLBACK( foobar_notification_service_handle_notify ),
		self );
	g_signal_connect(
		self->skeleton,
		"handle-close-notification",
		G_CALLBACK( foobar_notification_service_handle_close_notification ),
		self );
	g_signal_connect(
		self->skeleton,
		"handle-get-capabilities",
		G_CALLBACK( foobar_notification_service_handle_get_capabilities ),
		self );
	g_signal_connect(
		self->skeleton,
		"handle-get-server-information",
		G_CALLBACK( foobar_notification_service_handle_get_server_information ),
		self );

	g_autoptr( GError ) error = NULL;
	if ( !g_dbus_interface_skeleton_export(
			G_DBUS_INTERFACE_SKELETON( self->skeleton ),
			connection,
			"/org/freedesktop/Notifications",
			&error ) )
	{
		g_warning( "Unable to export notifications interface: %s", error->message );
	}
}

//
// Called after the process has failed to acquire the notification bus.
//
void foobar_notification_service_handle_bus_lost(
	GDBusConnection* connection,
	gchar const*     name,
	gpointer         userdata )
{
	(void)connection;
	(void)name;
	(void)userdata;

	g_autofree gchar* other_name = NULL;
	g_autoptr( FoobarNotifications ) proxy = foobar_notifications_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.freedesktop.Notifications",
		"/org/freedesktop/Notifications",
		NULL,
		NULL );

	if ( proxy )
	{
		foobar_notifications_call_get_server_information_sync(
			proxy,
			&other_name,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL );
	}

	g_warning(
		"Unable to register notification daemon because %s is already running.",
		other_name ? other_name : "(unknown)" );
}

//
// DBus skeleton callback for the "Notify" method.
//
gboolean foobar_notification_service_handle_notify(
	FoobarNotifications*   iface,
	GDBusMethodInvocation* invocation,
	gchar const*           app_name,
	guint                  replaces_id,
	gchar const*           image,
	gchar const*           summary,
	gchar const*           body,
	gchar const* const*    actions,
	GVariant*              hints,
	gint                   expiration,
	gpointer               userdata )
{
	FoobarNotificationService* self = (FoobarNotificationService*)userdata;

	g_autoptr( GDateTime ) time = g_date_time_new_now_local( );
	g_autoptr( FoobarNotification ) notification = foobar_notification_new( self );
	foobar_notification_set_id( notification, replaces_id ? replaces_id : self->next_id++ );
	foobar_notification_set_app_name( notification, app_name );
	foobar_notification_set_summary( notification, summary );
	foobar_notification_set_body( notification, body );
	foobar_notification_set_time( notification, time );
	foobar_notification_set_timeout( notification, expiration != -1 ? expiration : DEFAULT_TIMEOUT );
	if ( image && *image ) { foobar_notification_set_image_from_path( notification, image ); }

	if ( hints )
	{
		// Process hints to set some more properties for the notification.

		gsize hint_count = g_variant_n_children( hints );
		for ( gsize i = 0; i < hint_count; ++i )
		{
			g_autoptr( GVariant ) hint = g_variant_get_child_value( hints, i );
			g_autoptr( GVariant ) key = g_variant_get_child_value( hint, 0 );
			g_autoptr( GVariant ) value_wrapper = g_variant_get_child_value( hint, 1 );
			g_autoptr( GVariant ) value = g_variant_get_variant( value_wrapper );
			gchar const* key_str = g_variant_get_string( key, NULL );

			if ( !g_strcmp0( key_str, "desktop-entry" ) )
			{
				foobar_notification_set_app_entry( notification, g_variant_get_string( value, NULL ) );
			}
			else if ( !g_strcmp0( key_str, "resident" ) )
			{
				foobar_notification_set_resident( notification, g_variant_get_boolean( value ) );
			}
			else if ( !g_strcmp0( key_str, "transient" ) )
			{
				foobar_notification_set_transient( notification, g_variant_get_boolean( value ) );
			}
			else if ( !g_strcmp0( key_str, "urgency" ) )
			{
				foobar_notification_set_urgency( notification, g_variant_get_byte( value ) );
			}

			if ( !foobar_notification_get_image( notification ) )
			{
				if ( !g_strcmp0( key_str, "image-path" ) ||
						!g_strcmp0( key_str, "image_path" ) )
				{
					foobar_notification_set_image_from_path( notification, g_variant_get_string( value, NULL ) );
				}
				else if ( !g_strcmp0( key_str, "image-data" ) ||
						!g_strcmp0( key_str, "image_data" ) ||
						!g_strcmp0( key_str, "icon_data" ) )
				{
					gint32 width, height, row_stride, bits_per_sample, channels;
					gboolean has_alpha;
					g_variant_get(
						value,
						"(iiibiiay)",
						&width,
						&height,
						&row_stride,
						&has_alpha,
						&bits_per_sample,
						&channels,
						NULL );

					g_autoptr( GVariant ) data_variant = g_variant_get_child_value( value, 6 );
					gsize data_len;
					guint8 const* data = g_variant_get_fixed_array( data_variant, &data_len, sizeof( guint8 ) );
					g_autoptr( GBytes ) bytes = g_bytes_new( data, data_len );

					g_autoptr( GdkPixbuf ) pixbuf = gdk_pixbuf_new_from_bytes(
						bytes,
						GDK_COLORSPACE_RGB,
						has_alpha,
						bits_per_sample,
						width,
						height,
						row_stride );
					foobar_notification_set_image( notification, pixbuf );
				}
			}
		}
	}

	if ( actions )
	{
		// Actions are stored in a flat array containing the label followed by the ID of the action.

		while ( *actions )
		{
			gchar const* label = *actions++;
			gchar const* id = *actions++;
			if ( !id )
			{
				g_warning( "Action missing id: %s", label );
				break;
			}

			g_autoptr( FoobarNotificationAction ) action = foobar_notification_action_new( notification );
			foobar_notification_action_set_id( action, id );
			foobar_notification_action_set_label( action, label );
			foobar_notification_add_action( notification, action );
		}
	}

	// Add the notification to the list and start the timeout.

	g_list_store_append( self->notifications, notification );
	foobar_notification_resume_timeout( notification );
	foobar_notification_service_write_cache( self );

	foobar_notifications_complete_notify( iface, invocation, foobar_notification_get_id( notification ) );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// DBus skeleton callback for the "CloseNotification" method.
//
gboolean foobar_notification_service_handle_close_notification(
	FoobarNotifications*   iface,
	GDBusMethodInvocation* invocation,
	guint                  id,
	gpointer               userdata )
{
	FoobarNotificationService* self = (FoobarNotificationService*)userdata;

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->notifications ) ); ++i )
	{
		FoobarNotification* notification = g_list_model_get_item( G_LIST_MODEL( self->notifications ), i );
		if ( foobar_notification_get_id( notification ) == id )
		{
			foobar_notification_close( notification );
			break;
		}
	}

	foobar_notifications_complete_close_notification( iface, invocation );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// DBus skeleton callback for the "GetCapabilities" method.
//
gboolean foobar_notification_service_handle_get_capabilities(
	FoobarNotifications*   iface,
	GDBusMethodInvocation* invocation,
	gpointer               userdata )
{
	(void)userdata;

	gchar const* capabilities[] =
		{
			"action-icons",
			"actions",
			"body",
			"body-hyperlinks",
			"body-markup",
			"icon-static",
			"persistence",
			NULL
		};
	foobar_notifications_complete_get_capabilities( iface, invocation, capabilities );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

//
// DBus skeleton callback for the "GetServerInformation" method.
//
gboolean foobar_notification_service_handle_get_server_information(
	FoobarNotifications*   iface,
	GDBusMethodInvocation* invocation,
	gpointer               userdata )
{
	(void)userdata;

	foobar_notifications_complete_get_server_information(
		iface,
		invocation,
		"Foobar",
		"Hannes Schulze",
		"1.0.0",
		"1.2" );
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Synchronously read the cache file at self->cache_path, populating self->notifications and initializing self->next_id.
//
void foobar_notification_service_read_cache( FoobarNotificationService* self )
{
	g_autoptr( GError ) error = NULL;

	if ( self->cache_path && g_file_test( self->cache_path, G_FILE_TEST_EXISTS ) )
	{
		g_autoptr( JsonParser ) parser = json_parser_new( );
		if ( !json_parser_load_from_mapped_file( parser, self->cache_path, &error ) )
		{
			g_warning( "Unable to read cached notifications: %s", error->message );
			return;
		}

		JsonNode* root_node = json_parser_get_root( parser );
		JsonArray* root_array = json_node_get_array( root_node );
		for ( guint i = 0; i < json_array_get_length( root_array ); ++i )
		{
			JsonObject* notification_object = json_array_get_object_element( root_array, i );
			g_autoptr( FoobarNotification ) notification = foobar_notification_new( self );

			foobar_notification_set_dismissed( notification, TRUE );

			guint id = json_object_get_int_member( notification_object, "id" );
			foobar_notification_set_id( notification, id );

			gchar const* app_entry = json_object_get_string_member_with_default( notification_object, "app-entry", NULL );
			foobar_notification_set_app_entry( notification, app_entry );

			gchar const* app_name = json_object_get_string_member_with_default( notification_object, "app-name", NULL );
			foobar_notification_set_app_name( notification, app_name );

			gchar const* body = json_object_get_string_member_with_default( notification_object, "body", NULL );
			foobar_notification_set_body( notification, body );

			gchar const* summary = json_object_get_string_member_with_default( notification_object, "summary", NULL );
			foobar_notification_set_summary( notification, summary );

			gchar const* image_path = json_object_get_string_member_with_default( notification_object, "image-path", NULL );
			gchar const* image_data = json_object_get_string_member_with_default( notification_object, "image-data", NULL );
			if ( image_path ) { foobar_notification_set_image_from_path( notification, image_path ); }
			else if ( image_data ) { foobar_notification_set_image_from_data( notification, image_data ); }

			gboolean is_resident = json_object_get_boolean_member( notification_object, "is-resident" );
			foobar_notification_set_resident( notification, is_resident );

			gchar const* time_str = json_object_get_string_member_with_default( notification_object, "time", NULL );
			GTimeZone* tz = g_time_zone_new_local( );
			GDateTime* time = time_str ? g_date_time_new_from_iso8601( time_str, tz ) : NULL;
			foobar_notification_set_time( notification, time );
			g_time_zone_unref( tz );
			if ( time ) { g_date_time_unref( time ); }

			gint64 timeout = json_object_get_int_member( notification_object, "timeout" );
			foobar_notification_set_timeout( notification, timeout );

			FoobarNotificationUrgency urgency = json_object_get_int_member( notification_object, "urgency" );
			foobar_notification_set_urgency( notification, urgency );

			JsonArray* actions_array = json_object_get_array_member( notification_object, "actions" );
			for ( guint j = 0; j < json_array_get_length( actions_array ); ++j )
			{
				JsonObject* action_object = json_array_get_object_element( actions_array, j );
				g_autoptr( FoobarNotificationAction ) action = foobar_notification_action_new( notification );

				gchar const* action_id = json_object_get_string_member_with_default( action_object, "id", NULL );
				foobar_notification_action_set_id( action, action_id );

				gchar const* label = json_object_get_string_member_with_default( action_object, "label", NULL );
				foobar_notification_action_set_label( action, label );

				foobar_notification_add_action( notification, action );
			}

			g_list_store_append( self->notifications, notification );
			self->next_id = MAX( self->next_id, id + 1 );
		}
	}
}

//
// Asynchronously start writing the cache file at self->cache_path.
//
void foobar_notification_service_write_cache( FoobarNotificationService* self )
{
	guint count = g_list_model_get_n_items( G_LIST_MODEL( self->notifications ) );
	g_autoptr( GPtrArray ) notifications = g_ptr_array_new_full( count, g_object_unref );
	for ( guint i = 0; i < count; ++i )
	{
		FoobarNotification* notification = g_list_model_get_item( G_LIST_MODEL( self->notifications ), i );
		if ( !foobar_notification_is_transient( notification ) )
		{
			g_ptr_array_add( notifications, g_object_ref( notification ) );
		}
	}

	foobar_notification_service_write_cache_async(
		self,
		g_steal_pointer( &notifications ),
		NULL,
		foobar_notification_service_write_cache_cb,
		NULL );
}

//
// Callback invoked when the cache was written successfully or failed.
//
void foobar_notification_service_write_cache_cb(
	GObject*      object,
	GAsyncResult* result,
	gpointer      userdata )
{
	(void)userdata;
	FoobarNotificationService* self = (FoobarNotificationService*)object;

	g_autoptr( GError ) error = NULL;
	if ( !foobar_notification_service_write_cache_finish( self, result, &error ) )
	{
		g_warning( "Unable to write cached notifications: %s", error->message );
	}
}

//
// Asynchronously write the cache file at self->cache_path, using the provided snapshot of the list.
//
void foobar_notification_service_write_cache_async(
	FoobarNotificationService* self,
	GPtrArray*                 notifications,
	GCancellable*              cancellable,
	GAsyncReadyCallback        callback,
	gpointer                   userdata )
{
	g_autoptr( GTask ) task = g_task_new( self, cancellable, callback, userdata );
	g_task_set_name( task, "write-notification-cache" );
	g_task_set_task_data( task, notifications, (GDestroyNotify)g_ptr_array_unref );
	g_task_run_in_thread( task, foobar_notification_service_write_cache_thread );
}

//
// Get the asynchronous result for writing the cache file, returning TRUE on success, or FALSE on error.
//
gboolean foobar_notification_service_write_cache_finish(
	FoobarNotificationService* self,
	GAsyncResult*              result,
	GError**                   error )
{
	(void)self;

	return g_task_propagate_boolean( G_TASK( result ), error );
}

//
// Task implementation for foobar_notification_service_write_cache_async, invoked on a background thread.
//
void foobar_notification_service_write_cache_thread(
	GTask*        task,
	gpointer      source_object,
	gpointer      task_data,
	GCancellable* cancellable )
{
	(void)cancellable;
	FoobarNotificationService* self = (FoobarNotificationService*)source_object;
	GPtrArray* notifications = (GPtrArray*)task_data;

	if ( !self->cache_path )
	{
		g_task_return_boolean( task, TRUE );
		return;
	}

	g_autoptr( GError ) error = NULL;

	g_autoptr( JsonBuilder ) builder = json_builder_new( );
	json_builder_begin_array( builder );
	for ( guint i = 0; i < notifications->len; ++i )
	{
		// Only init-only properties are read here so no extra synchronization is needed.

		FoobarNotification* notification = g_ptr_array_index( notifications, i );

		json_builder_begin_object( builder );

		json_builder_set_member_name( builder, "id" );
		json_builder_add_int_value( builder, foobar_notification_get_id( notification ) );

		json_builder_set_member_name( builder, "app-entry" );
		json_builder_add_string_value( builder, foobar_notification_get_app_entry( notification ) );

		json_builder_set_member_name( builder, "app-name" );
		json_builder_add_string_value( builder, foobar_notification_get_app_name( notification ) );

		json_builder_set_member_name( builder, "body" );
		json_builder_add_string_value( builder, foobar_notification_get_body( notification ) );

		json_builder_set_member_name( builder, "summary" );
		json_builder_add_string_value( builder, foobar_notification_get_summary( notification ) );

		json_builder_set_member_name( builder, "image-path" );
		json_builder_add_string_value( builder, foobar_notification_get_image_path( notification ) );

		json_builder_set_member_name( builder, "image-data" );
		json_builder_add_string_value( builder, foobar_notification_get_image_data( notification ) );

		json_builder_set_member_name( builder, "is-resident" );
		json_builder_add_boolean_value( builder, foobar_notification_is_resident( notification ) );

		GDateTime* time = foobar_notification_get_time( notification );
		gchar* time_str = time ? g_date_time_format_iso8601( time ) : NULL;
		json_builder_set_member_name( builder, "time" );
		json_builder_add_string_value( builder, time_str );
		g_free( time_str );

		json_builder_set_member_name( builder, "timeout" );
		json_builder_add_int_value( builder, foobar_notification_get_timeout( notification ) );

		json_builder_set_member_name( builder, "urgency" );
		json_builder_add_int_value( builder, foobar_notification_get_urgency( notification ) );

		guint action_count;
		FoobarNotificationAction** actions = foobar_notification_get_actions( notification, &action_count );
		json_builder_set_member_name( builder, "actions" );
		json_builder_begin_array( builder );
		for ( guint j = 0; j < action_count; ++j )
		{
			FoobarNotificationAction* action = actions[j];

			json_builder_begin_object( builder );

			json_builder_set_member_name( builder, "id" );
			json_builder_add_string_value( builder, foobar_notification_action_get_id( action ) );

			json_builder_set_member_name( builder, "label" );
			json_builder_add_string_value( builder, foobar_notification_action_get_label( action ) );

			json_builder_end_object( builder );
		}
		json_builder_end_array( builder );

		json_builder_end_object( builder );
	}
	json_builder_end_array( builder );

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
// Filtering callback for the list of pop-up notifications.
//
gboolean foobar_notification_service_popup_filter_func(
	gpointer item,
	gpointer userdata )
{
	(void)userdata;

	FoobarNotification* notification = item;
	return !foobar_notification_is_dismissed( notification );
}

//
// Sorting callback for the list of notifications.
//
// Notifications are sorted by timestamp in descending order.
//
gint foobar_notification_service_sort_func(
	gconstpointer item_a,
	gconstpointer item_b,
	gpointer      userdata )
{
	(void)userdata;

	FoobarNotification* notification_a = (FoobarNotification*)item_a;
	FoobarNotification* notification_b = (FoobarNotification*)item_b;
	return -g_date_time_compare(
		foobar_notification_get_time( notification_a ),
		foobar_notification_get_time( notification_b ) );
}
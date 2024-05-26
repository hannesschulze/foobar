#pragma once

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_NOTIFICATION_URGENCY foobar_notification_urgency_get_type( )
#define FOOBAR_TYPE_NOTIFICATION_ACTION  foobar_notification_action_get_type( )
#define FOOBAR_TYPE_NOTIFICATION         foobar_notification_get_type( )
#define FOOBAR_TYPE_NOTIFICATION_SERVICE foobar_notification_service_get_type( )

typedef enum
{
	// reminder to myself that i should have studied this shit instead of writing a fucking bar in c
	FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_COMFORT = 0,
	FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_MONEY   = 1,
	FOOBAR_NOTIFICATION_URGENCY_LOSS_OF_LIFE    = 2,
} FoobarNotificationUrgency;

GType foobar_notification_urgency_get_type( void );

G_DECLARE_FINAL_TYPE( FoobarNotificationAction, foobar_notification_action, FOOBAR, NOTIFICATION_ACTION, GObject )

gchar const* foobar_notification_action_get_id   ( FoobarNotificationAction* self );
gchar const* foobar_notification_action_get_label( FoobarNotificationAction* self );
void         foobar_notification_action_invoke   ( FoobarNotificationAction* self );

G_DECLARE_FINAL_TYPE( FoobarNotification, foobar_notification, FOOBAR, NOTIFICATION, GObject )

guint                      foobar_notification_get_id        ( FoobarNotification* self );
FoobarNotificationAction** foobar_notification_get_actions   ( FoobarNotification* self,
                                                               guint*              out_count );
gchar const*               foobar_notification_get_app_entry ( FoobarNotification* self );
gchar const*               foobar_notification_get_app_name  ( FoobarNotification* self );
gchar const*               foobar_notification_get_body      ( FoobarNotification* self );
gchar const*               foobar_notification_get_summary   ( FoobarNotification* self );
GdkPixbuf*                 foobar_notification_get_image     ( FoobarNotification* self );
gboolean                   foobar_notification_is_dismissed  ( FoobarNotification* self );
gboolean                   foobar_notification_is_resident   ( FoobarNotification* self );
gboolean                   foobar_notification_is_transient  ( FoobarNotification* self );
GDateTime*                 foobar_notification_get_time      ( FoobarNotification* self );
gint64                     foobar_notification_get_timeout   ( FoobarNotification* self );
FoobarNotificationUrgency  foobar_notification_get_urgency   ( FoobarNotification* self );
void                       foobar_notification_block_timeout ( FoobarNotification* self );
void                       foobar_notification_resume_timeout( FoobarNotification* self );
void                       foobar_notification_dismiss       ( FoobarNotification* self );
void                       foobar_notification_close         ( FoobarNotification* self );

G_DECLARE_FINAL_TYPE( FoobarNotificationService, foobar_notification_service, FOOBAR, NOTIFICATION_SERVICE, GObject )

FoobarNotificationService* foobar_notification_service_new                    ( void );
GListModel*                foobar_notification_service_get_notifications      ( FoobarNotificationService* self );
GListModel*                foobar_notification_service_get_popup_notifications( FoobarNotificationService* self );

G_END_DECLS

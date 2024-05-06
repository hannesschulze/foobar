#pragma once

#include <gtk/gtk.h>
#include "services/notification-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION foobar_notification_close_action_get_type( )
#define FOOBAR_TYPE_NOTIFICATION_WIDGET       foobar_notification_widget_get_type( )

typedef enum
{
	FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_REMOVE,
	FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_DISMISS,
} FoobarNotificationCloseAction;

GType foobar_notification_close_action_get_type( void );

G_DECLARE_FINAL_TYPE( FoobarNotificationWidget, foobar_notification_widget, FOOBAR, NOTIFICATION_WIDGET, GtkWidget )

GtkWidget*                    foobar_notification_widget_new                   ( void );
FoobarNotification*           foobar_notification_widget_get_notification      ( FoobarNotificationWidget*     self );
FoobarNotificationCloseAction foobar_notification_widget_get_close_action      ( FoobarNotificationWidget*     self );
gchar const*                  foobar_notification_widget_get_time_format       ( FoobarNotificationWidget*     self );
gint                          foobar_notification_widget_get_min_height        ( FoobarNotificationWidget*     self );
gint                          foobar_notification_widget_get_close_button_inset( FoobarNotificationWidget*     self );
gint                          foobar_notification_widget_get_inset_start       ( FoobarNotificationWidget*     self );
gint                          foobar_notification_widget_get_inset_end         ( FoobarNotificationWidget*     self );
gint                          foobar_notification_widget_get_inset_top         ( FoobarNotificationWidget*     self );
gint                          foobar_notification_widget_get_inset_bottom      ( FoobarNotificationWidget*     self );
void                          foobar_notification_widget_set_notification      ( FoobarNotificationWidget*     self,
                                                                                 FoobarNotification*           value );
void                          foobar_notification_widget_set_close_action      ( FoobarNotificationWidget*     self,
                                                                                 FoobarNotificationCloseAction value );
void                          foobar_notification_widget_set_time_format       ( FoobarNotificationWidget*     self,
                                                                                 gchar const*                  value );
void                          foobar_notification_widget_set_min_height        ( FoobarNotificationWidget*     self,
                                                                                 gint                          value );
void                          foobar_notification_widget_set_close_button_inset( FoobarNotificationWidget*     self,
                                                                                 gint                          value );
void                          foobar_notification_widget_set_inset_start       ( FoobarNotificationWidget*     self,
                                                                                 gint                          value );
void                          foobar_notification_widget_set_inset_end         ( FoobarNotificationWidget*     self,
                                                                                 gint                          value );
void                          foobar_notification_widget_set_inset_top         ( FoobarNotificationWidget*     self,
                                                                                 gint                          value );
void                          foobar_notification_widget_set_inset_bottom      ( FoobarNotificationWidget*     self,
                                                                                 gint                          value );

G_END_DECLS
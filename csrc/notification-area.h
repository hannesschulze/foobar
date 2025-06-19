#pragma once

#include <gtk/gtk.h>
#include "services/notification-service.h"
#include "services/configuration-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_NOTIFICATION_AREA foobar_notification_area_get_type( )

G_DECLARE_FINAL_TYPE( FoobarNotificationArea, foobar_notification_area, FOOBAR, NOTIFICATION_AREA, GtkWindow )

FoobarNotificationArea* foobar_notification_area_new                ( FoobarNotificationService*             notification_service,
                                                                      FoobarConfigurationService*            configuration_service );
void                    foobar_notification_area_apply_configuration( FoobarNotificationArea*                self,
                                                                      FoobarNotificationConfiguration const* config );

G_END_DECLS

#pragma once

#include <gtk/gtk.h>
#include "services/brightness-service.h"
#include "services/audio-service.h"
#include "services/network-service.h"
#include "services/bluetooth-service.h"
#include "services/notification-service.h"
#include "services/configuration-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_CONTROL_CENTER foobar_control_center_get_type( )

G_DECLARE_FINAL_TYPE( FoobarControlCenter, foobar_control_center, FOOBAR, CONTROL_CENTER, GtkWindow )

FoobarControlCenter* foobar_control_center_new                ( FoobarBrightnessService*                brightness_service,
                                                                FoobarAudioService*                     audio_service,
                                                                FoobarNetworkService*                   network_service,
                                                                FoobarBluetoothService*                 bluetooth_service,
                                                                FoobarNotificationService*              notification_service,
                                                                FoobarConfigurationService*             configuration_service );
void                 foobar_control_center_apply_configuration( FoobarControlCenter*                    self,
                                                                FoobarControlCenterConfiguration const* config,
                                                                FoobarNotificationConfiguration const*  notification_config );

G_END_DECLS

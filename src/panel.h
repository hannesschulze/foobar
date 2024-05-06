#pragma once

#include <gtk/gtk.h>
#include "services/configuration-service.h"
#include "services/battery-service.h"
#include "services/clock-service.h"
#include "services/brightness-service.h"
#include "services/workspace-service.h"
#include "services/audio-service.h"
#include "services/network-service.h"
#include "services/bluetooth-service.h"
#include "services/notification-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_PANEL foobar_panel_get_type( )

G_DECLARE_FINAL_TYPE( FoobarPanel, foobar_panel, FOOBAR, PANEL, GtkApplicationWindow )

FoobarPanel* foobar_panel_new                ( GtkApplication*                 app,
                                               GdkMonitor*                     monitor,
                                               FoobarBatteryService*           battery_service,
                                               FoobarClockService*             clock_service,
                                               FoobarBrightnessService*        brightness_service,
                                               FoobarWorkspaceService*         workspace_service,
                                               FoobarAudioService*             audio_service,
                                               FoobarNetworkService*           network_service,
                                               FoobarBluetoothService*         bluetooth_service,
                                               FoobarNotificationService*      notification_service,
                                               FoobarConfigurationService*     configuration_service );
void         foobar_panel_apply_configuration( FoobarPanel*                    self,
                                               FoobarPanelConfiguration const* config );

G_END_DECLS
#pragma once

#include "widgets/panel/panel-item.h"
#include "services/battery-service.h"
#include "services/brightness-service.h"
#include "services/audio-service.h"
#include "services/network-service.h"
#include "services/bluetooth-service.h"
#include "services/notification-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_PANEL_ITEM_STATUS foobar_panel_item_status_get_type( )

G_DECLARE_FINAL_TYPE( FoobarPanelItemStatus, foobar_panel_item_status, FOOBAR, PANEL_ITEM_STATUS, FoobarPanelItem )

FoobarPanelItem* foobar_panel_item_status_new( FoobarPanelItemConfiguration const* config,
                                               FoobarBatteryService*               battery_service,
                                               FoobarBrightnessService*            brightness_service,
                                               FoobarAudioService*                 audio_service,
                                               FoobarNetworkService*               network_service,
                                               FoobarBluetoothService*             bluetooth_service,
                                               FoobarNotificationService*          notification_service );

G_END_DECLS
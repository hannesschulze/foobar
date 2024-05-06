#pragma once

#include "widgets/panel/panel-item.h"
#include "services/clock-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_PANEL_ITEM_CLOCK foobar_panel_item_clock_get_type( )

G_DECLARE_FINAL_TYPE( FoobarPanelItemClock, foobar_panel_item_clock, FOOBAR, PANEL_ITEM_CLOCK, FoobarPanelItem )

FoobarPanelItem* foobar_panel_item_clock_new( FoobarPanelItemConfiguration const* config,
                                              FoobarClockService*                 clock_service );

G_END_DECLS
#pragma once

#include "widgets/panel/panel-item.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_PANEL_ITEM_ICON foobar_panel_item_icon_get_type( )

G_DECLARE_FINAL_TYPE( FoobarPanelItemIcon, foobar_panel_item_icon, FOOBAR, PANEL_ITEM_ICON, FoobarPanelItem )

FoobarPanelItem* foobar_panel_item_icon_new( FoobarPanelItemConfiguration const* config );

G_END_DECLS

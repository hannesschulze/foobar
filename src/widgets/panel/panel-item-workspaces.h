#pragma once

#include "widgets/panel/panel-item.h"
#include "services/workspace-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_PANEL_ITEM_WORKSPACES foobar_panel_item_workspaces_get_type( )

G_DECLARE_FINAL_TYPE( FoobarPanelItemWorkspaces, foobar_panel_item_workspaces, FOOBAR, PANEL_ITEM_WORKSPACES, FoobarPanelItem )

FoobarPanelItem* foobar_panel_item_workspaces_new( FoobarPanelItemConfiguration const* config,
                                                   GdkMonitor*                         monitor,
                                                   FoobarWorkspaceService*             workspace_service );

G_END_DECLS
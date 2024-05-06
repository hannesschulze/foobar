#pragma once

#include <gtk/gtk.h>
#include "services/configuration-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_PANEL_ITEM foobar_panel_item_get_type( )

G_DECLARE_DERIVABLE_TYPE( FoobarPanelItem, foobar_panel_item, FOOBAR, PANEL_ITEM, GtkWidget )

struct _FoobarPanelItemClass
{
	GtkWidgetClass parent_class;
};

GtkWidget* foobar_panel_item_get_child    ( FoobarPanelItem*      self );
void       foobar_panel_item_set_child    ( FoobarPanelItem*      self,
                                            GtkWidget*            child );
void       foobar_panel_item_invoke_action( FoobarPanelItem*      self,
                                            FoobarPanelItemAction action );

G_END_DECLS
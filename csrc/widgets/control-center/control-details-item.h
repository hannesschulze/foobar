#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_CONTROL_DETAILS_ACCESSORY foobar_control_details_accessory_get_type( )
#define FOOBAR_TYPE_CONTROL_DETAILS_ITEM      foobar_control_details_item_get_type( )

typedef enum
{
	FOOBAR_CONTROL_DETAILS_ACCESSORY_NONE,
	FOOBAR_CONTROL_DETAILS_ACCESSORY_CHECKED,
	FOOBAR_CONTROL_DETAILS_ACCESSORY_PROGRESS,
} FoobarControlDetailsAccessory;

GType foobar_control_details_accessory_get_type( void );

G_DECLARE_FINAL_TYPE( FoobarControlDetailsItem, foobar_control_details_item, FOOBAR, CONTROL_DETAILS_ITEM, GtkWidget )

GtkWidget*                    foobar_control_details_item_new          ( void );
gchar const*                  foobar_control_details_item_get_label    ( FoobarControlDetailsItem*     self );
FoobarControlDetailsAccessory foobar_control_details_item_get_accessory( FoobarControlDetailsItem*     self );
gboolean                      foobar_control_details_item_is_checked   ( FoobarControlDetailsItem*     self );
void                          foobar_control_details_item_set_label    ( FoobarControlDetailsItem*     self,
                                                                         gchar const*                  value );
void                          foobar_control_details_item_set_accessory( FoobarControlDetailsItem*     self,
                                                                         FoobarControlDetailsAccessory value );
void                          foobar_control_details_item_set_checked  ( FoobarControlDetailsItem*     self,
                                                                         gboolean                      value );

G_END_DECLS

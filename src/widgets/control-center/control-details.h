#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_CONTROL_DETAILS foobar_control_details_get_type( )

G_DECLARE_FINAL_TYPE( FoobarControlDetails, foobar_control_details, FOOBAR, CONTROL_DETAILS, GtkWidget )

GtkWidget* foobar_control_details_new          ( void );
GtkWidget* foobar_control_details_get_child    ( FoobarControlDetails* self );
gboolean   foobar_control_details_is_expanded  ( FoobarControlDetails* self );
gint       foobar_control_details_get_inset_top( FoobarControlDetails* self );
void       foobar_control_details_set_child    ( FoobarControlDetails* self,
                                                 GtkWidget*            value );
void       foobar_control_details_set_expanded ( FoobarControlDetails* self,
                                                 gboolean              value );
void       foobar_control_details_set_inset_top( FoobarControlDetails* self,
                                                 gint                  value );

G_END_DECLS
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_INSET_CONTAINER foobar_inset_container_get_type( )

G_DECLARE_FINAL_TYPE( FoobarInsetContainer, foobar_inset_container, FOOBAR, INSET_CONTAINER, GtkWidget )

GtkWidget* foobar_inset_container_new                 ( void );
GtkWidget* foobar_inset_container_get_child           ( FoobarInsetContainer* self );
gint       foobar_inset_container_get_inset_horizontal( FoobarInsetContainer* self );
gint       foobar_inset_container_get_inset_vertical  ( FoobarInsetContainer* self );
void       foobar_inset_container_set_child           ( FoobarInsetContainer* self,
                                                        GtkWidget*            value );
void       foobar_inset_container_set_inset_horizontal( FoobarInsetContainer* self,
                                                        gint                  value );
void       foobar_inset_container_set_inset_vertical  ( FoobarInsetContainer* self,
                                                        gint                  value );

G_END_DECLS
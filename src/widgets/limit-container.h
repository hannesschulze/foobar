#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_LIMIT_CONTAINER foobar_limit_container_get_type( )

G_DECLARE_FINAL_TYPE( FoobarLimitContainer, foobar_limit_container, FOOBAR, LIMIT_CONTAINER, GtkWidget )

GtkWidget* foobar_limit_container_new           ( void );
GtkWidget* foobar_limit_container_get_child     ( FoobarLimitContainer* self );
gint       foobar_limit_container_get_max_width ( FoobarLimitContainer* self );
gint       foobar_limit_container_get_max_height( FoobarLimitContainer* self );
void       foobar_limit_container_set_child     ( FoobarLimitContainer* self,
                                                  GtkWidget*            value );
void       foobar_limit_container_set_max_width ( FoobarLimitContainer* self,
                                                  gint                  value );
void       foobar_limit_container_set_max_height( FoobarLimitContainer* self,
                                                  gint                  value );

G_END_DECLS

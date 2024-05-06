#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_CONTROL_SLIDER foobar_control_slider_get_type( )

G_DECLARE_FINAL_TYPE( FoobarControlSlider, foobar_control_slider, FOOBAR, CONTROL_SLIDER, GtkWidget )

GtkWidget*   foobar_control_slider_new           ( void );
gchar const* foobar_control_slider_get_icon_name ( FoobarControlSlider* self );
gchar const* foobar_control_slider_get_label     ( FoobarControlSlider* self );
gboolean     foobar_control_slider_can_expand    ( FoobarControlSlider* self );
gboolean     foobar_control_slider_is_expanded   ( FoobarControlSlider* self );
gint         foobar_control_slider_get_percentage( FoobarControlSlider* self );
void         foobar_control_slider_set_icon_name ( FoobarControlSlider* self,
                                                   gchar const*         value );
void         foobar_control_slider_set_label     ( FoobarControlSlider* self,
                                                   gchar const*         value );
void         foobar_control_slider_set_can_expand( FoobarControlSlider* self,
                                                   gboolean             value );
void         foobar_control_slider_set_expanded  ( FoobarControlSlider* self,
                                                   gboolean             value );
void         foobar_control_slider_set_percentage( FoobarControlSlider* self,
                                                   gint                 value );

G_END_DECLS
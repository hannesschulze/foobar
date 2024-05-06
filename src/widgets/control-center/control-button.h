#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_CONTROL_BUTTON foobar_control_button_get_type( )

G_DECLARE_FINAL_TYPE( FoobarControlButton, foobar_control_button, FOOBAR, CONTROL_BUTTON, GtkWidget )

GtkWidget*   foobar_control_button_new           ( void );
gchar const* foobar_control_button_get_icon_name ( FoobarControlButton* self );
gchar const* foobar_control_button_get_label     ( FoobarControlButton* self );
gboolean     foobar_control_button_can_expand    ( FoobarControlButton* self );
gboolean     foobar_control_button_can_toggle    ( FoobarControlButton* self );
gboolean     foobar_control_button_is_expanded   ( FoobarControlButton* self );
gboolean     foobar_control_button_is_toggled    ( FoobarControlButton* self );
void         foobar_control_button_set_icon_name ( FoobarControlButton* self,
                                                   gchar const*         value );
void         foobar_control_button_set_label     ( FoobarControlButton* self,
                                                   gchar const*         value );
void         foobar_control_button_set_can_expand( FoobarControlButton* self,
                                                   gboolean             value );
void         foobar_control_button_set_can_toggle( FoobarControlButton* self,
                                                   gboolean             value );
void         foobar_control_button_set_expanded  ( FoobarControlButton* self,
                                                   gboolean             value );
void         foobar_control_button_set_toggled   ( FoobarControlButton* self,
                                                   gboolean             value );

G_END_DECLS
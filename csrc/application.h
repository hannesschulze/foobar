#pragma once

#include <gtk/gtk.h>
#include "services/configuration-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_APPLICATION foobar_application_get_type( )
#define FOOBAR_APPLICATION_DEFAULT FOOBAR_APPLICATION( g_application_get_default( ) )

G_DECLARE_FINAL_TYPE( FoobarApplication, foobar_application, FOOBAR, APPLICATION, GtkApplication )

FoobarApplication* foobar_application_new                  ( void );
void               foobar_application_toggle_launcher      ( FoobarApplication*                self );
void               foobar_application_toggle_control_center( FoobarApplication*                self );
void               foobar_application_apply_configuration  ( FoobarApplication*                self,
                                                             FoobarGeneralConfiguration const* config );

G_END_DECLS

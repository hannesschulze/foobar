#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_BRIGHTNESS_SERVICE foobar_brightness_service_get_type( )

G_DECLARE_FINAL_TYPE( FoobarBrightnessService, foobar_brightness_service, FOOBAR, BRIGHTNESS_SERVICE, GObject )

FoobarBrightnessService* foobar_brightness_service_new           ( void );
gint                     foobar_brightness_service_get_percentage( FoobarBrightnessService* self );
void                     foobar_brightness_service_set_percentage( FoobarBrightnessService* self,
                                                                   gint                     percentage );

G_END_DECLS

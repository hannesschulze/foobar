#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_CLOCK_SERVICE foobar_clock_service_get_type( )

G_DECLARE_FINAL_TYPE( FoobarClockService, foobar_clock_service, FOOBAR, CLOCK_SERVICE, GObject )

FoobarClockService* foobar_clock_service_new     ( void );
GDateTime*          foobar_clock_service_get_time( FoobarClockService* self );

G_END_DECLS

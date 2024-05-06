#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_BATTERY_STATE   foobar_battery_state_get_type( )
#define FOOBAR_TYPE_BATTERY_SERVICE foobar_battery_service_get_type( )

typedef struct _FoobarBatteryState FoobarBatteryState;

GType               foobar_battery_state_get_type          ( void );
FoobarBatteryState* foobar_battery_state_new               ( void );
FoobarBatteryState* foobar_battery_state_copy              ( FoobarBatteryState const* self );
void                foobar_battery_state_free              ( FoobarBatteryState*       self );
gboolean            foobar_battery_state_equal             ( FoobarBatteryState const* a,
                                                             FoobarBatteryState const* b );
gboolean            foobar_battery_state_is_charging       ( FoobarBatteryState const* self );
gboolean            foobar_battery_state_is_charged        ( FoobarBatteryState const* self );
gint                foobar_battery_state_get_percentage    ( FoobarBatteryState const* self );
gint                foobar_battery_state_get_time_remaining( FoobarBatteryState const* self );
void                foobar_battery_state_set_charging      ( FoobarBatteryState*       self,
                                                             gboolean                  value );
void                foobar_battery_state_set_charged       ( FoobarBatteryState*       self,
                                                             gboolean                  value );
void                foobar_battery_state_set_percentage    ( FoobarBatteryState*       self,
                                                             gint                      value );
void                foobar_battery_state_set_time_remaining( FoobarBatteryState*       self,
                                                             gint                      value );

G_DECLARE_FINAL_TYPE( FoobarBatteryService, foobar_battery_service, FOOBAR, BATTERY_SERVICE, GObject )

FoobarBatteryService*     foobar_battery_service_new      ( void );
FoobarBatteryState const* foobar_battery_service_get_state( FoobarBatteryService* self );

G_END_DECLS
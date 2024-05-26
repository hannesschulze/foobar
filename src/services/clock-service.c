#include "services/clock-service.h"

//
// FoobarClockService:
//
// Service providing an auto-updating timestamp value which can be used to display the current time.
//

struct _FoobarClockService
{
	GObject    parent_instance;
	GDateTime* time;
	guint      source_id;
};

enum
{
	PROP_TIME = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void     foobar_clock_service_class_init  ( FoobarClockServiceClass* klass );
static void     foobar_clock_service_init        ( FoobarClockService*      self );
static void     foobar_clock_service_get_property( GObject*                 object,
                                                   guint                    prop_id,
                                                   GValue*                  value,
                                                   GParamSpec*              pspec );
static void     foobar_clock_service_finalize    ( GObject*                 object );
static gboolean foobar_clock_service_handle_tick ( gpointer                 userdata );

G_DEFINE_FINAL_TYPE( FoobarClockService, foobar_clock_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the clock service.
//
void foobar_clock_service_class_init( FoobarClockServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_clock_service_get_property;
	object_klass->finalize = foobar_clock_service_finalize;

	props[PROP_TIME] = g_param_spec_boxed(
		"time",
		"Time",
		"The current time, regularly updated.",
		G_TYPE_DATE_TIME,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the clock service.
//
void foobar_clock_service_init( FoobarClockService* self )
{
	self->time = g_date_time_new_now_local( );
	self->source_id = g_timeout_add_seconds( 1, foobar_clock_service_handle_tick, self );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_clock_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarClockService* self = (FoobarClockService*)object;

	switch ( prop_id )
	{
		case PROP_TIME:
			g_value_set_boxed( value, foobar_clock_service_get_time( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the clock service.
//
void foobar_clock_service_finalize( GObject* object )
{
	FoobarClockService* self = (FoobarClockService*)object;

	g_clear_pointer( &self->time, g_date_time_unref );
	g_clear_handle_id( &self->source_id, g_source_remove );

	G_OBJECT_CLASS( foobar_clock_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new clock service instance.
//
FoobarClockService* foobar_clock_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_CLOCK_SERVICE, NULL );
}

//
// Get the current snapshot of the timestamp.
//
// This may be a bit older than the value returned by g_date_time_new_now_local.
//
GDateTime* foobar_clock_service_get_time( FoobarClockService* self )
{
	g_return_val_if_fail( FOOBAR_IS_CLOCK_SERVICE( self ), NULL );

	return self->time;
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called every time the update delay has elapsed to update the current timestamp value.
//
gboolean foobar_clock_service_handle_tick( gpointer userdata )
{
	FoobarClockService* self = (FoobarClockService*)userdata;

	g_clear_pointer( &self->time, g_date_time_unref );
	self->time = g_date_time_new_now_local( );
	g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_TIME] );

	return G_SOURCE_CONTINUE;
}

#include "services/battery-service.h"
#include "dbus/upower.h"
#include <math.h>

#define DEVICE_STATE_CHARGING      1
#define DEVICE_STATE_FULLY_CHARGED 4

//
// FoobarBatteryState:
//
// Structure storing the current information about the battery.
//

struct _FoobarBatteryState
{
	gboolean is_charging;
	gboolean is_charged;
	gint     percentage;
	gint     time_remaining;
};

G_DEFINE_BOXED_TYPE( FoobarBatteryState, foobar_battery_state, foobar_battery_state_copy, foobar_battery_state_free )

//
// FoobarBatteryService:
//
// Service monitoring the state of the battery. This is implemented using the UPower D-Bus API.
//

struct _FoobarBatteryService
{
	GObject             parent_instance;
	FoobarBatteryState* state;
	FoobarUPowerDevice* proxy;
	gulong              changed_handler_id;
};

enum
{
	PROP_STATE = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_battery_service_class_init               ( FoobarBatteryServiceClass* klass );
static void foobar_battery_service_init                     ( FoobarBatteryService*      self );
static void foobar_battery_service_get_property             ( GObject*                   object,
                                                              guint                      prop_id,
                                                              GValue*                    value,
                                                              GParamSpec*                pspec );
static void foobar_battery_service_finalize                 ( GObject*                   object );
static void foobar_battery_service_handle_connect           ( GObject*                   object,
                                                              GAsyncResult*              result,
                                                              gpointer                   userdata );
static void foobar_battery_service_handle_properties_changed( GDBusProxy*                proxy,
                                                              GVariant*                  changed_properties,
                                                              GStrv                      invalidated_properties,
                                                              gpointer                   userdata );
static void foobar_battery_service_update                   ( FoobarBatteryService*      self );

G_DEFINE_FINAL_TYPE( FoobarBatteryService, foobar_battery_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new, default-initialized state structure.
//
FoobarBatteryState* foobar_battery_state_new( void )
{
	return g_new0( FoobarBatteryState, 1 );
}

//
// Create a mutable copy of a state structure.
//
FoobarBatteryState* foobar_battery_state_copy( FoobarBatteryState const* self )
{
	if ( !self ) { return NULL; }

	FoobarBatteryState* copy = g_new0( FoobarBatteryState, 1 );
	copy->is_charging = self->is_charging;
	copy->is_charged = self->is_charged;
	copy->percentage = self->percentage;
	copy->time_remaining = self->time_remaining;
	return copy;
}

//
// Release resources associated with a state structure (NULL is allowed).
//
void foobar_battery_state_free( FoobarBatteryState* self )
{
	g_free( self );
}

//
// Compare two battery state structures (NULL is allowed)/
//
gboolean foobar_battery_state_equal(
	FoobarBatteryState const* a,
	FoobarBatteryState const* b )
{
	if ( a == NULL && b == NULL ) { return TRUE; }
	if ( a == NULL || b == NULL ) { return FALSE; }
	
	if ( a->is_charging != b->is_charging ) { return FALSE; }
	if ( a->is_charged != b->is_charged ) { return FALSE; }
	if ( a->percentage != b->percentage ) { return FALSE; }
	if ( a->time_remaining != b->time_remaining ) { return FALSE; }
	
	return TRUE;
}

//
// Indicates whether the battery is currently being charged.
//
gboolean foobar_battery_state_is_charging( FoobarBatteryState const* self )
{
	g_return_val_if_fail( self != NULL, FALSE );
	return self->is_charging;
}

//
// Indicates whether the battery is currently being charged.
//
void foobar_battery_state_set_charging(
	FoobarBatteryState* self,
	gboolean            value )
{
	g_return_if_fail( self != NULL );
	self->is_charging = !!value;
}

//
// Indicates whether the battery is fully charged.
//
gboolean foobar_battery_state_is_charged( FoobarBatteryState const* self )
{
	g_return_val_if_fail( self != NULL, FALSE );
	return self->is_charged;
}

//
// Indicates whether the battery is fully charged.
//
void foobar_battery_state_set_charged(
	FoobarBatteryState* self,
	gboolean            value )
{
	g_return_if_fail( self != NULL );
	self->is_charged = !!value;
}

//
// Current battery level as a percentage value.
//
gint foobar_battery_state_get_percentage( FoobarBatteryState const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->percentage;
}

//
// Current battery level as a percentage value.
//
void foobar_battery_state_set_percentage(
	FoobarBatteryState* self,
	gint                value )
{
	g_return_if_fail( self != NULL );
	self->percentage = CLAMP( value, 0, 100 );
}

//
// Estimated remaining time until fully charged or empty in seconds.
//
gint foobar_battery_state_get_time_remaining( FoobarBatteryState const* self )
{
	g_return_val_if_fail( self != NULL, 0 );
	return self->time_remaining;
}

//
// Estimated remaining time until fully charged or empty in seconds.
//
void foobar_battery_state_set_time_remaining(
	FoobarBatteryState* self,
	gint                value )
{
	g_return_if_fail( self != NULL );
	self->time_remaining = MAX( value, 0 );
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the battery service.
//
void foobar_battery_service_class_init( FoobarBatteryServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_battery_service_get_property;
	object_klass->finalize = foobar_battery_service_finalize;

	props[PROP_STATE] = g_param_spec_boxed(
		"state",
		"State",
		"Current state of the battery, if available.",
		FOOBAR_TYPE_BATTERY_STATE,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the battery service.
//
void foobar_battery_service_init( FoobarBatteryService* self )
{
	foobar_upower_device_proxy_new_for_bus(
		G_BUS_TYPE_SYSTEM,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.freedesktop.UPower",
		"/org/freedesktop/UPower/devices/DisplayDevice",
		NULL,
		foobar_battery_service_handle_connect,
		g_object_ref( self ) );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_battery_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarBatteryService* self = (FoobarBatteryService*)object;

	switch ( prop_id )
	{
		case PROP_STATE:
			g_value_set_boxed( value, foobar_battery_service_get_state( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the battery service.
//
void foobar_battery_service_finalize( GObject* object )
{
	FoobarBatteryService* self = (FoobarBatteryService*)object;

	g_clear_signal_handler( &self->changed_handler_id, self->proxy );
	g_clear_object( &self->proxy );
	g_clear_pointer( &self->state, foobar_battery_state_free );

	G_OBJECT_CLASS( foobar_battery_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new battery service instance.
//
FoobarBatteryService* foobar_battery_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_BATTERY_SERVICE, NULL );
}

//
// Get the current state of the battery, if available.
//
FoobarBatteryState const* foobar_battery_service_get_state( FoobarBatteryService* self )
{
	g_return_val_if_fail( FOOBAR_IS_BATTERY_SERVICE( self ), NULL );

	return self->state;
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called after asynchronous initialization of the D-Bus proxy.
//
void foobar_battery_service_handle_connect(
	GObject*      object,
	GAsyncResult* result,
	gpointer      userdata )
{
	(void)object;
	g_autoptr( FoobarBatteryService ) self = (FoobarBatteryService*)userdata;

	g_autoptr( GError ) error = NULL;
	self->proxy = foobar_upower_device_proxy_new_for_bus_finish( result, &error );
	if ( self->proxy )
	{
		self->changed_handler_id = g_signal_connect(
			self->proxy,
			"g-properties-changed",
			G_CALLBACK( foobar_battery_service_handle_properties_changed ),
			self );
		foobar_battery_service_update( self );
	}
	else
	{
		g_warning( "Unable to connect to battery status service: %s", error->message );
	}
}

//
// Called by the D-Bus server when the battery state has changed.
//
void foobar_battery_service_handle_properties_changed(
	GDBusProxy* proxy,
	GVariant*   changed_properties,
	GStrv       invalidated_properties,
	gpointer    userdata )
{
	(void)proxy;
	(void)changed_properties;
	(void)invalidated_properties;
	FoobarBatteryService* self = (FoobarBatteryService*)userdata;

	foobar_battery_service_update( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Reload the battery state.
//
void foobar_battery_service_update( FoobarBatteryService* self )
{
	g_return_if_fail( FOOBAR_IS_BATTERY_SERVICE( self ) );

	FoobarBatteryState* new_state = NULL;
	if ( self->proxy && foobar_upower_device_get_is_present( self->proxy ) )
	{
		guint state = foobar_upower_device_get_state( self->proxy );
		gint percent = (gint)round( foobar_upower_device_get_percentage( self->proxy ) );
		gboolean is_charging = state == DEVICE_STATE_CHARGING;
		gboolean is_charged =
			state == DEVICE_STATE_FULLY_CHARGED ||
			( state == DEVICE_STATE_CHARGING && percent == 100 );
		gint time_remaining = is_charging
			? (gint)foobar_upower_device_get_time_to_full( self->proxy )
			: (gint)foobar_upower_device_get_time_to_empty( self->proxy );

		new_state = foobar_battery_state_new( );
		foobar_battery_state_set_charging( new_state, is_charging );
		foobar_battery_state_set_charged( new_state, is_charged );
		foobar_battery_state_set_percentage( new_state, percent );
		foobar_battery_state_set_time_remaining( new_state, time_remaining );
	}

	if ( !foobar_battery_state_equal( self->state, new_state ) )
	{
		g_clear_pointer( &self->state, foobar_battery_state_free );
		self->state = new_state;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_STATE] );
	}
}

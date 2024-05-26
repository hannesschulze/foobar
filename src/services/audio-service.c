#include "services/audio-service.h"
#include <gvc-mixer-stream.h>
#include <gvc-mixer-control.h>
#include <gvc-mixer-source.h>
#include <gvc-mixer-sink.h>
#include <gtk/gtk.h>
#include <math.h>

#define DEFAULT_VOLUME 25

//
// FoobarAudioDeviceKind:
//
// The type of an audio device (either speaker/output or microphone/input).
//

G_DEFINE_ENUM_TYPE(
	FoobarAudioDeviceKind,
	foobar_audio_device_kind, 
	G_DEFINE_ENUM_VALUE( FOOBAR_AUDIO_DEVICE_INPUT, "input" ), 
	G_DEFINE_ENUM_VALUE( FOOBAR_AUDIO_DEVICE_OUTPUT, "output" ) )

//
// FoobarAudioDevice:
//
// Representation of a real input/output device or an abstract device (like the default device which only acts as a
// proxy for another device).
//

struct _FoobarAudioDevice
{
	GObject               parent_instance;
	FoobarAudioService*   service;
	FoobarAudioDeviceKind kind;
	GvcMixerStream*       stream;
	gboolean              is_default;
	gulong                notify_handler_id;
};

enum
{
	DEVICE_PROP_KIND = 1,
	DEVICE_PROP_ID,
	DEVICE_PROP_NAME,
	DEVICE_PROP_DESCRIPTION,
	DEVICE_PROP_VOLUME,
	DEVICE_PROP_IS_MUTED,
	DEVICE_PROP_IS_DEFAULT,
	DEVICE_PROP_IS_AVAILABLE,
	N_DEVICE_PROPS,
};

static GParamSpec* device_props[N_DEVICE_PROPS] = { 0 };

static void               foobar_audio_device_class_init   ( FoobarAudioDeviceClass* klass );
static void               foobar_audio_device_init         ( FoobarAudioDevice*      self );
static void               foobar_audio_device_get_property ( GObject*                object,
                                                             guint                   prop_id,
                                                             GValue*                 value,
                                                             GParamSpec*             pspec );
static void               foobar_audio_device_set_property ( GObject*                object,
                                                             guint                   prop_id,
                                                             GValue const*           value,
                                                             GParamSpec*             pspec );
static void               foobar_audio_device_finalize     ( GObject*                object );
static FoobarAudioDevice* foobar_audio_device_new          ( FoobarAudioService*     service,
                                                             FoobarAudioDeviceKind   kind );
static void               foobar_audio_device_set_stream   ( FoobarAudioDevice*      self,
                                                             GvcMixerStream*         value );
static void               foobar_audio_device_set_default  ( FoobarAudioDevice*      self,
                                                             gboolean                value );
static void               foobar_audio_device_handle_notify( GObject*                object,
                                                             GParamSpec*             pspec,
                                                             gpointer                userdata );

G_DEFINE_FINAL_TYPE( FoobarAudioDevice, foobar_audio_device, G_TYPE_OBJECT )

//
// FoobarAudioService:
//
// Service managing the available audio input/output devices. This is implemented using the Gnome Volume Control
// library.
//

struct _FoobarAudioService
{
	GObject             parent_instance;
	GvcMixerControl*    control;
	GListStore*         devices;
	GtkSortListModel*   sorted_devices;
	GtkFilterListModel* inputs;
	GtkFilterListModel* outputs;
	FoobarAudioDevice*  default_input;
	FoobarAudioDevice*  default_output;
	gulong              default_sink_handler_id;
	gulong              default_source_handler_id;
	gulong              stream_added_handler_id;
	gulong              stream_removed_handler_id;
};

enum
{
	PROP_INPUTS = 1,
	PROP_OUTPUTS,
	PROP_DEFAULT_INPUT,
	PROP_DEFAULT_OUTPUT,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void     foobar_audio_service_class_init                   ( FoobarAudioServiceClass* klass );
static void     foobar_audio_service_init                         ( FoobarAudioService*      self );
static void     foobar_audio_service_get_property                 ( GObject*                 object,
                                                                    guint                    prop_id,
                                                                    GValue*                  value,
                                                                    GParamSpec*              pspec );
static void     foobar_audio_service_finalize                     ( GObject*                 object );
static void     foobar_audio_service_handle_default_sink_changed  ( GvcMixerControl*         control,
                                                                    guint                    id,
                                                                    gpointer                 userdata );
static void     foobar_audio_service_handle_default_source_changed( GvcMixerControl*         control,
                                                                    guint                    id,
                                                                    gpointer                 userdata );
static void     foobar_audio_service_handle_stream_added          ( GvcMixerControl*         control,
                                                                    guint                    id,
                                                                    gpointer                 userdata );
static void     foobar_audio_service_handle_stream_removed        ( GvcMixerControl*         control,
                                                                    guint                    id,
                                                                    gpointer                 userdata );
static gboolean foobar_audio_service_identify_stream              ( GvcMixerStream*          stream,
                                                                    FoobarAudioDeviceKind*   out_kind );
static void     foobar_audio_service_update_default               ( GListModel*              list,
                                                                    guint                    id );
static gint     foobar_audio_service_sort_func                    ( gconstpointer            item_a,
                                                                    gconstpointer            item_b,
                                                                    gpointer                 userdata );
static gboolean foobar_audio_service_input_filter_func            ( gpointer                 item,
                                                                    gpointer                 userdata );
static gboolean foobar_audio_service_output_filter_func           ( gpointer                 item,
                                                                    gpointer                 userdata );

G_DEFINE_FINAL_TYPE( FoobarAudioService, foobar_audio_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for audio devices.
//
void foobar_audio_device_class_init( FoobarAudioDeviceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_audio_device_get_property;
	object_klass->set_property = foobar_audio_device_set_property;
	object_klass->finalize = foobar_audio_device_finalize;

	device_props[DEVICE_PROP_KIND] = g_param_spec_enum(
		"kind",
		"Kind",
		"The kind of audio device.",
		FOOBAR_TYPE_AUDIO_DEVICE_KIND,
		FOOBAR_AUDIO_DEVICE_INPUT,
		G_PARAM_READABLE );
	device_props[DEVICE_PROP_ID] = g_param_spec_uint(
		"id",
		"ID",
		"Current numeric ID of the device (may change over time).",
		0,
		UINT_MAX,
		UINT_MAX,
		G_PARAM_READABLE );
	device_props[DEVICE_PROP_NAME] = g_param_spec_string( 
		"name", 
		"Name", 
		"Current name of the device.", 
		NULL,
		G_PARAM_READABLE );
	device_props[DEVICE_PROP_DESCRIPTION] = g_param_spec_string(
		"description",
		"Description",
		"Short textual description of the device.",
		NULL,
		G_PARAM_READABLE );
	device_props[DEVICE_PROP_VOLUME] = g_param_spec_int(
		"volume",
		"Volume",
		"Current volume in percent.",
		0,
		100,
		0,
		G_PARAM_READWRITE );
	device_props[DEVICE_PROP_IS_MUTED] = g_param_spec_int(
		"is-muted",
		"Is Muted",
		"Indicates whether the device is currently muted (i.e. the volume is 0).",
		0,
		100,
		0,
		G_PARAM_READWRITE );
	device_props[DEVICE_PROP_IS_DEFAULT] = g_param_spec_boolean(
		"is-default",
		"Is Default",
		"Indicates whether the device is currently also a default device (either microphone or speaker).",
		FALSE,
		G_PARAM_READABLE );
	device_props[DEVICE_PROP_IS_AVAILABLE] = g_param_spec_boolean(
		"is-available",
		"Is Available",
		"Indicates whether the device is currently available to use/plugged in.",
		FALSE,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_DEVICE_PROPS, device_props );
}

//
// Instance initialization for audio devices.
//
void foobar_audio_device_init( FoobarAudioDevice* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_audio_device_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarAudioDevice* self = (FoobarAudioDevice*)object;

	switch ( prop_id )
	{
		case DEVICE_PROP_KIND:
			g_value_set_enum( value, foobar_audio_device_get_kind( self ) );
			break;
		case DEVICE_PROP_ID:
			g_value_set_uint( value, foobar_audio_device_get_id( self ) );
			break;
		case DEVICE_PROP_NAME:
			g_value_set_string( value, foobar_audio_device_get_name( self ) );
			break;
		case DEVICE_PROP_DESCRIPTION:
			g_value_set_string( value, foobar_audio_device_get_description( self ) );
			break;
		case DEVICE_PROP_VOLUME:
			g_value_set_int( value, foobar_audio_device_get_volume( self ) );
			break;
		case DEVICE_PROP_IS_MUTED:
			g_value_set_boolean( value, foobar_audio_device_is_muted( self ) );
			break;
		case DEVICE_PROP_IS_DEFAULT:
			g_value_set_boolean( value, foobar_audio_device_is_default( self ) );
			break;
		case DEVICE_PROP_IS_AVAILABLE:
			g_value_set_boolean( value, foobar_audio_device_is_available( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_audio_device_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarAudioDevice* self = (FoobarAudioDevice*)object;

	switch ( prop_id )
	{
		case DEVICE_PROP_VOLUME:
			foobar_audio_device_set_volume( self, g_value_get_int( value ) );
			break;
		case DEVICE_PROP_IS_MUTED:
			foobar_audio_device_set_muted( self, g_value_get_boolean( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for audio devices.
//
void foobar_audio_device_finalize( GObject* object )
{
	FoobarAudioDevice* self = (FoobarAudioDevice*)object;

	g_clear_signal_handler( &self->notify_handler_id, self->stream );
	g_clear_object( &self->stream );

	G_OBJECT_CLASS( foobar_audio_device_parent_class )->finalize( object );
}

//
// Create a new audio device object with the specified parent service (captured as an unowned reference).
//
FoobarAudioDevice* foobar_audio_device_new(
	FoobarAudioService*   service,
	FoobarAudioDeviceKind kind )
{
	FoobarAudioDevice* self = g_object_new( FOOBAR_TYPE_AUDIO_DEVICE, NULL );
	self->service = service;
	self->kind = kind;
	return self;
}

//
// Get the type of this device.
//
FoobarAudioDeviceKind foobar_audio_device_get_kind( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), 0 );
	return self->kind;
}

//
// Get a numeric identifier for the device provided by the system.
//
guint foobar_audio_device_get_id( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), 0 );
	return self->stream ? gvc_mixer_stream_get_id( self->stream ) : UINT_MAX;
}

//
// Get the internal name for the device. This should not be used in the UI.
//
gchar const* foobar_audio_device_get_name( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), NULL );
	return self->stream ? gvc_mixer_stream_get_name( self->stream ) : NULL;
}

//
// Get a human-readable description for the device.
//
gchar const* foobar_audio_device_get_description( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), NULL );
	return self->stream ? gvc_mixer_stream_get_description( self->stream ) : NULL;
}

//
// Get the current volume as a percentage value. If muted, this is 0.
//
gint foobar_audio_device_get_volume( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), 0 );

	if ( !self->service ) { return 0; }
	if ( self->stream && gvc_mixer_stream_get_is_muted( self->stream ) ) { return 0; }
	gdouble max_volume = gvc_mixer_control_get_vol_max_norm( self->service->control );
	gdouble cur_volume = self->stream ? gvc_mixer_stream_get_volume( self->stream ) : 0;
	gint percentage = (gint)round( 100. * cur_volume / max_volume );
	return CLAMP( percentage, 0, 100 );
}

//
// Check whether the device is currently muted, i.e. its volume is 0.
//
gboolean foobar_audio_device_is_muted( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), FALSE );
	return foobar_audio_device_get_volume( self ) == 0;
}

//
// Check whether this device is the system-default for its kind.
//
// This will always return TRUE for the static default device instance.
//
gboolean foobar_audio_device_is_default( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), FALSE );
	return self->is_default;
}

//
// Check whether the device is currently available.
//
gboolean foobar_audio_device_is_available( FoobarAudioDevice* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ), FALSE );
	return self->stream != NULL;
}

//
// Update the stream backing this device object.
//
void foobar_audio_device_set_stream(
	FoobarAudioDevice* self,
	GvcMixerStream*    value )
{
	g_return_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ) );

	if ( self->stream != value )
	{
		g_clear_signal_handler( &self->notify_handler_id, self->stream );
		g_clear_object( &self->stream );

		if ( value )
		{
			self->stream = g_object_ref( value );
			self->notify_handler_id = g_signal_connect(
				self->stream,
				"notify",
				G_CALLBACK( foobar_audio_device_handle_notify ),
				self );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_ID] );
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_NAME] );
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_DESCRIPTION] );
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_VOLUME] );
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_IS_MUTED] );
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_IS_AVAILABLE] );
	}
}

//
// Update the device's "default" flag. This does not actually make it the default device.
//
void foobar_audio_device_set_default(
	FoobarAudioDevice* self,
	gboolean           value )
{
	g_return_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ) );

	value = !!value;
	if ( self->is_default != value )
	{
		self->is_default = value;
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_IS_DEFAULT] );
	}
}

//
// Update the device's volume as a percentage value.
//
void foobar_audio_device_set_volume(
	FoobarAudioDevice* self,
	gint               value )
{
	g_return_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ) );

	if ( !self->service || !self->stream ) { return; }

	value = CLAMP( value, 0, 100 );
	if ( foobar_audio_device_get_volume( self ) != value )
	{
		gdouble max_volume = gvc_mixer_control_get_vol_max_norm( self->service->control );
		gdouble new_volume = value * max_volume / 100.;
		gvc_mixer_stream_set_volume( self->stream, (guint32)new_volume );
		gvc_mixer_stream_push_volume( self->stream );
		gvc_mixer_stream_set_is_muted( self->stream, value == 0 );
		gvc_mixer_stream_change_is_muted( self->stream, value == 0 );
	}
}

//
// Update the device's "muted" state.
//
// If the device was previously muted, the volume is set to either the previous volume or to a default value.
//
void foobar_audio_device_set_muted(
	FoobarAudioDevice* self,
	gboolean           value )
{
	g_return_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ) );

	if ( foobar_audio_device_is_muted( self ) != value )
	{
		if ( self->stream )
		{
			gvc_mixer_stream_set_is_muted( self->stream, value );
			gvc_mixer_stream_change_is_muted( self->stream, value );
		}

		if ( !value && foobar_audio_device_get_volume( self ) == 0 )
		{
			foobar_audio_device_set_volume( self, DEFAULT_VOLUME );
		}
	}
}

//
// Let this device become default one for its kind.
//
void foobar_audio_device_make_default( FoobarAudioDevice* self )
{
	g_return_if_fail( FOOBAR_IS_AUDIO_DEVICE( self ) );

	if ( !self->service || !self->stream ) { return; }

	if ( !foobar_audio_device_is_default( self ) )
	{
		switch ( self->kind )
		{
			case FOOBAR_AUDIO_DEVICE_INPUT:
				gvc_mixer_control_set_default_source( self->service->control, self->stream );
				break;
			case FOOBAR_AUDIO_DEVICE_OUTPUT:
				gvc_mixer_control_set_default_sink( self->service->control, self->stream );
				break;
			default:
				break;
		}
	}
}

//
// Called by the underlying stream when one of its properties changes.
//
void foobar_audio_device_handle_notify(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	FoobarAudioDevice* self = (FoobarAudioDevice*)userdata;

	gchar const* property = g_param_spec_get_name( pspec );
	if ( !g_strcmp0( property, "id" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_ID] );
	}
	else if ( !g_strcmp0( property, "name" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_NAME] );
	}
	else if ( !g_strcmp0( property, "description" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_DESCRIPTION] );
	}
	else if ( !g_strcmp0( property, "volume" ) || !g_strcmp0( property, "is-muted" ) )
	{
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_VOLUME] );
		g_object_notify_by_pspec( G_OBJECT( self ), device_props[DEVICE_PROP_IS_MUTED] );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the audio service.
//
void foobar_audio_service_class_init( FoobarAudioServiceClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_audio_service_get_property;
	object_klass->finalize = foobar_audio_service_finalize;

	props[PROP_INPUTS] = g_param_spec_object(
		"inputs",
		"Inputs",
		"Sorted list of input devices.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	props[PROP_OUTPUTS] = g_param_spec_object(
		"outputs",
		"Outputs",
		"Sorted list of output devices.",
		G_TYPE_LIST_MODEL,
		G_PARAM_READABLE );
	props[PROP_DEFAULT_INPUT] = g_param_spec_object(
		"default-input",
		"Default Input",
		"The default input device (reference remains constant).",
		FOOBAR_TYPE_AUDIO_DEVICE,
		G_PARAM_READABLE );
	props[PROP_DEFAULT_OUTPUT] = g_param_spec_object(
		"default-output",
		"Default Output",
		"The default output device (reference remains constant).",
		FOOBAR_TYPE_AUDIO_DEVICE,
		G_PARAM_READABLE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for the audio service.
//
void foobar_audio_service_init( FoobarAudioService* self )
{
	self->control = gvc_mixer_control_new( "Foobar mixer control" );
	self->devices = g_list_store_new( FOOBAR_TYPE_AUDIO_DEVICE );

	GtkCustomSorter* sorter = gtk_custom_sorter_new( foobar_audio_service_sort_func, NULL, NULL );
	self->sorted_devices = gtk_sort_list_model_new(
		G_LIST_MODEL( g_object_ref( self->devices ) ),
		GTK_SORTER( sorter ) );

	GtkCustomFilter* input_filter = gtk_custom_filter_new( foobar_audio_service_input_filter_func, NULL, NULL );
	self->inputs = gtk_filter_list_model_new(
		G_LIST_MODEL( g_object_ref( self->sorted_devices ) ),
		GTK_FILTER( input_filter ) );

	GtkCustomFilter* output_filter = gtk_custom_filter_new( foobar_audio_service_output_filter_func, NULL, NULL );
	self->outputs = gtk_filter_list_model_new(
		G_LIST_MODEL( g_object_ref( self->sorted_devices ) ),
		GTK_FILTER( output_filter ) );

	self->default_input = foobar_audio_device_new( self, FOOBAR_AUDIO_DEVICE_INPUT );
	self->default_output = foobar_audio_device_new( self, FOOBAR_AUDIO_DEVICE_OUTPUT );
	foobar_audio_device_set_default( self->default_input, TRUE );
	foobar_audio_device_set_default( self->default_output, TRUE );

	self->default_sink_handler_id = g_signal_connect(
		self->control,
		"default-sink-changed",
		G_CALLBACK( foobar_audio_service_handle_default_sink_changed ),
		self );
	self->default_source_handler_id = g_signal_connect(
		self->control,
		"default-source-changed",
		G_CALLBACK( foobar_audio_service_handle_default_source_changed ),
		self );
	self->stream_added_handler_id = g_signal_connect(
		self->control,
		"stream-added",
		G_CALLBACK( foobar_audio_service_handle_stream_added ),
		self );
	self->stream_removed_handler_id = g_signal_connect(
		self->control,
		"stream-removed",
		G_CALLBACK( foobar_audio_service_handle_stream_removed ),
		self );

	gvc_mixer_control_open( self->control );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_audio_service_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarAudioService* self = (FoobarAudioService*)object;

	switch ( prop_id )
	{
		case PROP_INPUTS:
			g_value_set_object( value, foobar_audio_service_get_inputs( self ) );
			break;
		case PROP_OUTPUTS:
			g_value_set_object( value, foobar_audio_service_get_outputs( self ) );
			break;
		case PROP_DEFAULT_INPUT:
			g_value_set_object( value, foobar_audio_service_get_default_input( self ) );
			break;
		case PROP_DEFAULT_OUTPUT:
			g_value_set_object( value, foobar_audio_service_get_default_output( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for the audio service.
//
void foobar_audio_service_finalize( GObject* object )
{
	FoobarAudioService* self = (FoobarAudioService*)object;

	g_clear_signal_handler( &self->default_sink_handler_id, self->control );
	g_clear_signal_handler( &self->default_source_handler_id, self->control );
	g_clear_signal_handler( &self->stream_added_handler_id, self->control );
	g_clear_signal_handler( &self->stream_removed_handler_id, self->control );

	gvc_mixer_control_close( self->control );

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->devices ) ); ++i )
	{
		FoobarAudioDevice* device = g_list_model_get_item( G_LIST_MODEL( self->devices ), i );
		device->service = NULL;
	}

	self->default_input->service = NULL;
	self->default_output->service = NULL;

	g_clear_object( &self->control );
	g_clear_object( &self->inputs );
	g_clear_object( &self->outputs );
	g_clear_object( &self->sorted_devices );
	g_clear_object( &self->devices );
	g_clear_object( &self->default_input );
	g_clear_object( &self->default_output );

	G_OBJECT_CLASS( foobar_audio_service_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new audio service instance.
//
FoobarAudioService* foobar_audio_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_AUDIO_SERVICE, NULL );
}

//
// Get the default input device. This reference remains constant.
//
FoobarAudioDevice* foobar_audio_service_get_default_input( FoobarAudioService* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( self ), NULL );
	return self->default_input;
}

//
// Get the default output device. This reference remains constant.
//
FoobarAudioDevice* foobar_audio_service_get_default_output( FoobarAudioService* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( self ), NULL );
	return self->default_output;
}

//
// Get a sorted list of input devices.
//
GListModel* foobar_audio_service_get_inputs( FoobarAudioService* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->inputs );
}

//
// Get a sorted list of output devices.
//
GListModel* foobar_audio_service_get_outputs( FoobarAudioService* self )
{
	g_return_val_if_fail( FOOBAR_IS_AUDIO_SERVICE( self ), NULL );
	return G_LIST_MODEL( self->outputs );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called by the mixer control when the default output device has changed.
//
// This will update the "default" state of the device and set the backing stream for the static default device.
//
void foobar_audio_service_handle_default_sink_changed(
	GvcMixerControl* control,
	guint            id,
	gpointer         userdata )
{
	(void)control;
	FoobarAudioService* self = (FoobarAudioService*)userdata;

	foobar_audio_service_update_default( G_LIST_MODEL( self->outputs ), id );

	GvcMixerStream* stream = gvc_mixer_control_lookup_stream_id( self->control, id );
	foobar_audio_device_set_stream( self->default_output, stream );
}

//
// Called by the mixer control when the default input device has changed.
//
// This will update the "default" state of the device and set the backing stream for the static default device.
//
void foobar_audio_service_handle_default_source_changed(
	GvcMixerControl* control,
	guint            id,
	gpointer         userdata )
{
	(void)control;
	FoobarAudioService* self = (FoobarAudioService*)userdata;

	foobar_audio_service_update_default( G_LIST_MODEL( self->inputs ), id );

	GvcMixerStream* stream = gvc_mixer_control_lookup_stream_id( self->control, id );
	foobar_audio_device_set_stream( self->default_input, stream );
}

//
// Called when an audio device was added.
//
void foobar_audio_service_handle_stream_added(
	GvcMixerControl* control,
	guint            id,
	gpointer         userdata )
{
	(void)control;
	FoobarAudioService* self = (FoobarAudioService*)userdata;

	GvcMixerStream*       stream = gvc_mixer_control_lookup_stream_id( self->control, id );
	FoobarAudioDeviceKind kind;
	if ( foobar_audio_service_identify_stream( stream, &kind ) )
	{
		g_autoptr( FoobarAudioDevice ) device = foobar_audio_device_new( self, kind );
		foobar_audio_device_set_stream( device, stream );
		g_list_store_append( self->devices, device );
	}
}

//
// Called when an audio device was removed.
//
void foobar_audio_service_handle_stream_removed(
	GvcMixerControl* control,
	guint            id,
	gpointer         userdata )
{
	(void)control;
	FoobarAudioService* self = (FoobarAudioService*)userdata;

	for ( guint i = 0; i < g_list_model_get_n_items( G_LIST_MODEL( self->devices ) ); ++i )
	{
		FoobarAudioDevice* device = g_list_model_get_item( G_LIST_MODEL( self->devices ), i );
		if ( foobar_audio_device_get_id( device ) == id )
		{
			g_object_ref( device );
			device->service = NULL;
			g_list_store_remove( self->devices, i );
			foobar_audio_device_set_stream( device, NULL );
			g_object_unref( device );
			break;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Identify whether a stream is an input device or an output device, returning FALSE if unknown.
//
gboolean foobar_audio_service_identify_stream(
	GvcMixerStream*        stream,
	FoobarAudioDeviceKind* out_kind )
{
	if ( GVC_IS_MIXER_SINK( stream ) )
	{
		*out_kind = FOOBAR_AUDIO_DEVICE_OUTPUT;
		return TRUE;
	}

	if ( GVC_IS_MIXER_SOURCE( stream ) )
	{
		*out_kind = FOOBAR_AUDIO_DEVICE_INPUT;
		return TRUE;
	}

	return FALSE;
}

//
// Set the "default" state for the device with the specified ID to TRUE and all other devices to FALSE.
//
void foobar_audio_service_update_default(
	GListModel* list,
	guint       id )
{
	for ( guint i = 0; i < g_list_model_get_n_items( list ); ++i )
	{
		FoobarAudioDevice* device = g_list_model_get_item( list, i );
		gboolean is_default = foobar_audio_device_get_id( device ) == id;
		foobar_audio_device_set_default( device, is_default );
	}
}

//
// Sorting callback for audio devices.
//
gint foobar_audio_service_sort_func(
	gconstpointer item_a,
	gconstpointer item_b,
	gpointer      userdata )
{
	(void)userdata;

	FoobarAudioDevice* device_a = (FoobarAudioDevice*)item_a;
	FoobarAudioDevice* device_b = (FoobarAudioDevice*)item_b;
	return g_strcmp0(
		foobar_audio_device_get_description( device_a ),
		foobar_audio_device_get_description( device_b ) );
}

//
// Filtering callback for the list of input devices.
//
gboolean foobar_audio_service_input_filter_func(
	gpointer item,
	gpointer userdata )
{
	(void)userdata;

	FoobarAudioDevice* device = item;
	return foobar_audio_device_get_kind( device ) == FOOBAR_AUDIO_DEVICE_INPUT;
}

//
// Filtering callback for the list of output devices.
//
gboolean foobar_audio_service_output_filter_func(
	gpointer item,
	gpointer userdata )
{
	(void)userdata;

	FoobarAudioDevice* device = item;
	return foobar_audio_device_get_kind( device ) == FOOBAR_AUDIO_DEVICE_OUTPUT;
}

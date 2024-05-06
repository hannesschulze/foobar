#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_AUDIO_DEVICE_KIND foobar_audio_device_kind_get_type( )
#define FOOBAR_TYPE_AUDIO_DEVICE      foobar_audio_device_get_type( )
#define FOOBAR_TYPE_AUDIO_SERVICE     foobar_audio_service_get_type( )

typedef enum
{
	FOOBAR_AUDIO_DEVICE_INPUT = 0,
	FOOBAR_AUDIO_DEVICE_OUTPUT,
} FoobarAudioDeviceKind;

GType foobar_audio_device_kind_get_type( void );

G_DECLARE_FINAL_TYPE( FoobarAudioDevice, foobar_audio_device, FOOBAR, AUDIO_DEVICE, GObject )

FoobarAudioDeviceKind foobar_audio_device_get_kind       ( FoobarAudioDevice* self );
guint                 foobar_audio_device_get_id         ( FoobarAudioDevice* self );
gchar const*          foobar_audio_device_get_name       ( FoobarAudioDevice* self );
gchar const*          foobar_audio_device_get_description( FoobarAudioDevice* self );
gint                  foobar_audio_device_get_volume     ( FoobarAudioDevice* self );
gboolean              foobar_audio_device_is_muted       ( FoobarAudioDevice* self );
gboolean              foobar_audio_device_is_default     ( FoobarAudioDevice* self );
gboolean              foobar_audio_device_is_available   ( FoobarAudioDevice* self );
void                  foobar_audio_device_set_volume     ( FoobarAudioDevice* self,
                                                           gint               value );
void                  foobar_audio_device_set_muted      ( FoobarAudioDevice* self,
                                                           gboolean           value );
void                  foobar_audio_device_make_default   ( FoobarAudioDevice* self );

G_DECLARE_FINAL_TYPE( FoobarAudioService, foobar_audio_service, FOOBAR, AUDIO_SERVICE, GObject )

FoobarAudioService* foobar_audio_service_new               ( void );
FoobarAudioDevice*  foobar_audio_service_get_default_input ( FoobarAudioService* self );
FoobarAudioDevice*  foobar_audio_service_get_default_output( FoobarAudioService* self );
GListModel*         foobar_audio_service_get_inputs        ( FoobarAudioService* self );
GListModel*         foobar_audio_service_get_outputs       ( FoobarAudioService* self );

G_END_DECLS
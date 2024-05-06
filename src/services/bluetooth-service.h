#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_BLUETOOTH_DEVICE_STATE foobar_bluetooth_device_state_get_type( )
#define FOOBAR_TYPE_BLUETOOTH_DEVICE       foobar_bluetooth_device_get_type( )
#define FOOBAR_TYPE_BLUETOOTH_ADAPTER      foobar_bluetooth_adapter_get_type( )
#define FOOBAR_TYPE_BLUETOOTH_SERVICE      foobar_bluetooth_service_get_type( )

typedef enum
{
	FOOBAR_BLUETOOTH_DEVICE_STATE_DISCONNECTED,
	FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTING,
	FOOBAR_BLUETOOTH_DEVICE_STATE_CONNECTED,
} FoobarBluetoothDeviceState;

GType foobar_bluetooth_device_state_get_type( void );

G_DECLARE_FINAL_TYPE( FoobarBluetoothDevice, foobar_bluetooth_device, FOOBAR, BLUETOOTH_DEVICE, GObject )

gchar const*               foobar_bluetooth_device_get_name         ( FoobarBluetoothDevice* self );
FoobarBluetoothDeviceState foobar_bluetooth_device_get_state        ( FoobarBluetoothDevice* self );
void                       foobar_bluetooth_device_toggle_connection( FoobarBluetoothDevice* self );

G_DECLARE_FINAL_TYPE( FoobarBluetoothService, foobar_bluetooth_service, FOOBAR, BLUETOOTH_SERVICE, GObject )

FoobarBluetoothService* foobar_bluetooth_service_new                  ( void );
GListModel*             foobar_bluetooth_service_get_devices          ( FoobarBluetoothService* self );
GListModel*             foobar_bluetooth_service_get_connected_devices( FoobarBluetoothService* self );
gboolean                foobar_bluetooth_service_is_available         ( FoobarBluetoothService* self );
gboolean                foobar_bluetooth_service_is_enabled           ( FoobarBluetoothService* self );
gboolean                foobar_bluetooth_service_is_scanning          ( FoobarBluetoothService* self );
void                    foobar_bluetooth_service_set_enabled          ( FoobarBluetoothService* self,
                                                                        gboolean                value );
void                    foobar_bluetooth_service_set_scanning         ( FoobarBluetoothService* self,
                                                                        gboolean                value );

G_END_DECLS
#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_NETWORK               foobar_network_get_type( )
#define FOOBAR_TYPE_NETWORK_ADAPTER_STATE foobar_network_adapter_state_get_type( )
#define FOOBAR_TYPE_NETWORK_ADAPTER       foobar_network_adapter_get_type( )
#define FOOBAR_TYPE_NETWORK_ADAPTER_WIRED foobar_network_adapter_wired_get_type( )
#define FOOBAR_TYPE_NETWORK_ADAPTER_WIFI  foobar_network_adapter_wifi_get_type( )
#define FOOBAR_TYPE_NETWORK_SERVICE       foobar_network_service_get_type( )

G_DECLARE_FINAL_TYPE( FoobarNetwork, foobar_network, FOOBAR, NETWORK, GObject )

gchar const* foobar_network_get_name    ( FoobarNetwork* self );
gint         foobar_network_get_strength( FoobarNetwork* self );
gboolean     foobar_network_is_active   ( FoobarNetwork* self );

typedef enum
{
	FOOBAR_NETWORK_ADAPTER_STATE_DISCONNECTED,
	FOOBAR_NETWORK_ADAPTER_STATE_CONNECTING,
	FOOBAR_NETWORK_ADAPTER_STATE_CONNECTED,
	FOOBAR_NETWORK_ADAPTER_STATE_LIMITED,
} FoobarNetworkAdapterState;

GType foobar_network_adapter_state_get_type( void );

G_DECLARE_DERIVABLE_TYPE( FoobarNetworkAdapter, foobar_network_adapter, FOOBAR, NETWORK_ADAPTER, GObject )

struct _FoobarNetworkAdapterClass
{
	GObjectClass parent_class;
};

FoobarNetworkAdapterState foobar_network_adapter_get_state( FoobarNetworkAdapter* self );

G_DECLARE_FINAL_TYPE( FoobarNetworkAdapterWired, foobar_network_adapter_wired, FOOBAR, NETWORK_ADAPTER_WIRED, FoobarNetworkAdapter )

gint64 foobar_network_adapter_wired_get_speed( FoobarNetworkAdapterWired* self );

G_DECLARE_FINAL_TYPE( FoobarNetworkAdapterWifi, foobar_network_adapter_wifi, FOOBAR, NETWORK_ADAPTER_WIFI, FoobarNetworkAdapter )

GListModel*    foobar_network_adapter_wifi_get_networks( FoobarNetworkAdapterWifi* self );
FoobarNetwork* foobar_network_adapter_wifi_get_active  ( FoobarNetworkAdapterWifi* self );
gboolean       foobar_network_adapter_wifi_is_enabled  ( FoobarNetworkAdapterWifi* self );
gboolean       foobar_network_adapter_wifi_is_scanning ( FoobarNetworkAdapterWifi* self );
void           foobar_network_adapter_wifi_set_enabled ( FoobarNetworkAdapterWifi* self,
                                                         gboolean                  value );
void           foobar_network_adapter_wifi_set_scanning( FoobarNetworkAdapterWifi* self,
                                                         gboolean                  value );

G_DECLARE_FINAL_TYPE( FoobarNetworkService, foobar_network_service, FOOBAR, NETWORK_SERVICE, GObject )

FoobarNetworkService*      foobar_network_service_new       ( void );
FoobarNetworkAdapterWired* foobar_network_service_get_wired ( FoobarNetworkService* self );
FoobarNetworkAdapterWifi*  foobar_network_service_get_wifi  ( FoobarNetworkService* self );
FoobarNetworkAdapter*      foobar_network_service_get_active( FoobarNetworkService* self );

G_END_DECLS

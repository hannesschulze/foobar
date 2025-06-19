#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_LAUNCHER_ITEM foobar_launcher_item_get_type( )

G_DECLARE_INTERFACE( FoobarLauncherItem, foobar_launcher_item, FOOBAR, LAUNCHER_ITEM, GObject )

struct _FoobarLauncherItemInterface
{
	GTypeInterface parent_interface;

	gchar const* ( *get_title )      ( FoobarLauncherItem* self );
	gchar const* ( *get_description )( FoobarLauncherItem* self );
	GIcon*       ( *get_icon )       ( FoobarLauncherItem* self );
	void         ( *activate )       ( FoobarLauncherItem* self );
};

gchar const* foobar_launcher_item_get_title      ( FoobarLauncherItem* self );
gchar const* foobar_launcher_item_get_description( FoobarLauncherItem* self );
GIcon*       foobar_launcher_item_get_icon       ( FoobarLauncherItem* self );
void         foobar_launcher_item_activate       ( FoobarLauncherItem* self );

G_END_DECLS


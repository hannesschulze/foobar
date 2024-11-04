#include "launcher-item.h"

static void foobar_launcher_item_default_init( FoobarLauncherItemInterface* iface );

G_DEFINE_INTERFACE( FoobarLauncherItem, foobar_launcher_item, G_TYPE_OBJECT )

//
// Static initialization for the launcher item interface.
//
void foobar_launcher_item_default_init( FoobarLauncherItemInterface* iface )
{
	g_object_interface_install_property( iface, g_param_spec_string(
		"title",
		"Title",
		"The title text displayed in the launcher.",
		NULL,
		G_PARAM_READABLE ) );
	g_object_interface_install_property( iface, g_param_spec_string(
		"description",
		"Description",
		"Text displayed below the title.",
		NULL,
		G_PARAM_READABLE ) );
	g_object_interface_install_property( iface, g_param_spec_object(
		"icon",
		"Icon",
		"Icon to show for the item.",
		G_TYPE_ICON,
		G_PARAM_READABLE ) );
}

//
// Get the title text displayed in the launcher.
//
gchar const* foobar_launcher_item_get_title( FoobarLauncherItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_LAUNCHER_ITEM( self ), NULL );
	FoobarLauncherItemInterface* iface = FOOBAR_LAUNCHER_ITEM_GET_IFACE( self );
	g_return_val_if_fail( iface->get_title != NULL, NULL );
	return iface->get_title( self );
}

//
// Get the text displayed below the title.
//
gchar const* foobar_launcher_item_get_description( FoobarLauncherItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_LAUNCHER_ITEM( self ), NULL );
	FoobarLauncherItemInterface* iface = FOOBAR_LAUNCHER_ITEM_GET_IFACE( self );
	g_return_val_if_fail( iface->get_description != NULL, NULL );
	return iface->get_description( self );
}

//
// Get the icon to show for the item.
//
GIcon* foobar_launcher_item_get_icon( FoobarLauncherItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_LAUNCHER_ITEM( self ), NULL );
	FoobarLauncherItemInterface* iface = FOOBAR_LAUNCHER_ITEM_GET_IFACE( self );
	g_return_val_if_fail( iface->get_icon != NULL, NULL );
	return iface->get_icon( self );
}

//
// Activate the item, e.g. by launching the application.
//
void foobar_launcher_item_activate( FoobarLauncherItem* self )
{
	g_return_if_fail( FOOBAR_IS_LAUNCHER_ITEM( self ) );
	FoobarLauncherItemInterface* iface = FOOBAR_LAUNCHER_ITEM_GET_IFACE( self );
	if ( iface->activate ) { iface->activate( self ); }
}

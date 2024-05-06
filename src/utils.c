#include "utils.h"
#include <gio/gio.h>

//
// Return the path for a file in the user's XDG cache directory. The result should be freed using g_free.
//
// This will also ensure that the application's cache directory exists.
//
gchar* foobar_get_cache_path( gchar const* filename )
{
	g_autoptr( GError ) error = NULL;
	g_autofree gchar* directory_path = g_strdup_printf( "%s/foobar", g_get_user_cache_dir( ) );

	if ( !g_file_test( directory_path, G_FILE_TEST_EXISTS ) )
	{
		g_autoptr( GFile ) cache_dir = g_file_new_for_path( directory_path );
		if ( !g_file_make_directory_with_parents( cache_dir, NULL, &error ) )
		{
			g_warning( "Unable to create cache directory: %s", error->message );
			return NULL;
		}
	}

	return g_strdup_printf( "%s/%s", directory_path, filename );
}
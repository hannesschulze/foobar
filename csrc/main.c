#include "application.h"

//
// Entry point of the application, runs the GTK app.
//
int main(
	int    argc,
	char** argv )
{
	g_autoptr( FoobarApplication ) app = foobar_application_new( );
	return g_application_run( G_APPLICATION( app ), argc, argv );
}

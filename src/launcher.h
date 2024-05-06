#pragma once

#include <gtk/gtk.h>
#include "services/application-service.h"
#include "services/configuration-service.h"

G_BEGIN_DECLS

#define FOOBAR_TYPE_LAUNCHER foobar_launcher_get_type( )

G_DECLARE_FINAL_TYPE( FoobarLauncher, foobar_launcher, FOOBAR, LAUNCHER, GtkWindow )

FoobarLauncher* foobar_launcher_new                ( FoobarApplicationService*          application_service,
                                                     FoobarConfigurationService*        configuration_service );
void            foobar_launcher_apply_configuration( FoobarLauncher*                    self,
                                                     FoobarLauncherConfiguration const* config );

G_END_DECLS
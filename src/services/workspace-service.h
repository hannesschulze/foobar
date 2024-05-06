#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_WORKSPACE_FLAGS   foobar_workspace_flags_get_type( )
#define FOOBAR_TYPE_WORKSPACE         foobar_workspace_get_type( )
#define FOOBAR_TYPE_WORKSPACE_SERVICE foobar_workspace_service_get_type( )

typedef enum
{
	FOOBAR_WORKSPACE_FLAGS_NONE       = 0,
	// Indicates that the workspace is a globally active one (i.e. visible on the focused monitor).
	FOOBAR_WORKSPACE_FLAGS_ACTIVE     = 1 << 0,
	// Indicates that the workspace is currently visible.
	FOOBAR_WORKSPACE_FLAGS_VISIBLE    = 1 << 1,
	// Indicates that the workspace is a special workspace.
	FOOBAR_WORKSPACE_FLAGS_SPECIAL    = 1 << 2,
	// Indicates that the workspace is persistent (i.e. configured in the config file).
	FOOBAR_WORKSPACE_FLAGS_PERSISTENT = 1 << 3,
	// Indicates that the workspace requires an urgent action (e.g. a notification).
	FOOBAR_WORKSPACE_FLAGS_URGENT     = 1 << 4,
} FoobarWorkspaceFlags;

GType foobar_workspace_flags_get_type( void );

G_DECLARE_FINAL_TYPE( FoobarWorkspace, foobar_workspace, FOOBAR, WORKSPACE, GObject )

gint64               foobar_workspace_get_id     ( FoobarWorkspace* self );
gchar const*         foobar_workspace_get_name   ( FoobarWorkspace* self );
gchar const*         foobar_workspace_get_monitor( FoobarWorkspace* self );
FoobarWorkspaceFlags foobar_workspace_get_flags  ( FoobarWorkspace* self );
void                 foobar_workspace_activate   ( FoobarWorkspace* self );

G_DECLARE_FINAL_TYPE( FoobarWorkspaceService, foobar_workspace_service, FOOBAR, WORKSPACE_SERVICE, GObject )

FoobarWorkspaceService* foobar_workspace_service_new           ( void );
GListModel*             foobar_workspace_service_get_workspaces( FoobarWorkspaceService* self );

G_END_DECLS
#include "widgets/panel/panel-item-workspaces.h"
#include "widgets/inset-container.h"

//
// FoobarPanelItemWorkspaces:
//
// A panel item showing all workspaces for the current monitor (if multi-monitor mode is enabled) as buttons, indicating
// the current state of the workspace (i.e., whether it's active, visible, special, etc.).
//

struct _FoobarPanelItemWorkspaces
{
	FoobarPanelItem         parent_instance;
	GtkWidget*              list_view;
	GtkWidget*              inset_container;
	FoobarWorkspaceService* workspace_service;
	gint                    button_size;
	gint                    spacing;
	GtkFilter*              monitor_filter;
	gulong                  monitor_configuration_handler_id;
};

enum
{
	PROP_ORIENTATION = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void     foobar_panel_item_workspaces_class_init                          ( FoobarPanelItemWorkspacesClass* klass );
static void     foobar_panel_item_workspaces_init                                ( FoobarPanelItemWorkspaces*      self );
static void     foobar_panel_item_workspaces_get_property                        ( GObject*                        object,
                                                                                   guint                           prop_id,
                                                                                   GValue*                         value,
                                                                                   GParamSpec*                     pspec );
static void     foobar_panel_item_workspaces_set_property                        ( GObject*                        object,
                                                                                   guint                           prop_id,
                                                                                   GValue const*                   value,
                                                                                   GParamSpec*                     pspec );
static void     foobar_panel_item_workspaces_finalize                            ( GObject*                        object );
static void     foobar_panel_item_workspaces_handle_item_setup                   ( GtkListItemFactory*             factory,
                                                                                   GtkListItem*                    list_item,
                                                                                   gpointer                        userdata );
static void     foobar_panel_item_workspaces_handle_item_clicked                 ( GtkButton*                      button,
                                                                                   gpointer                        userdata );
static void     foobar_panel_item_workspaces_handle_monitor_configuration_changed( FoobarWorkspaceService*         service,
                                                                                   gpointer                        userdata );
static gchar**  foobar_panel_item_workspaces_compute_css_classes                 ( GtkExpression*                  expression,
                                                                                   FoobarWorkspaceFlags            flags,
                                                                                   gpointer                        userdata );
static gboolean foobar_panel_item_workspaces_monitor_filter_func                 ( gpointer                        item,
                                                                                   gpointer                        userdata );

G_DEFINE_FINAL_TYPE_WITH_CODE(
	FoobarPanelItemWorkspaces,
	foobar_panel_item_workspaces,
	FOOBAR_TYPE_PANEL_ITEM,
	G_IMPLEMENT_INTERFACE( GTK_TYPE_ORIENTABLE, NULL ) )

// ---------------------------------------------------------------------------------------------------------------------
// Panel Item Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for workspace panel items.
//
void foobar_panel_item_workspaces_class_init( FoobarPanelItemWorkspacesClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_panel_item_workspaces_get_property;
	object_klass->set_property = foobar_panel_item_workspaces_set_property;
	object_klass->finalize = foobar_panel_item_workspaces_finalize;

	gpointer orientable_iface = g_type_default_interface_peek( GTK_TYPE_ORIENTABLE );
	props[PROP_ORIENTATION] = g_param_spec_override(
		"orientation",
		g_object_interface_find_property( orientable_iface, "orientation" ) );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for workspace panel items.
//
void foobar_panel_item_workspaces_init( FoobarPanelItemWorkspaces* self )
{
	GtkListItemFactory* item_factory = gtk_signal_list_item_factory_new( );
	g_signal_connect( item_factory, "setup", G_CALLBACK( foobar_panel_item_workspaces_handle_item_setup ), self );

	self->list_view = gtk_list_view_new( NULL, item_factory );
	gtk_widget_set_halign( self->list_view, GTK_ALIGN_CENTER );
	gtk_widget_set_valign( self->list_view, GTK_ALIGN_CENTER );
	gtk_list_view_set_single_click_activate( GTK_LIST_VIEW( self->list_view ), TRUE );

	self->inset_container = foobar_inset_container_new( );
	foobar_inset_container_set_child( FOOBAR_INSET_CONTAINER( self->inset_container ), GTK_WIDGET( self->list_view ) );

	gtk_widget_add_css_class( GTK_WIDGET( self ), "workspaces" );
	foobar_panel_item_set_child( FOOBAR_PANEL_ITEM( self ), GTK_WIDGET( self->inset_container ) );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_panel_item_workspaces_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarPanelItemWorkspaces* self = (FoobarPanelItemWorkspaces*)object;

	switch ( prop_id )
	{
		case PROP_ORIENTATION:
			g_value_set_enum( value, gtk_orientable_get_orientation( GTK_ORIENTABLE( self->list_view ) ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_panel_item_workspaces_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarPanelItemWorkspaces* self = (FoobarPanelItemWorkspaces*)object;

	switch ( prop_id )
	{
		case PROP_ORIENTATION:
			gtk_orientable_set_orientation( GTK_ORIENTABLE( self->list_view ), g_value_get_enum( value ) );
			g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_ORIENTATION] );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for workspace panel items.
//
void foobar_panel_item_workspaces_finalize( GObject* object )
{
	FoobarPanelItemWorkspaces* self = (FoobarPanelItemWorkspaces*)object;

	g_clear_signal_handler( &self->monitor_configuration_handler_id, self->workspace_service );
	g_clear_object( &self->workspace_service );
	g_clear_object( &self->monitor_filter );

	G_OBJECT_CLASS( foobar_panel_item_workspaces_parent_class )->finalize( object );
}


// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new workspaces panel item with the given configuration.
//
// If monitor is not NULL, only workspaces on this monitor will be shown.
//
FoobarPanelItem* foobar_panel_item_workspaces_new(
	FoobarPanelItemConfiguration const* config,
	GdkMonitor*                         monitor,
	FoobarWorkspaceService*             workspace_service )
{
	g_return_val_if_fail( config != NULL, NULL );
	g_return_val_if_fail( foobar_panel_item_configuration_get_kind( config ) == FOOBAR_PANEL_ITEM_KIND_WORKSPACES, NULL );
	g_return_val_if_fail( monitor == NULL || GDK_IS_MONITOR( monitor ), NULL );
	g_return_val_if_fail( FOOBAR_IS_WORKSPACE_SERVICE( workspace_service ), NULL );

	FoobarPanelItemWorkspaces* self = g_object_new( FOOBAR_TYPE_PANEL_ITEM_WORKSPACES, NULL );
	self->button_size = foobar_panel_item_workspaces_configuration_get_button_size( config );
	self->spacing = foobar_panel_item_workspaces_configuration_get_spacing( config );
	self->workspace_service = g_object_ref( workspace_service );

	// Set up the source model, optionally enabling filtering.

	GListModel* source_model = g_object_ref( foobar_workspace_service_get_workspaces( self->workspace_service ) );
	if ( monitor )
	{
		GtkCustomFilter* filter = gtk_custom_filter_new(
			foobar_panel_item_workspaces_monitor_filter_func,
			g_object_ref( monitor ),
			g_object_unref );
		self->monitor_filter = GTK_FILTER( filter );
		source_model = G_LIST_MODEL( gtk_filter_list_model_new( source_model, g_object_ref( self->monitor_filter ) ) );

		self->monitor_configuration_handler_id = g_signal_connect(
			self->workspace_service,
			"monitor-configuration-changed",
			G_CALLBACK( foobar_panel_item_workspaces_handle_monitor_configuration_changed ),
			self );
	}
	GtkNoSelection* selection_model = gtk_no_selection_new( source_model );
	gtk_list_view_set_model( GTK_LIST_VIEW( self->list_view ), GTK_SELECTION_MODEL( selection_model ) );

	// Wrap the list in an inset container to remove the outer margin from the items.

	foobar_inset_container_set_inset_horizontal( FOOBAR_INSET_CONTAINER( self->inset_container ), -self->spacing / 2 );
	foobar_inset_container_set_inset_vertical( FOOBAR_INSET_CONTAINER( self->inset_container ), -self->spacing / 2 );

	return FOOBAR_PANEL_ITEM( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called by the list view to create a widget for displaying a workspace.
//
void foobar_panel_item_workspaces_handle_item_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	FoobarPanelItemWorkspaces* self = (FoobarPanelItemWorkspaces*)userdata;

	GtkWidget* button = gtk_button_new( );
	gtk_widget_set_size_request( button, self->button_size, self->button_size );
	gtk_widget_set_margin_top( button, self->spacing / 2 );
	gtk_widget_set_margin_bottom( button, self->spacing / 2 );
	gtk_widget_set_margin_start( button, self->spacing / 2 );
	gtk_widget_set_margin_end( button, self->spacing / 2 );
	g_signal_connect(
		button,
		"clicked",
		G_CALLBACK( foobar_panel_item_workspaces_handle_item_clicked ),
		list_item );

	gtk_list_item_set_child( list_item, button );

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* active_expr = gtk_property_expression_new( FOOBAR_TYPE_WORKSPACE, item_expr, "flags" );
		GtkExpression* params[] = { active_expr };
		GtkExpression* css_expr = gtk_cclosure_expression_new(
			G_TYPE_STRV,
			NULL,
			G_N_ELEMENTS( params ),
			params,
			G_CALLBACK( foobar_panel_item_workspaces_compute_css_classes ),
			NULL,
			NULL );
		gtk_expression_bind( css_expr, button, "css-classes", list_item );
	}
}

//
// Called by the workspace service when the monitor for any workspace has changed.
//
// This is needed because the GtkFilterListModel does not track changes to the item properties and needs to be
// re-evaluated manually.
//
void foobar_panel_item_workspaces_handle_monitor_configuration_changed(
	FoobarWorkspaceService* service,
	gpointer                userdata )
{
	(void)service;
	FoobarPanelItemWorkspaces* self = (FoobarPanelItemWorkspaces*)userdata;

	gtk_filter_changed( self->monitor_filter, GTK_FILTER_CHANGE_DIFFERENT );
}

//
// Called when a workspace button was clicked, activating that workspace.
//
void foobar_panel_item_workspaces_handle_item_clicked( GtkButton* button, gpointer userdata )
{
	(void)button;
	GtkListItem* item = (GtkListItem*)userdata;

	FoobarWorkspace* workspace = gtk_list_item_get_item( item );
	if ( workspace ) { foobar_workspace_activate( workspace ); }
}

// ---------------------------------------------------------------------------------------------------------------------
// Value Converters
// ---------------------------------------------------------------------------------------------------------------------

//
// Derive the list of CSS classes for a workspace button from the workspace's current flags.
//
gchar** foobar_panel_item_workspaces_compute_css_classes(
	GtkExpression*       expression,
	FoobarWorkspaceFlags flags,
	gpointer             userdata )
{
	(void)expression;
	(void)userdata;

	GStrvBuilder* builder = g_strv_builder_new( );
	if ( flags & FOOBAR_WORKSPACE_FLAGS_ACTIVE ) { g_strv_builder_add( builder, "active" ); }
	if ( flags & FOOBAR_WORKSPACE_FLAGS_VISIBLE ) { g_strv_builder_add( builder, "visible" ); }
	if ( flags & FOOBAR_WORKSPACE_FLAGS_SPECIAL ) { g_strv_builder_add( builder, "special" ); }
	if ( flags & FOOBAR_WORKSPACE_FLAGS_URGENT ) { g_strv_builder_add( builder, "urgent" ); }
	if ( flags & FOOBAR_WORKSPACE_FLAGS_PERSISTENT ) { g_strv_builder_add( builder, "persistent" ); }
	return g_strv_builder_end( builder );
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Check if a workspace is on the panel's monitor.
//
gboolean foobar_panel_item_workspaces_monitor_filter_func( gpointer item, gpointer userdata )
{
	FoobarWorkspace* workspace = (FoobarWorkspace*)item;
	GdkMonitor* monitor = (GdkMonitor*)userdata;

	gchar const* expected = gdk_monitor_get_connector( monitor );
	gchar const* actual = foobar_workspace_get_monitor( workspace );
	return !expected || !g_strcmp0( expected, actual );
}
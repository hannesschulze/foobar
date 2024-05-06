#include "launcher.h"
#include "widgets/limit-container.h"
#include <gtk4-layer-shell.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

//
// FoobarLauncher:
//
// A window allowing the user to quickly search for applications and launch them.
//
// Note that it should be possible to continue typing, even when the results list view is focused. Conversely, the arrow
// keys can be used to select an item, even when the search input is focused.
//

struct _FoobarLauncher
{
	GtkWindow                   parent_instance;
	gchar**                     search_terms;
	GtkWidget*                  search_text;
	GtkWidget*                  list_view;
	GtkWidget*                  limit_container;
	GtkFilterListModel*         filter_model;
	GtkSingleSelection*         selection_model;
	FoobarApplicationService*   application_service;
	FoobarConfigurationService* configuration_service;
	gulong                      config_handler_id;
};

static void     foobar_launcher_class_init               ( FoobarLauncherClass*   klass );
static void     foobar_launcher_init                     ( FoobarLauncher*        self );
static void     foobar_launcher_finalize                 ( GObject*               object );
static void     foobar_launcher_handle_search_changed    ( GtkEditable*           editable,
                                                           gpointer               userdata );
static void     foobar_launcher_handle_search_activate   ( GtkText*               text,
                                                           gpointer               userdata );
static gboolean foobar_launcher_handle_search_key        ( GtkEventControllerKey* controller,
                                                           guint                  keyval,
                                                           guint                  keycode,
                                                           GdkModifierType        state,
                                                           gpointer               userdata );
static gboolean foobar_launcher_handle_list_key          ( GtkEventControllerKey* controller,
                                                           guint                  keyval,
                                                           guint                  keycode,
                                                           GdkModifierType        state,
                                                           gpointer               userdata );
static gboolean foobar_launcher_handle_window_key        ( GtkEventControllerKey* controller,
                                                           guint                  keyval,
                                                           guint                  keycode,
                                                           GdkModifierType        state,
                                                           gpointer               userdata );
static void     foobar_launcher_handle_item_setup        ( GtkListItemFactory*    factory,
                                                           GtkListItem*           list_item,
                                                           gpointer               userdata );
static void     foobar_launcher_handle_item_activate     ( GtkListView*           view,
                                                           guint                  position,
                                                           gpointer               userdata );
static void     foobar_launcher_handle_config_change     ( GObject*               object,
                                                           GParamSpec*            pspec,
                                                           gpointer               userdata );
static void     foobar_launcher_handle_show              ( GtkWidget*             widget,
                                                           gpointer               userdata );
static gboolean foobar_launcher_compute_icon_visible     ( GtkExpression*         expression,
                                                           GIcon*                 icon,
                                                           gpointer               userdata );
static gboolean foobar_launcher_compute_label_visible    ( GtkExpression*         expression,
                                                           gchar const*           label,
                                                           gpointer               userdata );
static gboolean foobar_launcher_compute_separator_visible( GtkExpression*         expression,
                                                           guint                  item_count,
                                                           gpointer               userdata );
static gboolean foobar_launcher_filter_func              ( gpointer               item,
                                                           gpointer               userdata );
static gboolean foobar_launcher_is_navigation_key        ( guint                  keyval );

G_DEFINE_FINAL_TYPE( FoobarLauncher, foobar_launcher, GTK_TYPE_WINDOW )

// ---------------------------------------------------------------------------------------------------------------------
// Window Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the launcher.
//
void foobar_launcher_class_init( FoobarLauncherClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->finalize = foobar_launcher_finalize;
}

//
// Instance initialization for the launcher.
//
void foobar_launcher_init( FoobarLauncher* self )
{
	self->search_terms = g_new0( gchar*, 1 );

	// Set up the search input and an event controller for auto-switching focus to the result list.

	GtkEventController* search_controller = gtk_event_controller_key_new( );
	g_signal_connect( search_controller, "key-pressed", G_CALLBACK( foobar_launcher_handle_search_key ), self );
	g_signal_connect( search_controller, "key-released", G_CALLBACK( foobar_launcher_handle_search_key ), self );

	self->search_text = gtk_text_new( );
	gtk_text_set_placeholder_text( GTK_TEXT( self->search_text ), "Search…" );
	gtk_widget_add_controller( self->search_text, search_controller );
	gtk_widget_set_hexpand( self->search_text, TRUE );
	g_signal_connect( self->search_text, "changed", G_CALLBACK( foobar_launcher_handle_search_changed ), self );
	g_signal_connect( self->search_text, "activate", G_CALLBACK( foobar_launcher_handle_search_activate ), self );

	GtkWidget* search_icon = gtk_image_new_from_icon_name( "fluent-search-symbolic" );

	GtkWidget* search = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_append( GTK_BOX( search ), search_icon );
	gtk_box_append( GTK_BOX( search ), self->search_text );
	gtk_widget_add_css_class( search, "search" );

	// Set up the separator (only shown if the results are not empty).

	GtkWidget *separator = gtk_separator_new( GTK_ORIENTATION_HORIZONTAL );

	{
		GtkExpression* list_expr = gtk_constant_expression_new( GTK_TYPE_FILTER_LIST_MODEL, self->filter_model );
		GtkExpression* count_expr = gtk_property_expression_new( GTK_TYPE_FILTER_LIST_MODEL, list_expr, "n-items" );
		GtkExpression* visible_params[] = { count_expr };
		GtkExpression* visible_expr = gtk_cclosure_expression_new(
			G_TYPE_BOOLEAN,
			NULL,
			G_N_ELEMENTS( visible_params ),
			visible_params,
			G_CALLBACK( foobar_launcher_compute_separator_visible ),
			NULL,
			NULL );
		gtk_expression_bind( visible_expr, separator, "visible", NULL );
	}

	// Set up the results list view and an event controller for auto-switching focus to the input.

	GtkListItemFactory* item_factory = gtk_signal_list_item_factory_new( );
	g_signal_connect( item_factory, "setup", G_CALLBACK( foobar_launcher_handle_item_setup ), NULL );

	GtkCustomFilter* filter = gtk_custom_filter_new( foobar_launcher_filter_func, self, NULL );

	self->filter_model = gtk_filter_list_model_new( NULL, GTK_FILTER( filter ) );

	self->selection_model = gtk_single_selection_new( G_LIST_MODEL( g_object_ref( self->filter_model ) ) );

	GtkEventController* list_controller = gtk_event_controller_key_new( );
	g_signal_connect( list_controller, "key-pressed", G_CALLBACK( foobar_launcher_handle_list_key ), self );
	g_signal_connect( list_controller, "key-released", G_CALLBACK( foobar_launcher_handle_list_key ), self );

	self->list_view = gtk_list_view_new( GTK_SELECTION_MODEL( g_object_ref( self->selection_model ) ), item_factory );
	gtk_list_view_set_single_click_activate( GTK_LIST_VIEW( self->list_view ), TRUE );
	gtk_scrollable_set_vscroll_policy( GTK_SCROLLABLE( self->list_view ), GTK_SCROLL_MINIMUM );
	gtk_widget_add_controller( self->list_view, list_controller );
	g_signal_connect( self->list_view, "activate", G_CALLBACK( foobar_launcher_handle_item_activate ), self );

	GtkWidget* scrolled_window = gtk_scrolled_window_new( );
	// XXX: Use GTK_POLICY_AUTOMATIC instead of GTK_POLICY_EXTERNAL once I figured out how to remove the minimum
	//  height in this case. For now, no scrollbar is shown.
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL );
	gtk_scrolled_window_set_child( GTK_SCROLLED_WINDOW( scrolled_window ), self->list_view );
	gtk_scrolled_window_set_propagate_natural_height( GTK_SCROLLED_WINDOW( scrolled_window ), TRUE );
	gtk_widget_set_vexpand( scrolled_window, TRUE );

	// Set up the layout.

	GtkWidget* layout = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_append( GTK_BOX( layout ), search );
	gtk_box_append( GTK_BOX( layout ), separator );
	gtk_box_append( GTK_BOX( layout ), scrolled_window );

	// Let the launcher take up space to accommodate the result list's natural height until the configured maximum
	// height.

	self->limit_container = foobar_limit_container_new( );
	foobar_limit_container_set_child( FOOBAR_LIMIT_CONTAINER( self->limit_container ), layout );

	// Set up the window, listening for the "Escape" key.

	GtkEventController* window_controller = gtk_event_controller_key_new( );
	g_signal_connect( window_controller, "key-released", G_CALLBACK( foobar_launcher_handle_window_key ), self );

	gtk_window_set_child( GTK_WINDOW( self ), self->limit_container );
	gtk_widget_add_controller( GTK_WIDGET( self ), window_controller );
	gtk_window_set_title( GTK_WINDOW( self ), "Foobar Launcher" );
	gtk_widget_add_css_class( GTK_WIDGET( self ), "launcher" );
	gtk_layer_init_for_window( GTK_WINDOW( self ) );
	gtk_layer_set_keyboard_mode( GTK_WINDOW( self ), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE );
	gtk_layer_set_namespace( GTK_WINDOW( self ), "foobar-launcher" );
	gtk_layer_set_anchor( GTK_WINDOW( self ), GTK_LAYER_SHELL_EDGE_TOP, TRUE );
	g_signal_connect( self, "show", G_CALLBACK( foobar_launcher_handle_show ), self );
}

//
// Instance cleanup for the launcher.
//
void foobar_launcher_finalize( GObject* object )
{
	FoobarLauncher* self = (FoobarLauncher*)object;

	g_clear_signal_handler( &self->config_handler_id, self->configuration_service );
	g_clear_object( &self->filter_model );
	g_clear_object( &self->selection_model );
	g_clear_object( &self->application_service );
	g_clear_object( &self->configuration_service );
	g_clear_pointer( &self->search_terms, g_strfreev );

	G_OBJECT_CLASS( foobar_launcher_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new launcher instance.
//
FoobarLauncher* foobar_launcher_new(
	FoobarApplicationService*   application_service,
	FoobarConfigurationService* configuration_service )
{
	g_return_val_if_fail( FOOBAR_IS_APPLICATION_SERVICE( application_service ), NULL );
	g_return_val_if_fail( FOOBAR_IS_CONFIGURATION_SERVICE( configuration_service ), NULL );

	FoobarLauncher* self = g_object_new( FOOBAR_TYPE_LAUNCHER, NULL );
	self->application_service = g_object_ref( application_service );
	self->configuration_service = g_object_ref( configuration_service );

	// Set up the result list view's source model.

	GListModel* source_model = foobar_application_service_get_items( self->application_service );
	gtk_filter_list_model_set_model( self->filter_model, g_object_ref( source_model ) );

	// Apply the configuration and subscribe to changes.

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_launcher_apply_configuration( self, foobar_configuration_get_launcher( config ) );
	self->config_handler_id = g_signal_connect(
		self->configuration_service,
		"notify::current",
		G_CALLBACK( foobar_launcher_handle_config_change ),
		self );

	return self;
}

//
// Apply the launcher configuration provided by the configuration service.
//
void foobar_launcher_apply_configuration(
	FoobarLauncher*                    self,
	FoobarLauncherConfiguration const* config )
{
	g_return_if_fail( FOOBAR_IS_LAUNCHER( self ) );
	g_return_if_fail( config != NULL );

	gtk_window_set_default_size( GTK_WINDOW( self ), foobar_launcher_configuration_get_width( config ), 2 );
	gtk_layer_set_margin(
		GTK_WINDOW( self ),
		GTK_LAYER_SHELL_EDGE_TOP,
		foobar_launcher_configuration_get_position( config ) );
	foobar_limit_container_set_max_height(
		FOOBAR_LIMIT_CONTAINER( self->limit_container ),
		foobar_launcher_configuration_get_max_height( config ) );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the user confirms the selection while the search input is focused.
//
// We forward the activation to the results list view.
//
void foobar_launcher_handle_search_activate(
	GtkText* text,
	gpointer userdata )
{
	(void)text;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	guint selection = gtk_single_selection_get_selected( self->selection_model );
	if ( selection != GTK_INVALID_LIST_POSITION )
	{
		gtk_widget_activate_action( self->list_view, "list.activate-item", "u", selection );
	}
}

//
// Called when the search query has changed.
//
// We update the tokenized search terms and then the result filter.
//
void foobar_launcher_handle_search_changed(
	GtkEditable* editable,
	gpointer     userdata )
{
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	GStrvBuilder* terms_builder = g_strv_builder_new( );
	g_autofree gchar* query = g_strdup( gtk_editable_get_text( editable ) );
	gchar* query_start = query;
	gchar* save;
	gchar* token;
	while ( ( token = strtok_r( query_start, " \t", &save ) ) )
	{
		if ( *token ) { g_strv_builder_add( terms_builder, token ); }
		query_start = NULL;
	}

	g_clear_pointer( &self->search_terms, g_strfreev );
	self->search_terms = g_strv_builder_end( terms_builder );

	GtkFilter* filter = gtk_filter_list_model_get_filter( self->filter_model );
	gtk_filter_changed( filter, GTK_FILTER_CHANGE_DIFFERENT );
}

//
// Handle keyboard events while the search input is focused.
//
// If this is a navigational key, the event is forwarded to the results list view.
//
gboolean foobar_launcher_handle_search_key(
	GtkEventControllerKey* controller,
	guint                  keyval,
	guint                  keycode,
	GdkModifierType        state,
	gpointer               userdata )
{
	(void)keycode;
	(void)state;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	if ( foobar_launcher_is_navigation_key( keyval ) )
	{
		gtk_widget_grab_focus( self->list_view );
		return gtk_event_controller_key_forward( controller, self->list_view );
	}
	else
	{
		return FALSE;
	}
}

//
// Handle keyboard events while the list view is focused.
//
// If this is not a navigational key, the event is forwarded to the search input, which is also focused.
//
gboolean foobar_launcher_handle_list_key(
	GtkEventControllerKey* controller,
	guint                  keyval,
	guint                  keycode,
	GdkModifierType        state,
	gpointer               userdata )
{
	(void)keycode;
	(void)state;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	if ( foobar_launcher_is_navigation_key( keyval ) )
	{
		return FALSE;
	}
	else
	{
		gtk_text_grab_focus_without_selecting( GTK_TEXT( self->search_text ) );
		return gtk_event_controller_key_forward( controller, self->search_text );
	}
}

//
// Handle general keyboard events for the window.
//
// This is used to allow the user to close the launcher by pressing the "Escape" key.
//
gboolean foobar_launcher_handle_window_key(
	GtkEventControllerKey* controller,
	guint                  keyval,
	guint                  keycode,
	GdkModifierType        state,
	gpointer               userdata )
{
	(void)controller;
	(void)keycode;
	(void)state;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	if ( keyval == GDK_KEY_Escape )
	{
		gtk_widget_set_visible( GTK_WIDGET( self ), FALSE );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//
// Called by the result list view to create a widget for displaying an application item.
//
void foobar_launcher_handle_item_setup(
	GtkListItemFactory* factory,
	GtkListItem*        list_item,
	gpointer            userdata )
{
	(void)factory;
	(void)userdata;

	// Set up layout.

	GtkWidget* icon = gtk_image_new( );
	gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );
	gtk_widget_add_css_class( icon, "icon" );

	GtkWidget* name = gtk_label_new( NULL );
	gtk_label_set_justify( GTK_LABEL( name ), GTK_JUSTIFY_LEFT );
	gtk_label_set_xalign( GTK_LABEL( name ), 0 );
	gtk_label_set_wrap( GTK_LABEL( name ), FALSE );
	gtk_label_set_ellipsize( GTK_LABEL( name ), PANGO_ELLIPSIZE_END );
	gtk_widget_add_css_class( name, "name" );

	GtkWidget* description = gtk_label_new( NULL );
	gtk_label_set_justify( GTK_LABEL( description ), GTK_JUSTIFY_LEFT );
	gtk_label_set_xalign( GTK_LABEL( description ), 0 );
	gtk_label_set_wrap( GTK_LABEL( name ), FALSE );
	gtk_label_set_ellipsize( GTK_LABEL( description ), PANGO_ELLIPSIZE_END );
	gtk_widget_add_css_class( description, "description" );

	GtkWidget* column = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_hexpand( column, TRUE );
	gtk_widget_set_valign( column, GTK_ALIGN_CENTER );
	gtk_box_append( GTK_BOX( column ), name );
	gtk_box_append( GTK_BOX( column ), description );

	GtkWidget* row = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_add_css_class( row, "item" );
	gtk_box_append( GTK_BOX( row ), icon );
	gtk_box_append( GTK_BOX( row ), column );

	gtk_list_item_set_child( list_item, row );

	// Set up bindings.

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* icon_expr = gtk_property_expression_new( FOOBAR_TYPE_APPLICATION_ITEM, item_expr, "icon" );
		gtk_expression_bind( gtk_expression_ref( icon_expr ), icon, "gicon", list_item );
		GtkExpression* visible_params[] = { icon_expr };
		GtkExpression* visible_expr = gtk_cclosure_expression_new(
			G_TYPE_BOOLEAN,
			NULL,
			G_N_ELEMENTS( visible_params ),
			visible_params,
			G_CALLBACK( foobar_launcher_compute_icon_visible ),
			NULL,
			NULL );
		gtk_expression_bind( visible_expr, icon, "visible", list_item );
	}

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* name_expr = gtk_property_expression_new( FOOBAR_TYPE_APPLICATION_ITEM, item_expr, "name" );
		gtk_expression_bind( name_expr, name, "label", list_item );
	}

	{
		GtkExpression* item_expr = gtk_property_expression_new( GTK_TYPE_LIST_ITEM, NULL, "item" );
		GtkExpression* description_expr = gtk_property_expression_new( FOOBAR_TYPE_APPLICATION_ITEM, item_expr, "description" );
		gtk_expression_bind( gtk_expression_ref( description_expr ), description, "label", list_item );
		GtkExpression* visible_params[] = { description_expr };
		GtkExpression* visible_expr = gtk_cclosure_expression_new(
			G_TYPE_BOOLEAN,
			NULL,
			G_N_ELEMENTS( visible_params ),
			visible_params,
			G_CALLBACK( foobar_launcher_compute_label_visible ),
			NULL,
			NULL );
		gtk_expression_bind( visible_expr, description, "visible", list_item );
	}
}

//
// Called by the result list view when an application was selected.
//
void foobar_launcher_handle_item_activate(
	GtkListView* view,
	guint        position,
	gpointer     userdata )
{
	(void)view;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	GListModel* list = G_LIST_MODEL( gtk_list_view_get_model( view ) );
	FoobarApplicationItem* item = g_list_model_get_item( list, position );
	foobar_application_item_launch( item );
	gtk_widget_set_visible( GTK_WIDGET( self ), FALSE );
}

//
// Signal handler called when the global configuration file has changed.
//
void foobar_launcher_handle_config_change(
	GObject*    object,
	GParamSpec* pspec,
	gpointer    userdata )
{
	(void)object;
	(void)pspec;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	FoobarConfiguration const* config = foobar_configuration_service_get_current( self->configuration_service );
	foobar_launcher_apply_configuration( self, foobar_configuration_get_launcher( config ) );
}

//
// Called before the launcher is presented.
//
// This is used to reset the UI state.
//
void foobar_launcher_handle_show(
	GtkWidget* widget,
	gpointer   userdata )
{
	(void)widget;
	FoobarLauncher* self = (FoobarLauncher*)userdata;

	gtk_editable_set_text( GTK_EDITABLE( self->search_text ), "" );
	if ( g_list_model_get_n_items( G_LIST_MODEL( self->selection_model ) ) > 0 )
	{
		gtk_list_view_scroll_to(
			GTK_LIST_VIEW( self->list_view ),
			0,
			GTK_LIST_SCROLL_FOCUS | GTK_LIST_SCROLL_SELECT,
			NULL );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Value Converters
// ---------------------------------------------------------------------------------------------------------------------

//
// Derive the visibility of an application icon from its value.
//
gboolean foobar_launcher_compute_icon_visible(
	GtkExpression* expression,
	GIcon*         icon,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return icon != NULL;
}

//
// Derive the visibility of an application label from its value.
//
gboolean foobar_launcher_compute_label_visible(
	GtkExpression* expression,
	gchar const*   label,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return label != NULL;
}

//
// Derive the visibility of the separator between search input and result list view from the number of results.
//
gboolean foobar_launcher_compute_separator_visible(
	GtkExpression* expression,
	guint          item_count,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return item_count > 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Match a result item against the current search query.
//
gboolean foobar_launcher_filter_func( gpointer item, gpointer userdata )
{
	FoobarLauncher* self = (FoobarLauncher*)userdata;
	FoobarApplicationItem* app = (FoobarApplicationItem*)item;

	return foobar_application_item_match( app, (gchar const* const*)self->search_terms );
}

//
// Check if a key value is that of a navigation key (i.e., an arrow key).
//
gboolean foobar_launcher_is_navigation_key( guint keyval )
{
	switch ( keyval )
	{
		case GDK_KEY_Up:
		case GDK_KEY_Down:
		case GDK_KEY_Left:
		case GDK_KEY_Right:
			return TRUE;
		default:
			return FALSE;
	}
}
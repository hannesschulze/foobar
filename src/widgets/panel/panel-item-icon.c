#include "widgets/panel/panel-item-icon.h"

//
// FoobarPanelItemIcon:
//
// Simple item implementation displaying an icon.
//

struct _FoobarPanelItemIcon
{
	FoobarPanelItem       parent_instance;
	GtkWidget*            button;
	FoobarPanelItemAction action;
};

static void foobar_panel_item_icon_class_init    ( FoobarPanelItemIconClass* klass );
static void foobar_panel_item_icon_init          ( FoobarPanelItemIcon*      self );
static void foobar_panel_item_icon_handle_clicked( GtkButton*                button,
                                                   gpointer                  userdata );

G_DEFINE_FINAL_TYPE( FoobarPanelItemIcon, foobar_panel_item_icon, FOOBAR_TYPE_PANEL_ITEM )

// ---------------------------------------------------------------------------------------------------------------------
// Panel Item Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for icon panel items.
//
void foobar_panel_item_icon_class_init( FoobarPanelItemIconClass* klass )
{
	(void)klass;
}

//
// Instance initialization for icon panel items.
//
void foobar_panel_item_icon_init( FoobarPanelItemIcon* self )
{
	self->button = gtk_button_new( );
	gtk_button_set_has_frame( GTK_BUTTON( self->button ), FALSE );
	gtk_widget_set_halign( self->button, GTK_ALIGN_CENTER );
	gtk_widget_set_valign( self->button, GTK_ALIGN_CENTER );
	g_signal_connect(
		self->button,
		"clicked",
		G_CALLBACK( foobar_panel_item_icon_handle_clicked ),
		self );

	gtk_widget_add_css_class( GTK_WIDGET( self ), "icon" );
	foobar_panel_item_set_child( FOOBAR_PANEL_ITEM( self ), self->button );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new icon panel item with the given configuration.
//
FoobarPanelItem* foobar_panel_item_icon_new( FoobarPanelItemConfiguration const* config )
{
	g_return_val_if_fail( config != NULL, NULL );
	g_return_val_if_fail( foobar_panel_item_configuration_get_kind( config ) == FOOBAR_PANEL_ITEM_KIND_ICON, NULL );

	FoobarPanelItemIcon* self = g_object_new( FOOBAR_TYPE_PANEL_ITEM_ICON, NULL );
	self->action = foobar_panel_item_configuration_get_action( config );

	gtk_widget_set_sensitive( self->button, self->action != FOOBAR_PANEL_ITEM_ACTION_NONE );
	gtk_button_set_icon_name(
		GTK_BUTTON( self->button ),
		foobar_panel_item_icon_configuration_get_icon_name( config ) );

	return FOOBAR_PANEL_ITEM( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the button was clicked.
//
void foobar_panel_item_icon_handle_clicked(
	GtkButton* button,
	gpointer   userdata )
{
	(void)button;
	FoobarPanelItemIcon* self = (FoobarPanelItemIcon*)userdata;

	foobar_panel_item_invoke_action( FOOBAR_PANEL_ITEM( self ), self->action );
}
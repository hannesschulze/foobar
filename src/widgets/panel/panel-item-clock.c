#include "widgets/panel/panel-item-clock.h"

//
// FoobarPanelItemClock:
//
// A panel item displaying the current time in the format configured by the user.
//

struct _FoobarPanelItemClock
{
	FoobarPanelItem       parent_instance;
	gchar*                format;
	FoobarPanelItemAction action;
	GtkWidget*            label;
	GtkWidget*            button;
	FoobarClockService*   clock_service;
};

static void   foobar_panel_item_clock_class_init    ( FoobarPanelItemClockClass* klass );
static void   foobar_panel_item_clock_init          ( FoobarPanelItemClock*      self );
static void   foobar_panel_item_clock_finalize      ( GObject*                   object );
static void   foobar_panel_item_clock_handle_clicked( GtkButton*                 button,
                                                      gpointer                   userdata );
static gchar* foobar_panel_item_clock_compute_label ( GtkExpression*             expression,
                                                      GDateTime*                 time,
                                                      gpointer                   userdata );

G_DEFINE_FINAL_TYPE( FoobarPanelItemClock, foobar_panel_item_clock, FOOBAR_TYPE_PANEL_ITEM )

// ---------------------------------------------------------------------------------------------------------------------
// Panel Item Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for clock panel items.
//
void foobar_panel_item_clock_class_init( FoobarPanelItemClockClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->finalize = foobar_panel_item_clock_finalize;
}

//
// Instance initialization for clock panel items.
//
void foobar_panel_item_clock_init( FoobarPanelItemClock* self )
{
	self->label = gtk_label_new( NULL );
	gtk_label_set_justify( GTK_LABEL( self->label ), GTK_JUSTIFY_CENTER );

	self->button = gtk_button_new( );
	gtk_button_set_has_frame( GTK_BUTTON( self->button ), FALSE );
	gtk_button_set_child( GTK_BUTTON( self->button ), self->label );
	gtk_widget_set_halign( self->button, GTK_ALIGN_CENTER );
	gtk_widget_set_valign( self->button, GTK_ALIGN_CENTER );
	g_signal_connect(
		self->button,
		"clicked",
		G_CALLBACK( foobar_panel_item_clock_handle_clicked ),
		self );

	gtk_widget_add_css_class( GTK_WIDGET( self ), "clock" );
	foobar_panel_item_set_child( FOOBAR_PANEL_ITEM( self ), self->button );
}

//
// Instance cleanup for clock panel items.
//
void foobar_panel_item_clock_finalize( GObject* object )
{
	FoobarPanelItemClock* self = (FoobarPanelItemClock*)object;

	g_clear_pointer( &self->format, g_free );
	g_clear_object( &self->clock_service );

	G_OBJECT_CLASS( foobar_panel_item_clock_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new clock panel item with the given configuration.
//
FoobarPanelItem* foobar_panel_item_clock_new(
	FoobarPanelItemConfiguration const* config,
	FoobarClockService*                 clock_service )
{
	g_return_val_if_fail( config != NULL, NULL );
	g_return_val_if_fail( foobar_panel_item_configuration_get_kind( config ) == FOOBAR_PANEL_ITEM_KIND_CLOCK, NULL );
	g_return_val_if_fail( FOOBAR_IS_CLOCK_SERVICE( clock_service ), NULL );

	FoobarPanelItemClock* self = g_object_new( FOOBAR_TYPE_PANEL_ITEM_CLOCK, NULL );
	self->format = g_strdup( foobar_panel_item_clock_configuration_get_format( config ) );
	self->action = foobar_panel_item_configuration_get_action( config );
	self->clock_service = g_object_ref( clock_service );

	gtk_widget_set_sensitive( self->button, self->action != FOOBAR_PANEL_ITEM_ACTION_NONE );

	{
		GtkExpression* service_expr = gtk_constant_expression_new( FOOBAR_TYPE_CLOCK_SERVICE, self->clock_service );
		GtkExpression* time_expr = gtk_property_expression_new( FOOBAR_TYPE_CLOCK_SERVICE, service_expr, "time" );
		GtkExpression* label_params[] = { time_expr };
		GtkExpression* label_expr = gtk_cclosure_expression_new(
			G_TYPE_STRING,
			NULL,
			G_N_ELEMENTS( label_params ),
			label_params,
			G_CALLBACK( foobar_panel_item_clock_compute_label ),
			self,
			NULL );
		gtk_expression_bind( label_expr, self->label, "label", NULL );
	}

	return FOOBAR_PANEL_ITEM( self );
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the button was clicked.
//
void foobar_panel_item_clock_handle_clicked(
	GtkButton* button,
	gpointer   userdata )
{
	(void)button;
	FoobarPanelItemClock* self = (FoobarPanelItemClock*)userdata;

	foobar_panel_item_invoke_action( FOOBAR_PANEL_ITEM( self ), self->action );
}

// ---------------------------------------------------------------------------------------------------------------------
// Value Converters
// ---------------------------------------------------------------------------------------------------------------------

//
// Derive the label from a timestamp, depending on the configured time format.
//
gchar* foobar_panel_item_clock_compute_label(
	GtkExpression* expression,
	GDateTime*     time,
	gpointer       userdata )
{
	(void)expression;
	FoobarPanelItemClock* self = (FoobarPanelItemClock*)userdata;

	return g_date_time_format( time, self->format );
}

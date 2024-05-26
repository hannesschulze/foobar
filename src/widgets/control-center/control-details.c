#include "widgets/control-center/control-details.h"

//
// FoobarControlDetails:
//
// A revealer for details in the "controls" section of the control center. This widget is usually used to embed a list
// view. It takes care of scrolling by embedding the child in a GtkScrolledWindow.
//

struct _FoobarControlDetails
{
	GtkWidget  parent_instance;
	GtkWidget* child;
	gboolean   is_expanded;
	gint       inset_top;
};

enum
{
	PROP_CHILD = 1,
	PROP_IS_EXPANDED,
	PROP_INSET_TOP,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_control_details_class_init  ( FoobarControlDetailsClass* klass );
static void foobar_control_details_init        ( FoobarControlDetails*      self );
static void foobar_control_details_get_property( GObject*                   object,
                                                 guint                      prop_id,
                                                 GValue*                    value,
                                                 GParamSpec*                pspec );
static void foobar_control_details_set_property( GObject*                   object,
                                                 guint                      prop_id,
                                                 GValue const*              value,
                                                 GParamSpec*                pspec );
static void foobar_control_details_dispose     ( GObject*                   object );

G_DEFINE_FINAL_TYPE( FoobarControlDetails, foobar_control_details, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for control details.
//
void foobar_control_details_class_init( FoobarControlDetailsClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	gtk_widget_class_set_css_name( widget_klass, "control-details" );
	gtk_widget_class_set_layout_manager_type( widget_klass, GTK_TYPE_BIN_LAYOUT );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_control_details_get_property;
	object_klass->set_property = foobar_control_details_set_property;
	object_klass->dispose = foobar_control_details_dispose;

	props[PROP_CHILD] = g_param_spec_object(
		"child",
		"Child",
		"Child widget for the details revealer.",
		GTK_TYPE_WIDGET,
		G_PARAM_READWRITE );
	props[PROP_IS_EXPANDED] = g_param_spec_boolean(
		"is-expanded",
		"Is Expanded",
		"Current state of the details revealer.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_INSET_TOP] = g_param_spec_int(
		"inset-top",
		"Inset Top",
		"Inset from the top edge (only visible when expanded).",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for control details.
//
void foobar_control_details_init( FoobarControlDetails* self )
{
	GtkWidget* scrolled_window = gtk_scrolled_window_new( );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL );
	gtk_scrolled_window_set_propagate_natural_height( GTK_SCROLLED_WINDOW( scrolled_window ), TRUE );
	g_object_bind_property(
		self,
		"child",
		scrolled_window,
		"child",
		G_BINDING_SYNC_CREATE );
	g_object_bind_property(
		self,
		"inset-top",
		scrolled_window,
		"margin-top",
		G_BINDING_SYNC_CREATE );

	GtkWidget* revealer = gtk_revealer_new( );
	gtk_revealer_set_transition_type( GTK_REVEALER( revealer ), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN );
	gtk_revealer_set_transition_duration( GTK_REVEALER( revealer ), 100 );
	gtk_revealer_set_child( GTK_REVEALER( revealer ), scrolled_window );
	g_object_bind_property(
		self,
		"is-expanded",
		revealer,
		"reveal-child",
		G_BINDING_SYNC_CREATE );

	gtk_widget_set_parent( revealer, GTK_WIDGET( self ) );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_control_details_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarControlDetails* self = (FoobarControlDetails*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			g_value_set_object( value, foobar_control_details_get_child( self ) );
			break;
		case PROP_IS_EXPANDED:
			g_value_set_boolean( value, foobar_control_details_is_expanded( self ) );
			break;
		case PROP_INSET_TOP:
			g_value_set_int( value, foobar_control_details_get_inset_top( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_control_details_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarControlDetails* self = (FoobarControlDetails*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			foobar_control_details_set_child( self, g_value_get_object( value ) );
			break;
		case PROP_IS_EXPANDED:
			foobar_control_details_set_expanded( self, g_value_get_boolean( value ) );
			break;
		case PROP_INSET_TOP:
			foobar_control_details_set_inset_top( self, g_value_get_int( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for control details.
//
void foobar_control_details_dispose( GObject* object )
{
	FoobarControlDetails* self = (FoobarControlDetails*)object;

	GtkWidget* child;
	while ( ( child = gtk_widget_get_first_child( GTK_WIDGET( self ) ) ) )
	{
		gtk_widget_unparent( child );
	}

	g_clear_object( &self->child );

	G_OBJECT_CLASS( foobar_control_details_parent_class )->dispose( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new control details instance.
//
GtkWidget* foobar_control_details_new( void )
{
	return g_object_new( FOOBAR_TYPE_CONTROL_DETAILS, NULL );
}

//
// Get the child widget for the details revealer.
//
GtkWidget* foobar_control_details_get_child( FoobarControlDetails* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_DETAILS( self ), NULL );
	return self->child;
}

//
// Get the current state of the details revealer.
//
gboolean foobar_control_details_is_expanded( FoobarControlDetails* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_DETAILS( self ), FALSE );
	return self->is_expanded;
}

//
// Get the inset from the top edge (only visible when expanded).
//
gint foobar_control_details_get_inset_top( FoobarControlDetails* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_DETAILS( self ), FALSE );
	return self->inset_top;
}

//
// Update the child widget for the details revealer.
//
void foobar_control_details_set_child(
	FoobarControlDetails* self,
	GtkWidget*            value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_DETAILS( self ) );

	if ( self->child != value )
	{
		g_clear_object( &self->child );
		if ( value ) { self->child = g_object_ref( value ); }
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CHILD] );
	}
}

//
// Update the current state of the details revealer.
//
void foobar_control_details_set_expanded(
	FoobarControlDetails* self,
	gboolean              value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_DETAILS( self ) );

	value = !!value;
	if ( self->is_expanded != value )
	{
		self->is_expanded = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_EXPANDED] );

		if ( self->is_expanded )
		{
			gtk_widget_add_css_class( GTK_WIDGET( self ), "expanded" );
		}
		else
		{
			gtk_widget_remove_css_class( GTK_WIDGET( self ), "expanded" );
		}
	}
}

//
// Update the inset from the top edge (only visible when expanded).
//
void foobar_control_details_set_inset_top(
	FoobarControlDetails* self,
	gint                  value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_DETAILS( self ) );

	value = MAX( value, 0 );
	if ( self->inset_top != value )
	{
		self->inset_top = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_TOP] );
	}
}

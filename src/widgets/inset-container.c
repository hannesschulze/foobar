#include "widgets/inset-container.h"

//
// FoobarInsetContainer:
//
// A custom widget which renders a child with an inset/padding.
//
// Notably, it also supports negative inset values which is useful in order to provide uniform spacing between widgets.
//

struct _FoobarInsetContainer
{
	GtkWidget  parent_instance;
	GtkWidget* child;
	gint       inset_horizontal;
	gint       inset_vertical;
};

enum
{
	PROP_CHILD = 1,
	PROP_INSET_HORIZONTAL,
	PROP_INSET_VERTICAL,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void               foobar_inset_container_class_init      ( FoobarInsetContainerClass* klass );
static void               foobar_inset_container_init            ( FoobarInsetContainer*      self );
static void               foobar_inset_container_get_property    ( GObject*                   object,
                                                                   guint                      prop_id,
                                                                   GValue*                    value,
                                                                   GParamSpec*                pspec );
static void               foobar_inset_container_set_property    ( GObject*                   object,
                                                                   guint                      prop_id,
                                                                   GValue const*              value,
                                                                   GParamSpec*                pspec );
static void               foobar_inset_container_dispose         ( GObject*                   object );
static GtkSizeRequestMode foobar_inset_container_get_request_mode( GtkWidget*                 widget );
static void               foobar_inset_container_measure         ( GtkWidget*                 widget,
                                                                   GtkOrientation             orientation,
                                                                   int                        for_size,
                                                                   int*                       minimum,
                                                                   int*                       natural,
                                                                   int*                       minimum_baseline,
                                                                   int*                       natural_baseline );
static void               foobar_inset_container_size_allocate   ( GtkWidget*                 widget,
                                                                   int                        width,
                                                                   int                        height,
                                                                   int                        baseline );

G_DEFINE_FINAL_TYPE( FoobarInsetContainer, foobar_inset_container, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for inset containers.
//
void foobar_inset_container_class_init( FoobarInsetContainerClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	widget_klass->get_request_mode = foobar_inset_container_get_request_mode;
	widget_klass->measure = foobar_inset_container_measure;
	widget_klass->size_allocate = foobar_inset_container_size_allocate;
	gtk_widget_class_set_css_name( widget_klass, "inset-container" );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_inset_container_get_property;
	object_klass->set_property = foobar_inset_container_set_property;
	object_klass->dispose = foobar_inset_container_dispose;

	props[PROP_CHILD] = g_param_spec_object(
		"child",
		"Child",
		"Child widget for the panel item.",
		GTK_TYPE_WIDGET,
		G_PARAM_READWRITE );
	props[PROP_INSET_HORIZONTAL] = g_param_spec_int(
		"inset-horizontal",
		"Inset Horizontal",
		"The horizontal offset (may be negative).",
		INT_MIN,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_INSET_VERTICAL] = g_param_spec_int(
		"inset-vertical",
		"Inset Vertical",
		"The vertical offset (may be negative).",
		INT_MIN,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for inset containers.
//
void foobar_inset_container_init( FoobarInsetContainer* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_inset_container_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarInsetContainer* self = (FoobarInsetContainer*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			g_value_set_object( value, foobar_inset_container_get_child( self ) );
			break;
		case PROP_INSET_HORIZONTAL:
			g_value_set_int( value, foobar_inset_container_get_inset_horizontal( self ) );
			break;
		case PROP_INSET_VERTICAL:
			g_value_set_int( value, foobar_inset_container_get_inset_vertical( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_inset_container_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarInsetContainer* self = (FoobarInsetContainer*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			foobar_inset_container_set_child( self, g_value_get_object( value ) );
			break;
		case PROP_INSET_HORIZONTAL:
			foobar_inset_container_set_inset_horizontal( self, g_value_get_int( value ) );
			break;
		case PROP_INSET_VERTICAL:
			foobar_inset_container_set_inset_vertical( self, g_value_get_int( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for inset containers.
//
void foobar_inset_container_dispose( GObject* object )
{
	FoobarInsetContainer* self = (FoobarInsetContainer*)object;

	g_clear_pointer( &self->child, gtk_widget_unparent );

	G_OBJECT_CLASS( foobar_inset_container_parent_class )->dispose( object );
}

//
// Determine the size request mode used to measure the widget.
//
// This implementation just forwards the request mode from the child widget.
//
GtkSizeRequestMode foobar_inset_container_get_request_mode( GtkWidget* widget )
{
	FoobarInsetContainer* self = (FoobarInsetContainer*)widget;
	if ( !self->child ) { return GTK_SIZE_REQUEST_CONSTANT_SIZE; }

	return gtk_widget_get_request_mode( self->child );
}

//
// Compute the requested size of the widget.
//
// This implementation uses the size requested by the child widget and then adds the inset to it.
//
void foobar_inset_container_measure(
	GtkWidget*     widget,
	GtkOrientation orientation,
	int            for_size,
	int*           minimum,
	int*           natural,
	int*           minimum_baseline,
	int*           natural_baseline )
{
	FoobarInsetContainer* self = (FoobarInsetContainer*)widget;
	if ( !self->child )
	{
		*minimum = 0;
		*natural = 0;
		*minimum_baseline = 0;
		*natural_baseline = 0;
		return;
	}

	// Get the inset for the given orientation and the other one.

	gint inset;
	gint other_inset;
	switch ( orientation )
	{
		case GTK_ORIENTATION_HORIZONTAL:
			inset = self->inset_horizontal;
			other_inset = self->inset_vertical;
			break;
		case GTK_ORIENTATION_VERTICAL:
			inset = self->inset_vertical;
			other_inset = self->inset_horizontal;
			break;
		default:
			g_warn_if_reached( );
			return;
	}

	// Compute the new size, taking into consideration that the inset might have already been added to measure the other
	// side and needs to be subtracted before passing it to the child widget's measure function.

	for_size = MAX( 0, for_size - ( 2 * other_inset ) );
	gtk_widget_measure( self->child, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline );
	*minimum = MAX( 0, *minimum + ( 2 * inset ) );
	*natural = MAX( 0, *natural + ( 2 * inset ) );
	if ( orientation == GTK_ORIENTATION_VERTICAL )
	{
		*minimum_baseline = MAX( 0, *minimum_baseline + inset );
		*natural_baseline = MAX( 0, *natural_baseline + inset );
	}
}

//
// Arrange the child widget.
//
void foobar_inset_container_size_allocate(
	GtkWidget* widget,
	int        width,
	int        height,
	int        baseline )
{
	FoobarInsetContainer* self = (FoobarInsetContainer*)widget;
	if ( !self->child ) { return; }

	GtkAllocation allocation =
		{
			.width = MAX( 0, width - 2 * self->inset_horizontal ),
			.height = MAX( 0, height - 2 * self->inset_vertical ),
			.x = self->inset_horizontal,
			.y = self->inset_vertical,
		};
	gtk_widget_size_allocate( self->child, &allocation, MAX( 0, baseline + self->inset_vertical ) );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new inset container instance.
//
GtkWidget* foobar_inset_container_new( void )
{
	return g_object_new( FOOBAR_TYPE_INSET_CONTAINER, NULL );
}

//
// Get the container's child widget.
//
GtkWidget* foobar_inset_container_get_child( FoobarInsetContainer* self )
{
	g_return_val_if_fail( FOOBAR_IS_INSET_CONTAINER( self ), NULL );
	return self->child;
}

//
// Get the inset applied to the left and right side of the widget.
//
gint foobar_inset_container_get_inset_horizontal( FoobarInsetContainer* self )
{
	g_return_val_if_fail( FOOBAR_IS_INSET_CONTAINER( self ), 0 );
	return self->inset_horizontal;
}

//
// Get the inset applied to the top and bottom side of the widget.
//
gint foobar_inset_container_get_inset_vertical( FoobarInsetContainer* self )
{
	g_return_val_if_fail( FOOBAR_IS_INSET_CONTAINER( self ), 0 );
	return self->inset_vertical;
}

//
// Update the container's child widget.
//
void foobar_inset_container_set_child(
	FoobarInsetContainer* self,
	GtkWidget*            child )
{
	g_return_if_fail( FOOBAR_IS_INSET_CONTAINER( self ) );
	g_return_if_fail( child == NULL || self->child == child || gtk_widget_get_parent( child ) == NULL );

	if ( self->child != child )
	{
		g_clear_pointer( &self->child, gtk_widget_unparent );

		if ( child )
		{
			self->child = child;
			gtk_widget_set_parent( child, GTK_WIDGET( self ) );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CHILD] );
	}
}

//
// Update the inset applied to the left and right side of the widget.
//
void foobar_inset_container_set_inset_horizontal(
	FoobarInsetContainer* self,
	gint                  value )
{
	g_return_if_fail( FOOBAR_IS_INSET_CONTAINER( self ) );

	if ( self->inset_horizontal != value )
	{
		self->inset_horizontal = value;
		gtk_widget_queue_resize( GTK_WIDGET( self ) );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_HORIZONTAL] );
	}
}

//
// Update the inset applied to the top and bottom side of the widget.
//
void foobar_inset_container_set_inset_vertical(
	FoobarInsetContainer* self,
	gint                  value )
{
	g_return_if_fail( FOOBAR_IS_INSET_CONTAINER( self ) );

	if ( self->inset_vertical != value )
	{
		self->inset_vertical = value;
		gtk_widget_queue_resize( GTK_WIDGET( self ) );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_VERTICAL] );
	}
}
#include "widgets/limit-container.h"

//
// FoobarLimitContainer:
//
// A container which uses the natural size requested by the child as its minimum size, but only up to a certain point.
//
// This is useful because gtk_scrolled_window_set_max_content_height doesn't seem to be doing what I understand it to
// do. This way, we can set propagate_natural_height to TRUE and then limit the height through this container.
//

struct _FoobarLimitContainer
{
	GtkWidget  parent_instance;
	GtkWidget* child;
	gint       max_width;
	gint       max_height;
};

enum
{
	PROP_CHILD = 1,
	PROP_MAX_WIDTH,
	PROP_MAX_HEIGHT,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void               foobar_limit_container_class_init      ( FoobarLimitContainerClass* klass );
static void               foobar_limit_container_init            ( FoobarLimitContainer*      self );
static void               foobar_limit_container_get_property    ( GObject*                   object,
                                                                   guint                      prop_id,
                                                                   GValue*                    value,
                                                                   GParamSpec*                pspec );
static void               foobar_limit_container_set_property    ( GObject*                   object,
                                                                   guint                      prop_id,
                                                                   GValue const*              value,
                                                                   GParamSpec*                pspec );
static void               foobar_limit_container_dispose         ( GObject*                   object );
static GtkSizeRequestMode foobar_limit_container_get_request_mode( GtkWidget*                 widget );
static void               foobar_limit_container_measure         ( GtkWidget*                 widget,
                                                                   GtkOrientation             orientation,
                                                                   int                        for_size,
                                                                   int*                       minimum,
                                                                   int*                       natural,
                                                                   int*                       minimum_baseline,
                                                                   int*                       natural_baseline );
static void               foobar_limit_container_size_allocate   ( GtkWidget*                 widget,
                                                                   int                        width,
                                                                   int                        height,
                                                                   int                        baseline );

G_DEFINE_FINAL_TYPE( FoobarLimitContainer, foobar_limit_container, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for limit containers.
//
void foobar_limit_container_class_init( FoobarLimitContainerClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	widget_klass->get_request_mode = foobar_limit_container_get_request_mode;
	widget_klass->measure = foobar_limit_container_measure;
	widget_klass->size_allocate = foobar_limit_container_size_allocate;
	gtk_widget_class_set_css_name( widget_klass, "limit-container" );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_limit_container_get_property;
	object_klass->set_property = foobar_limit_container_set_property;
	object_klass->dispose = foobar_limit_container_dispose;

	props[PROP_CHILD] = g_param_spec_object(
		"child",
		"Child",
		"Child widget for the panel item.",
		GTK_TYPE_WIDGET,
		G_PARAM_READWRITE );
	props[PROP_MAX_WIDTH] = g_param_spec_int(
		"max-width",
		"Max Width",
		"The maximum natural width supported as a minimum size.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_MAX_HEIGHT] = g_param_spec_int(
		"max-height",
		"Max Height",
		"The maximum natural height supported as a minimum size.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for limit containers.
//
void foobar_limit_container_init( FoobarLimitContainer* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_limit_container_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarLimitContainer* self = (FoobarLimitContainer*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			g_value_set_object( value, foobar_limit_container_get_child( self ) );
			break;
		case PROP_MAX_WIDTH:
			g_value_set_int( value, foobar_limit_container_get_max_width( self ) );
			break;
		case PROP_MAX_HEIGHT:
			g_value_set_int( value, foobar_limit_container_get_max_height( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_limit_container_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarLimitContainer* self = (FoobarLimitContainer*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			foobar_limit_container_set_child( self, g_value_get_object( value ) );
			break;
		case PROP_MAX_WIDTH:
			foobar_limit_container_set_max_width( self, g_value_get_int( value ) );
			break;
		case PROP_MAX_HEIGHT:
			foobar_limit_container_set_max_height( self, g_value_get_int( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for limit containers.
//
void foobar_limit_container_dispose( GObject* object )
{
	FoobarLimitContainer* self = (FoobarLimitContainer*)object;

	g_clear_pointer( &self->child, gtk_widget_unparent );

	G_OBJECT_CLASS( foobar_limit_container_parent_class )->dispose( object );
}

//
// Determine the size request mode used to measure the widget.
//
// This implementation just forwards the request mode from the child widget.
//
GtkSizeRequestMode foobar_limit_container_get_request_mode( GtkWidget* widget )
{
	FoobarLimitContainer* self = (FoobarLimitContainer*)widget;
	if ( !self->child ) { return GTK_SIZE_REQUEST_CONSTANT_SIZE; }

	return gtk_widget_get_request_mode( self->child );
}

//
// Compute the requested size of the widget.
//
void foobar_limit_container_measure(
	GtkWidget*     widget,
	GtkOrientation orientation,
	int            for_size,
	int*           minimum,
	int*           natural,
	int*           minimum_baseline,
	int*           natural_baseline )
{
	FoobarLimitContainer* self = (FoobarLimitContainer*)widget;
	if ( !self->child )
	{
		*minimum = 0;
		*natural = 0;
		*minimum_baseline = 0;
		*natural_baseline = 0;
		return;
	}

	gtk_widget_measure( self->child, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline );
	gint natural_minimum = *natural;
	switch ( orientation )
	{
		case GTK_ORIENTATION_HORIZONTAL:
			natural_minimum = MIN( natural_minimum, self->max_width );
			break;
		case GTK_ORIENTATION_VERTICAL:
			natural_minimum = MIN( natural_minimum, self->max_height );
			break;
		default:
			g_warn_if_reached( );
			break;
	}
	*minimum = MAX( *minimum, natural_minimum );
}

//
// Arrange the child widget.
//
void foobar_limit_container_size_allocate(
	GtkWidget* widget,
	int        width,
	int        height,
	int        baseline )
{
	FoobarLimitContainer* self = (FoobarLimitContainer*)widget;
	if ( !self->child ) { return; }

	gtk_widget_allocate( self->child, width, height, baseline, NULL );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new limit container instance.
//
GtkWidget* foobar_limit_container_new( void )
{
	return g_object_new( FOOBAR_TYPE_LIMIT_CONTAINER, NULL );
}

//
// Get the container's child widget.
//
GtkWidget* foobar_limit_container_get_child( FoobarLimitContainer* self )
{
	g_return_val_if_fail( FOOBAR_IS_LIMIT_CONTAINER( self ), NULL );
	return self->child;
}

//
// Get the maximum natural width of the child to use as the minimum width for this container.
//
gint foobar_limit_container_get_max_width( FoobarLimitContainer* self )
{
	g_return_val_if_fail( FOOBAR_IS_LIMIT_CONTAINER( self ), 0 );
	return self->max_width;
}

//
// Get the maximum natural height of the child to use as the minimum height for this container.
//
gint foobar_limit_container_get_max_height( FoobarLimitContainer* self )
{
	g_return_val_if_fail( FOOBAR_IS_LIMIT_CONTAINER( self ), 0 );
	return self->max_height;
}

//
// Update the container's child widget.
//
void foobar_limit_container_set_child(
	FoobarLimitContainer* self,
	GtkWidget*            child )
{
	g_return_if_fail( FOOBAR_IS_LIMIT_CONTAINER( self ) );
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
// Update the maximum natural width of the child to use as the minimum width for this container.
//
void foobar_limit_container_set_max_width(
	FoobarLimitContainer* self,
	gint                  value )
{
	g_return_if_fail( FOOBAR_IS_LIMIT_CONTAINER( self ) );
	g_return_if_fail( value >= 0 );

	if ( self->max_width != value )
	{
		self->max_width = value;
		gtk_widget_queue_resize( GTK_WIDGET( self ) );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_MAX_WIDTH] );
	}
}

//
// Update the maximum natural height of the child to use as the minimum height for this container.
//
void foobar_limit_container_set_max_height(
	FoobarLimitContainer* self,
	gint                  value )
{
	g_return_if_fail( FOOBAR_IS_LIMIT_CONTAINER( self ) );
	g_return_if_fail( value >= 0 );

	if ( self->max_height != value )
	{
		self->max_height = value;
		gtk_widget_queue_resize( GTK_WIDGET( self ) );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_MAX_HEIGHT] );
	}
}
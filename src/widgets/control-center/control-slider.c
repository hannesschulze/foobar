#include "widgets/control-center/control-slider.h"

//
// FoobarControlSlider:
//
// A slider in the "controls" section of the control center with a title and icon, as well as an optional secondary
// button to toggle a details view.
//

struct _FoobarControlSlider
{
	GtkWidget parent_instance;
	gchar*    icon_name;
	gchar*    label;
	gboolean  can_expand;
	gboolean  is_expanded;
	gint      percentage;
};

enum
{
	PROP_ICON_NAME = 1,
	PROP_LABEL,
	PROP_CAN_EXPAND,
	PROP_IS_EXPANDED,
	PROP_PERCENTAGE,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_control_slider_class_init           ( FoobarControlSliderClass* klass );
static void foobar_control_slider_init                 ( FoobarControlSlider*      self );
static void foobar_control_slider_get_property         ( GObject*                  object,
                                                         guint                     prop_id,
                                                         GValue*                   value,
                                                         GParamSpec*               pspec );
static void foobar_control_slider_set_property         ( GObject*                  object,
                                                         guint                     prop_id,
                                                         GValue const*             value,
                                                         GParamSpec*               pspec );
static void foobar_control_slider_dispose              ( GObject*                  object );
static void foobar_control_slider_finalize             ( GObject*                  object );
static void foobar_control_slider_handle_expand_clicked( GtkButton*                button,
                                                         gpointer                  userdata );

G_DEFINE_FINAL_TYPE( FoobarControlSlider, foobar_control_slider, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for control sliders.
//
void foobar_control_slider_class_init( FoobarControlSliderClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	gtk_widget_class_set_css_name( widget_klass, "control-slider" );
	gtk_widget_class_set_layout_manager_type( widget_klass, GTK_TYPE_BOX_LAYOUT );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_control_slider_get_property;
	object_klass->set_property = foobar_control_slider_set_property;
	object_klass->dispose = foobar_control_slider_dispose;
	object_klass->finalize = foobar_control_slider_finalize;

	props[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name",
		"Icon Name",
		"Name of the icon to be displayed next to the label.",
		NULL,
		G_PARAM_READWRITE );
	props[PROP_LABEL] = g_param_spec_string(
		"label",
		"Label",
		"The slider's main label.",
		NULL,
		G_PARAM_READWRITE );
	props[PROP_CAN_EXPAND] = g_param_spec_boolean(
		"can-expand",
		"Can Expand",
		"Indicates whether some details are hidden and can be revealed by a toggle button next to the slider.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_IS_EXPANDED] = g_param_spec_boolean(
		"is-expanded",
		"Is Expanded",
		"Indicates whether additional details should be shown.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_PERCENTAGE] = g_param_spec_int(
		"percentage",
		"Percentage",
		"Value shown by the slider.",
		0,
		100,
		0,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for control sliders.
//
void foobar_control_slider_init( FoobarControlSlider* self )
{
	GtkWidget* icon = gtk_image_new( );
	gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );
	g_object_bind_property(
		self,
		"icon-name",
		icon,
		"icon-name",
		G_BINDING_SYNC_CREATE );

	GtkWidget* label = gtk_label_new( NULL );
	gtk_widget_set_valign( label, GTK_ALIGN_CENTER );
	gtk_widget_set_hexpand( label, TRUE );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_LEFT );
	gtk_label_set_xalign( GTK_LABEL( label ), 0 );
	g_object_bind_property(
		self,
		"label",
		label,
		"label",
		G_BINDING_SYNC_CREATE );

	GtkWidget* header = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_append( GTK_BOX( header ), icon );
	gtk_box_append( GTK_BOX( header ), label );

	GtkWidget* scale = gtk_scale_new_with_range( GTK_ORIENTATION_HORIZONTAL, 0, 100, 2 );
	g_object_bind_property(
		self,
		"percentage",
		gtk_range_get_adjustment( GTK_RANGE( scale ) ),
		"value",
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL );

	GtkWidget* primary_content = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_add_css_class( primary_content, "primary" );
	gtk_box_append( GTK_BOX( primary_content ), header );
	gtk_box_append( GTK_BOX( primary_content ), scale );

	GtkWidget* expand_button = gtk_button_new_from_icon_name( "fluent-chevron-right-symbolic" );
	gtk_widget_add_css_class( expand_button, "expand" );
	g_object_bind_property(
		self,
		"can-expand",
		expand_button,
		"visible",
		G_BINDING_SYNC_CREATE );
	g_signal_connect( expand_button, "clicked", G_CALLBACK( foobar_control_slider_handle_expand_clicked ), self );

	GtkLayoutManager* layout = gtk_widget_get_layout_manager( GTK_WIDGET( self ) );
	gtk_orientable_set_orientation( GTK_ORIENTABLE( layout ), GTK_ORIENTATION_HORIZONTAL );
	gtk_widget_insert_before( primary_content, GTK_WIDGET( self ), NULL );
	gtk_widget_insert_before( expand_button, GTK_WIDGET( self ), NULL );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_control_slider_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarControlSlider* self = (FoobarControlSlider*)object;

	switch ( prop_id )
	{
		case PROP_ICON_NAME:
			g_value_set_string( value, foobar_control_slider_get_icon_name( self ) );
			break;
		case PROP_LABEL:
			g_value_set_string( value, foobar_control_slider_get_label( self ) );
			break;
		case PROP_CAN_EXPAND:
			g_value_set_boolean( value, foobar_control_slider_can_expand( self ) );
			break;
		case PROP_IS_EXPANDED:
			g_value_set_boolean( value, foobar_control_slider_is_expanded( self ) );
			break;
		case PROP_PERCENTAGE:
			g_value_set_int( value, foobar_control_slider_get_percentage( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_control_slider_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarControlSlider* self = (FoobarControlSlider*)object;

	switch ( prop_id )
	{
		case PROP_ICON_NAME:
			foobar_control_slider_set_icon_name( self, g_value_get_string( value ) );
			break;
		case PROP_LABEL:
			foobar_control_slider_set_label( self, g_value_get_string( value ) );
			break;
		case PROP_CAN_EXPAND:
			foobar_control_slider_set_can_expand( self, g_value_get_boolean( value ) );
			break;
		case PROP_IS_EXPANDED:
			foobar_control_slider_set_expanded( self, g_value_get_boolean( value ) );
			break;
		case PROP_PERCENTAGE:
			foobar_control_slider_set_percentage( self, g_value_get_int( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for control sliders.
//
void foobar_control_slider_dispose( GObject* object )
{
	FoobarControlSlider* self = (FoobarControlSlider*)object;

	GtkWidget* child;
	while ( ( child = gtk_widget_get_first_child( GTK_WIDGET( self ) ) ) )
	{
		gtk_widget_unparent( child );
	}

	G_OBJECT_CLASS( foobar_control_slider_parent_class )->dispose( object );
}

//
// Instance cleanup for control sliders.
//
void foobar_control_slider_finalize( GObject* object )
{
	FoobarControlSlider* self = (FoobarControlSlider*)object;

	g_clear_pointer( &self->icon_name, g_free );
	g_clear_pointer( &self->label, g_free );

	G_OBJECT_CLASS( foobar_control_slider_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new control slider instance.
//
GtkWidget* foobar_control_slider_new( void )
{
	return g_object_new( FOOBAR_TYPE_CONTROL_SLIDER, NULL );
}

//
// Get the name of the icon to be displayed next to the label.
//
gchar const* foobar_control_slider_get_icon_name( FoobarControlSlider* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ), NULL );
	return self->icon_name;
}

//
// Get the slider's main label.
//
gchar const* foobar_control_slider_get_label( FoobarControlSlider* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ), NULL );
	return self->label;
}

//
// Check whether some details are hidden and can be revealed by a toggle button next to the slider.
//
gboolean foobar_control_slider_can_expand( FoobarControlSlider* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ), FALSE );
	return self->can_expand;
}

//
// Check whether additional details should currently be shown.
//
gboolean foobar_control_slider_is_expanded( FoobarControlSlider* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ), FALSE );
	return self->is_expanded;
}

//
// Get the value shown by the slider.
//
gint foobar_control_slider_get_percentage( FoobarControlSlider* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ), FALSE );
	return self->percentage;
}

//
// Update the name of the icon to be displayed next to the label.
//
void foobar_control_slider_set_icon_name(
	FoobarControlSlider* self,
	gchar const*         value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ) );

	if ( g_strcmp0( self->icon_name, value ) )
	{
		g_clear_pointer( &self->icon_name, g_free );
		self->icon_name = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_ICON_NAME] );
	}
}

//
// Update the slider's main label.
//
void foobar_control_slider_set_label(
	FoobarControlSlider* self,
	gchar const*         value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ) );

	if ( g_strcmp0( self->label, value ) )
	{
		g_clear_pointer( &self->label, g_free );
		self->label = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_LABEL] );
	}
}

//
// Update whether some details are hidden and can be revealed by a toggle button next to the slider.
//
void foobar_control_slider_set_can_expand(
	FoobarControlSlider* self,
	gboolean             value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ) );

	if ( !value ) { foobar_control_slider_set_expanded( self, FALSE ); }

	value = !!value;
	if ( self->can_expand != value )
	{
		self->can_expand = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CAN_EXPAND] );
	}
}

//
// Update whether additional details are currently shown.
//
void foobar_control_slider_set_expanded(
	FoobarControlSlider* self,
	gboolean             value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ) );

	value = value && self->can_expand;
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
// Update the value shown by the slider.
//
void foobar_control_slider_set_percentage( FoobarControlSlider* self, gint value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_SLIDER( self ) );

	value = CLAMP( value, 0, 100 );
	if ( self->percentage != value )
	{
		self->percentage = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_PERCENTAGE] );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the expand button was clicked.
//
// If allowed, this will toggle the "expanded" state of the slider.
//
void foobar_control_slider_handle_expand_clicked( GtkButton* button, gpointer userdata )
{
	(void)button;
	FoobarControlSlider* self = (FoobarControlSlider*)userdata;

	if ( foobar_control_slider_can_expand( self ) )
	{
		foobar_control_slider_set_expanded( self, !foobar_control_slider_is_expanded( self ) );
	}
}
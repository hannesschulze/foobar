#include "widgets/control-center/control-button.h"

//
// FoobarControlButton:
//
// A button in the "controls" section of the control center. This can be a regular button or a toggle button with an
// optional secondary button to toggle a details view.
//

struct _FoobarControlButton
{
	GtkWidget parent_instance;
	gchar*    icon_name;
	gchar*    label;
	gboolean  can_expand;
	gboolean  can_toggle;
	gboolean  is_expanded;
	gboolean  is_toggled;
};

enum
{
	PROP_ICON_NAME = 1,
	PROP_LABEL,
	PROP_CAN_EXPAND,
	PROP_CAN_TOGGLE,
	PROP_IS_EXPANDED,
	PROP_IS_TOGGLED,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_control_button_class_init            ( FoobarControlButtonClass* klass );
static void foobar_control_button_init                  ( FoobarControlButton*      self );
static void foobar_control_button_get_property          ( GObject*                  object,
                                                          guint                     prop_id,
                                                          GValue*                   value,
                                                          GParamSpec*               pspec );
static void foobar_control_button_set_property          ( GObject*                  object,
                                                          guint                     prop_id,
                                                          GValue const*             value,
                                                          GParamSpec*               pspec );
static void foobar_control_button_dispose               ( GObject*                  object );
static void foobar_control_button_finalize              ( GObject*                  object );
static void foobar_control_button_handle_primary_clicked( GtkButton*                button,
                                                          gpointer                  userdata );
static void foobar_control_button_handle_expand_clicked ( GtkButton*                button,
                                                          gpointer                  userdata );

G_DEFINE_FINAL_TYPE( FoobarControlButton, foobar_control_button, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for control buttons.
//
void foobar_control_button_class_init( FoobarControlButtonClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	gtk_widget_class_set_css_name( widget_klass, "control-button" );
	gtk_widget_class_set_layout_manager_type( widget_klass, GTK_TYPE_BOX_LAYOUT );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_control_button_get_property;
	object_klass->set_property = foobar_control_button_set_property;
	object_klass->dispose = foobar_control_button_dispose;
	object_klass->finalize = foobar_control_button_finalize;

	props[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name",
		"Icon Name",
		"Name of the icon to be displayed next to the label.",
		NULL,
		G_PARAM_READWRITE );
	props[PROP_LABEL] = g_param_spec_string(
		"label",
		"Label",
		"The button's main label.",
		NULL,
		G_PARAM_READWRITE );
	props[PROP_CAN_EXPAND] = g_param_spec_boolean(
		"can-expand",
		"Can Expand",
		"Indicates whether some details are hidden and can be revealed by a toggle button next to the main action.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_CAN_TOGGLE] = g_param_spec_boolean(
		"can-toggle",
		"Can Toggle",
		"Indicates whether the main button can be toggled.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_IS_EXPANDED] = g_param_spec_boolean(
		"is-expanded",
		"Is Expanded",
		"Indicates whether additional details should be shown.",
		FALSE,
		G_PARAM_READWRITE );
	props[PROP_IS_TOGGLED] = g_param_spec_boolean(
		"is-toggled",
		"Is Toggled",
		"Indicates whether the main button is currently toggled.",
		FALSE,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for control buttons.
//
void foobar_control_button_init( FoobarControlButton* self )
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

	GtkWidget* content = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_append( GTK_BOX( content ), icon );
	gtk_box_append( GTK_BOX( content ), label );

	GtkWidget* primary_button = gtk_button_new( );
	gtk_widget_add_css_class( primary_button, "primary" );
	gtk_widget_set_hexpand( primary_button, TRUE );
	gtk_button_set_child( GTK_BUTTON( primary_button ), content );
	g_object_bind_property(
		self,
		"can-toggle",
		primary_button,
		"sensitive",
		G_BINDING_SYNC_CREATE );
	g_signal_connect( primary_button, "clicked", G_CALLBACK( foobar_control_button_handle_primary_clicked ), self );

	GtkWidget* expand_button = gtk_button_new_from_icon_name( "fluent-chevron-right-symbolic" );
	gtk_widget_add_css_class( expand_button, "expand" );
	g_object_bind_property(
		self,
		"can-expand",
		expand_button,
		"visible",
		G_BINDING_SYNC_CREATE );
	g_signal_connect( expand_button, "clicked", G_CALLBACK( foobar_control_button_handle_expand_clicked ), self );

	GtkLayoutManager* layout = gtk_widget_get_layout_manager( GTK_WIDGET( self ) );
	gtk_orientable_set_orientation( GTK_ORIENTABLE( layout ), GTK_ORIENTATION_HORIZONTAL );
	gtk_widget_insert_before( primary_button, GTK_WIDGET( self ), NULL );
	gtk_widget_insert_before( expand_button, GTK_WIDGET( self ), NULL );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_control_button_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarControlButton* self = (FoobarControlButton*)object;

	switch ( prop_id )
	{
		case PROP_ICON_NAME:
			g_value_set_string( value, foobar_control_button_get_icon_name( self ) );
			break;
		case PROP_LABEL:
			g_value_set_string( value, foobar_control_button_get_label( self ) );
			break;
		case PROP_CAN_EXPAND:
			g_value_set_boolean( value, foobar_control_button_can_expand( self ) );
			break;
		case PROP_CAN_TOGGLE:
			g_value_set_boolean( value, foobar_control_button_can_toggle( self ) );
			break;
		case PROP_IS_EXPANDED:
			g_value_set_boolean( value, foobar_control_button_is_expanded( self ) );
			break;
		case PROP_IS_TOGGLED:
			g_value_set_boolean( value, foobar_control_button_is_toggled( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_control_button_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarControlButton* self = (FoobarControlButton*)object;

	switch ( prop_id )
	{
		case PROP_ICON_NAME:
			foobar_control_button_set_icon_name( self, g_value_get_string( value ) );
			break;
		case PROP_LABEL:
			foobar_control_button_set_label( self, g_value_get_string( value ) );
			break;
		case PROP_CAN_EXPAND:
			foobar_control_button_set_can_expand( self, g_value_get_boolean( value ) );
			break;
		case PROP_CAN_TOGGLE:
			foobar_control_button_set_can_toggle( self, g_value_get_boolean( value ) );
			break;
		case PROP_IS_EXPANDED:
			foobar_control_button_set_expanded( self, g_value_get_boolean( value ) );
			break;
		case PROP_IS_TOGGLED:
			foobar_control_button_set_toggled( self, g_value_get_boolean( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for control buttons.
//
void foobar_control_button_dispose( GObject* object )
{
	FoobarControlButton* self = (FoobarControlButton*)object;

	GtkWidget* child;
	while ( ( child = gtk_widget_get_first_child( GTK_WIDGET( self ) ) ) )
	{
		gtk_widget_unparent( child );
	}

	G_OBJECT_CLASS( foobar_control_button_parent_class )->dispose( object );
}

//
// Instance cleanup for control buttons.
//
void foobar_control_button_finalize( GObject* object )
{
	FoobarControlButton* self = (FoobarControlButton*)object;

	g_clear_pointer( &self->icon_name, g_free );
	g_clear_pointer( &self->label, g_free );

	G_OBJECT_CLASS( foobar_control_button_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new control button instance.
//
GtkWidget* foobar_control_button_new( void )
{
	return g_object_new( FOOBAR_TYPE_CONTROL_BUTTON, NULL );
}

//
// Get the name of the icon to be displayed next to the label.
//
gchar const* foobar_control_button_get_icon_name( FoobarControlButton* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ), NULL );
	return self->icon_name;
}

//
// Get the button's main label.
//
gchar const* foobar_control_button_get_label( FoobarControlButton* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ), NULL );
	return self->label;
}

//
// Check whether some details are hidden and can be revealed by a toggle button next to the main action.
//
gboolean foobar_control_button_can_expand( FoobarControlButton* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ), FALSE );
	return self->can_expand;
}

//
// Check whether the main button can be toggled.
//
gboolean foobar_control_button_can_toggle( FoobarControlButton* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ), FALSE );
	return self->can_toggle;
}

//
// Check whether additional details should currently be shown.
//
gboolean foobar_control_button_is_expanded( FoobarControlButton* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ), FALSE );
	return self->is_expanded;
}

//
// Check whether the main button is currently toggled.
//
gboolean foobar_control_button_is_toggled( FoobarControlButton* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ), FALSE );
	return self->is_toggled;
}

//
// Update the name of the icon to be displayed next to the label.
//
void foobar_control_button_set_icon_name(
	FoobarControlButton* self,
	gchar const*         value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ) );

	if ( g_strcmp0( self->icon_name, value ) )
	{
		g_clear_pointer( &self->icon_name, g_free );
		self->icon_name = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_ICON_NAME] );
	}
}

//
// Update the button's main label.
//
void foobar_control_button_set_label(
	FoobarControlButton* self,
	gchar const*         value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ) );

	if ( g_strcmp0( self->label, value ) )
	{
		g_clear_pointer( &self->label, g_free );
		self->label = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_LABEL] );
	}
}

//
// Update whether some details are hidden and can be revealed by a toggle button next to the main action.
//
void foobar_control_button_set_can_expand(
	FoobarControlButton* self,
	gboolean             value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ) );

	if ( !value ) { foobar_control_button_set_expanded( self, FALSE ); }

	value = !!value;
	if ( self->can_expand != value )
	{
		self->can_expand = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CAN_EXPAND] );
	}
}

//
// Update whether the main button can be toggled.
//
void foobar_control_button_set_can_toggle(
	FoobarControlButton* self,
	gboolean             value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ) );

	if ( !value ) { foobar_control_button_set_toggled( self, FALSE ); }

	value = !!value;
	if ( self->can_toggle != value )
	{
		self->can_toggle = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CAN_TOGGLE] );
	}
}

//
// Update whether additional details are currently shown.
//
void foobar_control_button_set_expanded(
	FoobarControlButton* self,
	gboolean             value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ) );

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
// Update whether the main button is currently toggled.
//
void foobar_control_button_set_toggled(
	FoobarControlButton* self,
	gboolean             value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_BUTTON( self ) );

	value = value && self->can_toggle;
	if ( self->is_toggled != value )
	{
		self->is_toggled = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_TOGGLED] );

		if ( self->is_toggled )
		{
			gtk_widget_add_css_class( GTK_WIDGET( self ), "toggled" );
		}
		else
		{
			gtk_widget_remove_css_class( GTK_WIDGET( self ), "toggled" );
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the primary button was clicked.
//
// If allowed, this will toggle the button.
//
void foobar_control_button_handle_primary_clicked( GtkButton* button, gpointer userdata )
{
	(void)button;
	FoobarControlButton* self = (FoobarControlButton*)userdata;

	if ( foobar_control_button_can_toggle( self ) )
	{
		foobar_control_button_set_toggled( self, !foobar_control_button_is_toggled( self ) );
	}
}

//
// Called when the expand button was clicked.
//
// If allowed, this will toggle the "expanded" state of the button.
//
void foobar_control_button_handle_expand_clicked( GtkButton* button, gpointer userdata )
{
	(void)button;
	FoobarControlButton* self = (FoobarControlButton*)userdata;

	if ( foobar_control_button_can_expand( self ) )
	{
		foobar_control_button_set_expanded( self, !foobar_control_button_is_expanded( self ) );
	}
}

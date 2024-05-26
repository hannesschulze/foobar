#include "widgets/control-center/control-details-item.h"

//
// FoobarControlDetailsAccessory:
//
// The accessory shown next to an item in the control center details list (if any).
//

G_DEFINE_ENUM_TYPE(
	FoobarControlDetailsAccessory,
	foobar_control_details_accessory,
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_DETAILS_ACCESSORY_NONE, "none" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_DETAILS_ACCESSORY_PROGRESS, "progress" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_CONTROL_DETAILS_ACCESSORY_CHECKED, "checked" ) )

//
// FoobarControlDetailsItem:
//
// Standard row layout for items in a FoobarControlDetails list. The rows are made up of a label and an accessory that
// is shown or hidden depending on the item's state.
//

struct _FoobarControlDetailsItem
{
	GtkWidget                     parent_instance;
	GtkWidget*                    spinner;
	GtkWidget*                    checkmark;
	gchar*                        label;
	FoobarControlDetailsAccessory accessory;
};

enum
{
	PROP_LABEL = 1,
	PROP_ACCESSORY,
	PROP_IS_CHECKED,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_control_details_item_class_init  ( FoobarControlDetailsItemClass* klass );
static void foobar_control_details_item_init        ( FoobarControlDetailsItem*      self );
static void foobar_control_details_item_get_property( GObject*                       object,
                                                      guint                          prop_id,
                                                      GValue*                        value,
                                                      GParamSpec*                    pspec );
static void foobar_control_details_item_set_property( GObject*                       object,
                                                      guint                          prop_id,
                                                      GValue const*                  value,
                                                      GParamSpec*                    pspec );
static void foobar_control_details_item_dispose     ( GObject*                       object );
static void foobar_control_details_item_finalize    ( GObject*                       object );

G_DEFINE_FINAL_TYPE( FoobarControlDetailsItem, foobar_control_details_item, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for control details items.
//
void foobar_control_details_item_class_init( FoobarControlDetailsItemClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	gtk_widget_class_set_css_name( widget_klass, "control-details-item" );
	gtk_widget_class_set_layout_manager_type( widget_klass, GTK_TYPE_BOX_LAYOUT );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_control_details_item_get_property;
	object_klass->set_property = foobar_control_details_item_set_property;
	object_klass->dispose = foobar_control_details_item_dispose;
	object_klass->finalize = foobar_control_details_item_finalize;

	props[PROP_LABEL] = g_param_spec_string(
		"label",
		"Label",
		"Label shown for the item.",
		NULL,
		G_PARAM_READWRITE );
	props[PROP_ACCESSORY] = g_param_spec_enum(
		"accessory",
		"Accessory",
		"Accessory shown for the item.",
		FOOBAR_TYPE_CONTROL_DETAILS_ACCESSORY,
		0,
		G_PARAM_READWRITE );
	props[PROP_IS_CHECKED] = g_param_spec_boolean(
		"is-checked",
		"Is Checked",
		"Current checked-state for the item.",
		FALSE,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for control details items.
//
void foobar_control_details_item_init( FoobarControlDetailsItem* self )
{
	GtkWidget* label = gtk_label_new( NULL );
	gtk_widget_set_hexpand( label, TRUE );
	gtk_widget_set_valign( label, GTK_ALIGN_CENTER );
	gtk_label_set_ellipsize( GTK_LABEL( label ), PANGO_ELLIPSIZE_END );
	gtk_label_set_justify( GTK_LABEL( label ), GTK_JUSTIFY_LEFT );
	gtk_label_set_xalign( GTK_LABEL( label ), 0 );
	gtk_label_set_wrap( GTK_LABEL( label ), FALSE );
	g_object_bind_property(
		self,
		"label",
		label,
		"label",
		G_BINDING_SYNC_CREATE );

	self->spinner = gtk_spinner_new( );
	gtk_spinner_set_spinning( GTK_SPINNER( self->spinner ), TRUE );
	gtk_widget_add_css_class( self->spinner, "accessory" );
	gtk_widget_set_valign( self->spinner, GTK_ALIGN_CENTER );
	gtk_widget_set_visible( self->spinner, FALSE );

	self->checkmark = gtk_image_new_from_icon_name( "fluent-checkmark-symbolic" );
	gtk_widget_add_css_class( self->checkmark, "checkmark" );
	gtk_widget_add_css_class( self->checkmark, "accessory" );
	gtk_widget_set_valign( self->checkmark, GTK_ALIGN_CENTER );
	gtk_widget_set_visible( self->checkmark, FALSE );

	gtk_widget_insert_before( label, GTK_WIDGET( self ), NULL );
	gtk_widget_insert_before( self->spinner, GTK_WIDGET( self ), NULL );
	gtk_widget_insert_before( self->checkmark, GTK_WIDGET( self ), NULL );
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_control_details_item_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarControlDetailsItem* self = (FoobarControlDetailsItem*)object;

	switch ( prop_id )
	{
		case PROP_LABEL:
			g_value_set_string( value, foobar_control_details_item_get_label( self ) );
			break;
		case PROP_ACCESSORY:
			g_value_set_enum( value, foobar_control_details_item_get_accessory( self ) );
			break;
		case PROP_IS_CHECKED:
			g_value_set_boolean( value, foobar_control_details_item_is_checked( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_control_details_item_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarControlDetailsItem* self = (FoobarControlDetailsItem*)object;

	switch ( prop_id )
	{
		case PROP_LABEL:
			foobar_control_details_item_set_label( self, g_value_get_string( value ) );
			break;
		case PROP_ACCESSORY:
			foobar_control_details_item_set_accessory( self, g_value_get_enum( value ) );
			break;
		case PROP_IS_CHECKED:
			foobar_control_details_item_set_checked( self, g_value_get_boolean( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for control details items.
//
void foobar_control_details_item_dispose( GObject* object )
{
	FoobarControlDetailsItem* self = (FoobarControlDetailsItem*)object;

	GtkWidget* child;
	while ( ( child = gtk_widget_get_first_child( GTK_WIDGET( self ) ) ) )
	{
		gtk_widget_unparent( child );
	}

	G_OBJECT_CLASS( foobar_control_details_item_parent_class )->dispose( object );
}

//
// Instance cleanup for control details items.
//
void foobar_control_details_item_finalize( GObject* object )
{
	FoobarControlDetailsItem* self = (FoobarControlDetailsItem*)object;

	g_clear_pointer( &self->label, g_free );

	G_OBJECT_CLASS( foobar_control_details_item_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new control details item instance.
//
GtkWidget* foobar_control_details_item_new( void )
{
	return g_object_new( FOOBAR_TYPE_CONTROL_DETAILS_ITEM, NULL );
}

//
// Get the label shown for the item.
//
gchar const* foobar_control_details_item_get_label( FoobarControlDetailsItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_DETAILS_ITEM( self ), NULL );
	return self->label;
}

//
// Get the accessory shown for the item.
//
FoobarControlDetailsAccessory foobar_control_details_item_get_accessory( FoobarControlDetailsItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_DETAILS_ITEM( self ), 0 );
	return self->accessory;
}

//
// Get the current checked-state for the item.
//
gboolean foobar_control_details_item_is_checked( FoobarControlDetailsItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_CONTROL_DETAILS_ITEM( self ), FALSE );
	return self->accessory == FOOBAR_CONTROL_DETAILS_ACCESSORY_CHECKED;
}

//
// Update the label shown for the item.
//
void foobar_control_details_item_set_label(
	FoobarControlDetailsItem* self,
	gchar const*              value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_DETAILS_ITEM( self ) );

	if ( g_strcmp0( self->label, value ) )
	{
		g_clear_pointer( &self->label, g_free );
		self->label = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_LABEL] );
	}
}

//
// Update the current accessory shown for the item.
//
void foobar_control_details_item_set_accessory(
	FoobarControlDetailsItem*     self,
	FoobarControlDetailsAccessory value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_DETAILS_ITEM( self ) );

	if ( self->accessory != value )
	{
		self->accessory = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_ACCESSORY] );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_IS_CHECKED] );

		gtk_widget_set_visible( self->spinner, self->accessory == FOOBAR_CONTROL_DETAILS_ACCESSORY_PROGRESS );
		gtk_widget_set_visible( self->checkmark, self->accessory == FOOBAR_CONTROL_DETAILS_ACCESSORY_CHECKED );
	}
}

//
// Update the current checked-state for the item.
//
void foobar_control_details_item_set_checked(
	FoobarControlDetailsItem* self,
	gboolean                  value )
{
	g_return_if_fail( FOOBAR_IS_CONTROL_DETAILS_ITEM( self ) );

	FoobarControlDetailsAccessory accessory = value
		? FOOBAR_CONTROL_DETAILS_ACCESSORY_CHECKED
		: FOOBAR_CONTROL_DETAILS_ACCESSORY_NONE;
	foobar_control_details_item_set_accessory( self, accessory );
}

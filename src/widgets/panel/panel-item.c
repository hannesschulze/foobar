#include "widgets/panel/panel-item.h"
#include "application.h"

//
// FoobarPanelItem:
//
// Base class for all panel items. This is basically a bin layout where the subclass usually sets the child widget.
//

typedef struct _FoobarPanelItemPrivate
{
	GtkWidget* child;
} FoobarPanelItemPrivate;

enum
{
	PROP_CHILD = 1,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void foobar_panel_item_class_init  ( FoobarPanelItemClass* klass );
static void foobar_panel_item_init        ( FoobarPanelItem*      self );
static void foobar_panel_item_get_property( GObject*              object,
                                            guint                 prop_id,
                                            GValue*               value,
                                            GParamSpec*           pspec );
static void foobar_panel_item_set_property( GObject*              object,
                                            guint                 prop_id,
                                            GValue const*         value,
                                            GParamSpec*           pspec );
static void foobar_panel_item_dispose     ( GObject*              object );

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE( FoobarPanelItem, foobar_panel_item, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for panel items.
//
void foobar_panel_item_class_init( FoobarPanelItemClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	gtk_widget_class_set_css_name( widget_klass, "panel-item" );
	gtk_widget_class_set_layout_manager_type( widget_klass, GTK_TYPE_BIN_LAYOUT );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_panel_item_get_property;
	object_klass->set_property = foobar_panel_item_set_property;
	object_klass->dispose = foobar_panel_item_dispose;

	props[PROP_CHILD] = g_param_spec_object(
		"child",
		"Child",
		"Child widget for the panel item.",
		GTK_TYPE_WIDGET,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for panel items.
//
void foobar_panel_item_init( FoobarPanelItem* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_panel_item_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarPanelItem* self = (FoobarPanelItem*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			g_value_set_object( value, foobar_panel_item_get_child( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_panel_item_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarPanelItem* self = (FoobarPanelItem*)object;

	switch ( prop_id )
	{
		case PROP_CHILD:
			foobar_panel_item_set_child( self, g_value_get_object( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for panel items.
//
void foobar_panel_item_dispose( GObject* object )
{
	FoobarPanelItem*        self = (FoobarPanelItem*)object;
	FoobarPanelItemPrivate* priv = foobar_panel_item_get_instance_private( self );

	g_clear_pointer( &priv->child, gtk_widget_unparent );

	G_OBJECT_CLASS( foobar_panel_item_parent_class )->dispose( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Get the child widget for the panel item.
//
GtkWidget* foobar_panel_item_get_child( FoobarPanelItem* self )
{
	g_return_val_if_fail( FOOBAR_IS_PANEL_ITEM( self ), NULL );
	FoobarPanelItemPrivate* priv = foobar_panel_item_get_instance_private( self );

	return priv->child;
}

//
// Update the child widget for the panel item.
//
void foobar_panel_item_set_child(
	FoobarPanelItem* self,
	GtkWidget*       child )
{
	g_return_if_fail( FOOBAR_IS_PANEL_ITEM( self ) );
	FoobarPanelItemPrivate* priv = foobar_panel_item_get_instance_private( self );
	g_return_if_fail( child == NULL || priv->child == child || gtk_widget_get_parent( child ) == NULL );

	if ( priv->child != child )
	{
		g_clear_pointer( &priv->child, gtk_widget_unparent );

		if ( child )
		{
			priv->child = child;
			gtk_widget_set_parent( child, GTK_WIDGET( self ) );
		}

		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CHILD] );
	}
}

//
// Invoke the action for the panel item.
//
// This needs to be triggered by a subclass, for example by adding a button to the item. Support for panel item actions
// is opt-in and subclasses have to explicitly implement it. This is just a helper for implementing the action.
//
void foobar_panel_item_invoke_action(
	FoobarPanelItem*      self,
	FoobarPanelItemAction action )
{
	g_return_if_fail( FOOBAR_IS_PANEL_ITEM( self ) );

	switch ( action )
	{
		case FOOBAR_PANEL_ITEM_ACTION_NONE:
			break;
		case FOOBAR_PANEL_ITEM_ACTION_LAUNCHER:
			foobar_application_toggle_launcher( FOOBAR_APPLICATION_DEFAULT );
			break;
		case FOOBAR_PANEL_ITEM_ACTION_CONTROL_CENTER:
			foobar_application_toggle_control_center( FOOBAR_APPLICATION_DEFAULT );
			break;
		default:
			g_warn_if_reached( );
			break;
	}
}

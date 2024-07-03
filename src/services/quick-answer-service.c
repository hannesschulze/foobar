#include "services/quick-answer-service.h"
#include "services/quick-answers/math.h"
#include "launcher-item.h"
#include <gdk/gdk.h>

struct _FoobarQuickAnswer
{
	GObject parent_instance;
	gchar* title;
	gchar* value;
	GIcon* icon;
};

enum
{
	ANSWER_PROP_VALUE = 1,
	ANSWER_PROP_TITLE,
	ANSWER_PROP_DESCRIPTION,
	ANSWER_PROP_ICON,
	N_ANSWER_PROPS,
};

static GParamSpec* answer_props[N_ANSWER_PROPS] = { 0 };

static void                   foobar_quick_answer_class_init                  ( FoobarQuickAnswerClass*      klass );
static void                   foobar_quick_answer_launcher_item_interface_init( FoobarLauncherItemInterface* iface );
static void                   foobar_quick_answer_init                        ( FoobarQuickAnswer*           self );
static void                   foobar_quick_answer_get_property                ( GObject*                     object,
                                                                                guint                        prop_id,
                                                                                GValue*                      value,
                                                                                GParamSpec*                  pspec );
static void                   foobar_quick_answer_finalize                    ( GObject*                     object );
static FoobarQuickAnswer*     foobar_quick_answer_new                         ( void );
static gchar const*           foobar_quick_answer_get_title                   ( FoobarLauncherItem*          self );
static gchar const*           foobar_quick_answer_get_description             ( FoobarLauncherItem*          self );
static GIcon*                 foobar_quick_answer_get_icon                    ( FoobarLauncherItem*          self );
static void                   foobar_quick_answer_activate                    ( FoobarLauncherItem*          self );
static void                   foobar_quick_answer_set_title                   ( FoobarQuickAnswer*           self,
                                                                                gchar const*                 value );
static void                   foobar_quick_answer_set_value                   ( FoobarQuickAnswer*           self,
                                                                                gchar const*                 value );
static void                   foobar_quick_answer_set_icon                    ( FoobarQuickAnswer*           self,
                                                                                GIcon*                       value );

G_DEFINE_FINAL_TYPE_WITH_CODE(
	FoobarQuickAnswer,
	foobar_quick_answer,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE( FOOBAR_TYPE_LAUNCHER_ITEM, foobar_quick_answer_launcher_item_interface_init ) )

struct _FoobarQuickAnswerService
{
	GObject parent_instance;
};

static void foobar_quick_answer_service_class_init( FoobarQuickAnswerServiceClass* klass );
static void foobar_quick_answer_service_init      ( FoobarQuickAnswerService*      self );

G_DEFINE_FINAL_TYPE( FoobarQuickAnswerService, foobar_quick_answer_service, G_TYPE_OBJECT )

// ---------------------------------------------------------------------------------------------------------------------
// Quick Answers
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for quick answers.
//
void foobar_quick_answer_class_init( FoobarQuickAnswerClass* klass )
{
	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_quick_answer_get_property;
	object_klass->finalize = foobar_quick_answer_finalize;

	gpointer launcher_item_iface = g_type_default_interface_peek( FOOBAR_TYPE_LAUNCHER_ITEM );
	answer_props[ANSWER_PROP_VALUE] = g_param_spec_string(
		"value",
		"Value",
		"The raw value of the quick answer.",
		NULL,
		G_PARAM_READABLE );
	answer_props[ANSWER_PROP_TITLE] = g_param_spec_override(
		"title",
		g_object_interface_find_property( launcher_item_iface, "title" ) );
	answer_props[ANSWER_PROP_DESCRIPTION] = g_param_spec_override(
		"description",
		g_object_interface_find_property( launcher_item_iface, "description" ) );
	answer_props[ANSWER_PROP_ICON] = g_param_spec_override(
		"icon",
		g_object_interface_find_property( launcher_item_iface, "icon" ) );
	g_object_class_install_properties( object_klass, N_ANSWER_PROPS, answer_props );
}

//
// Static initialization of the FoobarLauncherItem interface.
//
void foobar_quick_answer_launcher_item_interface_init( FoobarLauncherItemInterface* iface )
{
	iface->get_title = foobar_quick_answer_get_title;
	iface->get_description = foobar_quick_answer_get_description;
	iface->get_icon = foobar_quick_answer_get_icon;
	iface->activate = foobar_quick_answer_activate;
}

//
// Instance initialization for quick answers.
//
void foobar_quick_answer_init( FoobarQuickAnswer* self )
{
	(void)self;
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_quick_answer_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarQuickAnswer* self = (FoobarQuickAnswer*)object;

	switch ( prop_id )
	{
		case ANSWER_PROP_VALUE:
			g_value_set_string( value, foobar_quick_answer_get_value( self ) );
			break;
		case ANSWER_PROP_TITLE:
			g_value_set_string( value, foobar_launcher_item_get_title( FOOBAR_LAUNCHER_ITEM ( self ) ) );
			break;
		case ANSWER_PROP_DESCRIPTION:
			g_value_set_string( value, foobar_launcher_item_get_description( FOOBAR_LAUNCHER_ITEM( self ) ) );
			break;
		case ANSWER_PROP_ICON:
			g_value_set_object( value, foobar_launcher_item_get_icon( FOOBAR_LAUNCHER_ITEM( self ) ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance cleanup for quick answers.
//
void foobar_quick_answer_finalize( GObject* object )
{
	FoobarQuickAnswer* self = (FoobarQuickAnswer*)object;

	g_clear_pointer( &self->title, g_free );
	g_clear_pointer( &self->value, g_free );
	g_clear_object( &self->icon );

	G_OBJECT_CLASS( foobar_quick_answer_parent_class )->finalize( object );
}

//
// Create a new, default-initialized quick answer.
//
FoobarQuickAnswer* foobar_quick_answer_new( void )
{
	return g_object_new( FOOBAR_TYPE_QUICK_ANSWER, NULL );
}

//
// Get the raw value of the quick answer.
//
gchar const* foobar_quick_answer_get_value( FoobarQuickAnswer* self )
{
	g_return_val_if_fail( FOOBAR_IS_QUICK_ANSWER( self ), NULL );
	return self->value;
}

//
// Get the title for the quick answer.
//
gchar const* foobar_quick_answer_get_title( FoobarLauncherItem* item )
{
	FoobarQuickAnswer* self = ( FoobarQuickAnswer* )item;
	return self->title;
}

//
// Get the description shown below the title, always NULL for quick answers.
//
gchar const* foobar_quick_answer_get_description( FoobarLauncherItem* item )
{
	(void)item;
	return NULL;
}

//
// Get an icon representing the quick answer.
//
GIcon* foobar_quick_answer_get_icon( FoobarLauncherItem* item )
{
	FoobarQuickAnswer* self = ( FoobarQuickAnswer* )item;
	return self->icon;
}

//
// Copy the quick answer's value into the clipboard.
//
void foobar_quick_answer_activate( FoobarLauncherItem* item )
{
	FoobarQuickAnswer* self = ( FoobarQuickAnswer* )item;
	if ( self->value )
	{
		GdkClipboard* clipboard = gdk_display_get_clipboard( gdk_display_get_default( ) );
		gdk_clipboard_set_text( clipboard, self->value );
	}
}

//
// Update the raw value of the quick answer.
//
void foobar_quick_answer_set_value(
	FoobarQuickAnswer* self,
	gchar const*       value )
{
	g_return_if_fail( FOOBAR_IS_QUICK_ANSWER( self ) );

	if ( g_strcmp0( self->value, value ) )
	{
		g_clear_pointer( &self->value, g_free );
		self->value = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), answer_props[ANSWER_PROP_VALUE] );
	}
}

//
// Update the title for the quick answer.
//
void foobar_quick_answer_set_title(
	FoobarQuickAnswer* self,
	gchar const*       value )
{
	g_return_if_fail( FOOBAR_IS_QUICK_ANSWER( self ) );

	if ( g_strcmp0( self->title, value ) )
	{
		g_clear_pointer( &self->title, g_free );
		self->title = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), answer_props[ANSWER_PROP_TITLE] );
	}
}

//
// Update the icon representing the quick answer.
//
void foobar_quick_answer_set_icon(
	FoobarQuickAnswer* self,
	GIcon*             value )
{
	g_return_if_fail( FOOBAR_IS_QUICK_ANSWER( self ) );

	if ( self->icon != value )
	{
		g_clear_object( &self->icon );
		self->icon = value ? g_object_ref( value ) : NULL;
		g_object_notify_by_pspec( G_OBJECT( self ), answer_props[ANSWER_PROP_ICON] );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Service Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for the quick answer service.
//
void foobar_quick_answer_service_class_init( FoobarQuickAnswerServiceClass* klass )
{
	(void)klass;
}

//
// Instance initialization for the quick answer service.
//
void foobar_quick_answer_service_init( FoobarQuickAnswerService* self )
{
	(void)self;
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new quick answer service instance.
//
FoobarQuickAnswerService* foobar_quick_answer_service_new( void )
{
	return g_object_new( FOOBAR_TYPE_QUICK_ANSWER_SERVICE, NULL );
}

//
// Query the service for a quick answer.
//
FoobarQuickAnswer* foobar_quick_answer_service_query(
	FoobarQuickAnswerService* self,
	gchar const*              query )
{
	(void)self;

	gsize token_count;
	g_autofree FoobarMathToken* tokens = foobar_math_lex( query, strlen( query ), &token_count );
	if ( tokens )
	{
		FoobarMathExpression* expr = foobar_math_parse( tokens, token_count );
		if ( expr )
		{
			g_print( "---\n" );
			foobar_math_expression_print( expr, 0 );
			foobar_math_expression_free( expr );
		}
	}

	(void)foobar_quick_answer_new;
	(void)foobar_quick_answer_set_value;
	(void)foobar_quick_answer_set_icon;
	(void)foobar_quick_answer_set_title;

	return NULL;
}

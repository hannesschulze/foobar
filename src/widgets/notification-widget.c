#include "widgets/notification-widget.h"

//
// FoobarNotificationCloseAction:
//
// The action invoked when the user requests to "close" the notification. The notification is either marked as
// "dismissed" and kept around or removed from the list.
//

G_DEFINE_ENUM_TYPE(
	FoobarNotificationCloseAction,
	foobar_notification_close_action,
	G_DEFINE_ENUM_VALUE( FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_REMOVE, "remove" ),
	G_DEFINE_ENUM_VALUE( FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_DISMISS, "dismiss" ) )

//
// FoobarNotificationWidget:
//
// Shared widget for displaying a notification. The widget also manages an inset/margin -- this is done to allow the
// "close" button to extend into the margin area.
//

struct _FoobarNotificationWidget
{
	GtkWidget                     parent_instance;
	GtkWidget*                    container;
	GtkWidget*                    content;
	GtkWidget*                    close_button;
	FoobarNotification*           notification;
	FoobarNotificationCloseAction close_action;
	gchar*                        time_format;
	gint                          min_height;
	gint                          close_button_inset;
	gint                          inset_start;
	gint                          inset_end;
	gint                          inset_top;
	gint                          inset_bottom;
};

enum
{
	PROP_NOTIFICATION = 1,
	PROP_CLOSE_ACTION,
	PROP_TIME_FORMAT,
	PROP_MIN_HEIGHT,
	PROP_CLOSE_BUTTON_INSET,
	PROP_INSET_START,
	PROP_INSET_END,
	PROP_INSET_TOP,
	PROP_INSET_BOTTOM,
	N_PROPS,
};

static GParamSpec* props[N_PROPS] = { 0 };

static void     foobar_notification_widget_class_init          ( FoobarNotificationWidgetClass* klass );
static void     foobar_notification_widget_init                ( FoobarNotificationWidget*      self );
static void     foobar_notification_widget_get_property        ( GObject*                       object,
                                                                 guint                          prop_id,
                                                                 GValue*                        value,
                                                                 GParamSpec*                    pspec );
static void     foobar_notification_widget_set_property        ( GObject*                       object,
                                                                 guint                          prop_id,
                                                                 GValue const*                  value,
                                                                 GParamSpec*                    pspec );
static void     foobar_notification_widget_dispose             ( GObject*                       object );
static void     foobar_notification_widget_finalize            ( GObject*                       object );
static void     foobar_notification_widget_handle_enter        ( GtkEventControllerMotion*      controller,
                                                                 gdouble                        x,
                                                                 gdouble                        y,
                                                                 gpointer                       userdata );
static void     foobar_notification_widget_handle_leave        ( GtkEventControllerMotion*      controller,
                                                                 gpointer                       userdata );
static void     foobar_notification_widget_handle_close_clicked( GtkButton*                     button,
                                                                 gpointer                       userdata );
static gchar*   foobar_notification_widget_compute_time_label  ( GtkExpression*                 expression,
                                                                 GDateTime*                     time,
                                                                 gchar const*                   format,
                                                                 gpointer                       userdata );
static gboolean foobar_notification_widget_compute_icon_visible( GtkExpression*                 expression,
                                                                 GIcon*                         icon,
                                                                 gpointer                       userdata );
static gboolean foobar_notification_widget_compute_body_visible( GtkExpression*                 expression,
                                                                 gchar const*                   body,
                                                                 gpointer                       userdata );
static void     foobar_notification_widget_update_margins      ( FoobarNotificationWidget*      self );

G_DEFINE_FINAL_TYPE( FoobarNotificationWidget, foobar_notification_widget, GTK_TYPE_WIDGET )

// ---------------------------------------------------------------------------------------------------------------------
// Widget Implementation
// ---------------------------------------------------------------------------------------------------------------------

//
// Static initialization for notifications.
//
void foobar_notification_widget_class_init( FoobarNotificationWidgetClass* klass )
{
	GtkWidgetClass* widget_klass = GTK_WIDGET_CLASS( klass );
	gtk_widget_class_set_layout_manager_type( widget_klass, GTK_TYPE_BIN_LAYOUT );
	gtk_widget_class_set_css_name( widget_klass, "notification" );

	GObjectClass* object_klass = G_OBJECT_CLASS( klass );
	object_klass->get_property = foobar_notification_widget_get_property;
	object_klass->set_property = foobar_notification_widget_set_property;
	object_klass->dispose = foobar_notification_widget_dispose;
	object_klass->finalize = foobar_notification_widget_finalize;

	props[PROP_NOTIFICATION] = g_param_spec_object(
		"notification",
		"Notification",
		"Content to display.",
		FOOBAR_TYPE_NOTIFICATION,
		G_PARAM_READWRITE );
	props[PROP_CLOSE_ACTION] = g_param_spec_enum(
		"close-action",
		"Close Action",
		"Action to execute when the notification is closed.",
		FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION,
		0,
		G_PARAM_READWRITE );
	props[PROP_TIME_FORMAT] = g_param_spec_string(
		"time-format",
		"Time Format",
		"Time format used to display the notification's timestamp.",
		NULL,
		G_PARAM_READWRITE );
	props[PROP_MIN_HEIGHT] = g_param_spec_int(
		"min-height",
		"Minimum Height",
		"Minimum requested height for the notification itself.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_CLOSE_BUTTON_INSET] = g_param_spec_int(
		"close-button-inset",
		"Close Button Inset",
		"Offset of the close button within the notification (may be negative, possibly increasing the actual inset).",
		INT_MIN,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_INSET_START] = g_param_spec_int(
		"inset-start",
		"Inset Start",
		"Inset for the start edge of the widget.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_INSET_END] = g_param_spec_int(
		"inset-end",
		"Inset End",
		"Inset for the end edge of the widget.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_INSET_TOP] = g_param_spec_int(
		"inset-top",
		"Inset Top",
		"Inset for the top edge of the widget.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	props[PROP_INSET_BOTTOM] = g_param_spec_int(
		"inset-bottom",
		"Inset Bottom",
		"Inset for the bottom edge of the widget.",
		0,
		INT_MAX,
		0,
		G_PARAM_READWRITE );
	g_object_class_install_properties( object_klass, N_PROPS, props );
}

//
// Instance initialization for notifications.
//
void foobar_notification_widget_init( FoobarNotificationWidget* self )
{
	// Set up the main notification content.

	GtkWidget* icon = gtk_image_new( );
	gtk_widget_add_css_class( icon, "icon" );
	gtk_widget_set_valign( icon, GTK_ALIGN_CENTER );

	GtkWidget* title = gtk_label_new( NULL );
	gtk_label_set_justify( GTK_LABEL( title ), GTK_JUSTIFY_LEFT );
	gtk_label_set_ellipsize( GTK_LABEL( title ), PANGO_ELLIPSIZE_END );
	gtk_label_set_wrap( GTK_LABEL( title ), FALSE );
	gtk_label_set_xalign( GTK_LABEL( title ), 0 );
	gtk_widget_add_css_class( title, "title" );
	gtk_widget_set_valign( title, GTK_ALIGN_BASELINE_CENTER );
	gtk_widget_set_hexpand( title, TRUE );

	GtkWidget* time = gtk_label_new( NULL );
	gtk_label_set_wrap( GTK_LABEL( time ), FALSE );
	gtk_widget_add_css_class( time, "time" );
	gtk_widget_set_valign( time, GTK_ALIGN_BASELINE_CENTER );

	GtkWidget* body = gtk_label_new( NULL );
	gtk_label_set_use_markup( GTK_LABEL( body ), TRUE );
	gtk_label_set_justify( GTK_LABEL( body ), GTK_JUSTIFY_LEFT );
	gtk_label_set_ellipsize( GTK_LABEL( body ), PANGO_ELLIPSIZE_END );
	gtk_label_set_wrap( GTK_LABEL( body ), TRUE );
	gtk_label_set_wrap_mode( GTK_LABEL( body ), PANGO_WRAP_WORD_CHAR );
	// XXX: Increase once layout is fixed in GTK (https://gitlab.gnome.org/GNOME/gtk/-/issues/5885)
	gtk_label_set_lines( GTK_LABEL( body ), 1 );
	gtk_label_set_xalign( GTK_LABEL( body ), 0 );
	gtk_widget_add_css_class( body, "body" );

	GtkWidget* header = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_append( GTK_BOX( header ), title );
	gtk_box_append( GTK_BOX( header ), time );

	GtkWidget* row = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_append( GTK_BOX( row ), header );
	gtk_box_append( GTK_BOX( row ), body );
	gtk_widget_set_hexpand( row, TRUE );
	gtk_widget_set_valign( row, GTK_ALIGN_CENTER );

	self->content = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_append( GTK_BOX( self->content ), icon );
	gtk_box_append( GTK_BOX( self->content ), row );
	gtk_widget_add_css_class( self->content, "content" );

	// Set up the overlay for the close button.

	GtkWidget* close_icon = gtk_image_new_from_icon_name( "fluent-dismiss-symbolic" );
	gtk_image_set_pixel_size( GTK_IMAGE( close_icon ), 12 );

	self->close_button = gtk_button_new( );
	gtk_button_set_child( GTK_BUTTON( self->close_button ), close_icon );
	gtk_widget_add_css_class( self->close_button, "close-button" );
	gtk_widget_set_visible( self->close_button, FALSE );
	gtk_widget_set_halign( self->close_button, GTK_ALIGN_END );
	gtk_widget_set_valign( self->close_button, GTK_ALIGN_START );
	g_signal_connect( self->close_button, "clicked", G_CALLBACK( foobar_notification_widget_handle_close_clicked ), self );

	GtkEventController* hover_controller = gtk_event_controller_motion_new( );
	g_signal_connect( hover_controller, "enter", G_CALLBACK( foobar_notification_widget_handle_enter ), self );
	g_signal_connect( hover_controller, "leave", G_CALLBACK( foobar_notification_widget_handle_leave ), self );

	self->container = gtk_overlay_new( );
	gtk_overlay_set_child( GTK_OVERLAY( self->container ), self->content );
	gtk_overlay_add_overlay( GTK_OVERLAY( self->container ), self->close_button );
	gtk_widget_add_controller( self->container, hover_controller );
	gtk_widget_set_parent( self->container, GTK_WIDGET( self ) );

	// Set up bindings.

	{
		GtkExpression* notification_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION_WIDGET, NULL, "notification" );
		GtkExpression* image_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION, notification_expr, "image" );
		gtk_expression_bind( gtk_expression_ref( image_expr ), icon, "gicon", self );
		GtkExpression* visible_params[] = { image_expr };
		GtkExpression* visible_expr = gtk_cclosure_expression_new(
			G_TYPE_BOOLEAN,
			NULL,
			G_N_ELEMENTS( visible_params ),
			visible_params,
			G_CALLBACK( foobar_notification_widget_compute_icon_visible ),
			NULL,
			NULL );
		gtk_expression_bind( visible_expr, icon, "visible", self );
	}

	{
		GtkExpression* notification_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION_WIDGET, NULL, "notification" );
		GtkExpression* summary_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION, notification_expr, "summary" );
		gtk_expression_bind( summary_expr, title, "label", self );
	}

	{
		GtkExpression* notification_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION_WIDGET, NULL, "notification" );
		GtkExpression* body_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION, notification_expr, "body" );
		gtk_expression_bind( gtk_expression_ref( body_expr ), body, "label", self );
		GtkExpression* visible_params[] = { body_expr };
		GtkExpression* visible_expr = gtk_cclosure_expression_new(
			G_TYPE_BOOLEAN,
			NULL,
			G_N_ELEMENTS( visible_params ),
			visible_params,
			G_CALLBACK( foobar_notification_widget_compute_body_visible ),
			NULL,
			NULL );
		gtk_expression_bind( visible_expr, body, "visible", self );
	}

	{
		GtkExpression* notification_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION_WIDGET, NULL, "notification" );
		GtkExpression* time_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION, notification_expr, "time" );
		GtkExpression* format_expr = gtk_property_expression_new( FOOBAR_TYPE_NOTIFICATION_WIDGET, NULL, "time-format" );
		GtkExpression* str_params[] = { time_expr, format_expr };
		GtkExpression* str_expr = gtk_cclosure_expression_new(
			G_TYPE_STRING,
			NULL,
			G_N_ELEMENTS( str_params ),
			str_params,
			G_CALLBACK( foobar_notification_widget_compute_time_label ),
			self,
			NULL );
		gtk_expression_bind( str_expr, time, "label", self );
	}
}

//
// Property getter implementation, mapping a property id to a method.
//
void foobar_notification_widget_get_property(
	GObject*    object,
	guint       prop_id,
	GValue*     value,
	GParamSpec* pspec )
{
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)object;

	switch ( prop_id )
	{
		case PROP_NOTIFICATION:
			g_value_set_object( value, foobar_notification_widget_get_notification( self ) );
			break;
		case PROP_CLOSE_ACTION:
			g_value_set_enum( value, foobar_notification_widget_get_close_action( self ) );
			break;
		case PROP_TIME_FORMAT:
			g_value_set_string( value, foobar_notification_widget_get_time_format( self ) );
			break;
		case PROP_MIN_HEIGHT:
			g_value_set_int( value, foobar_notification_widget_get_min_height( self ) );
			break;
		case PROP_CLOSE_BUTTON_INSET:
			g_value_set_int( value, foobar_notification_widget_get_close_button_inset( self ) );
			break;
		case PROP_INSET_START:
			g_value_set_int( value, foobar_notification_widget_get_inset_start( self ) );
			break;
		case PROP_INSET_END:
			g_value_set_int( value, foobar_notification_widget_get_inset_end( self ) );
			break;
		case PROP_INSET_TOP:
			g_value_set_int( value, foobar_notification_widget_get_inset_top( self ) );
			break;
		case PROP_INSET_BOTTOM:
			g_value_set_int( value, foobar_notification_widget_get_inset_bottom( self ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Property setter implementation, mapping a property id to a method.
//
void foobar_notification_widget_set_property(
	GObject*      object,
	guint         prop_id,
	GValue const* value,
	GParamSpec*   pspec )
{
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)object;

	switch ( prop_id )
	{
		case PROP_NOTIFICATION:
			foobar_notification_widget_set_notification( self, g_value_get_object( value ) );
			break;
		case PROP_CLOSE_ACTION:
			foobar_notification_widget_set_close_action( self, g_value_get_enum( value ) );
			break;
		case PROP_TIME_FORMAT:
			foobar_notification_widget_set_time_format( self, g_value_get_string( value ) );
			break;
		case PROP_MIN_HEIGHT:
			foobar_notification_widget_set_min_height( self, g_value_get_int( value ) );
			break;
		case PROP_CLOSE_BUTTON_INSET:
			foobar_notification_widget_set_close_button_inset( self, g_value_get_int( value ) );
			break;
		case PROP_INSET_START:
			foobar_notification_widget_set_inset_start( self, g_value_get_int( value ) );
			break;
		case PROP_INSET_END:
			foobar_notification_widget_set_inset_end( self, g_value_get_int( value ) );
			break;
		case PROP_INSET_TOP:
			foobar_notification_widget_set_inset_top( self, g_value_get_int( value ) );
			break;
		case PROP_INSET_BOTTOM:
			foobar_notification_widget_set_inset_bottom( self, g_value_get_int( value ) );
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
	}
}

//
// Instance de-initialization for notifications.
//
void foobar_notification_widget_dispose( GObject* object )
{
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)object;

	GtkWidget* child;
	while ( ( child = gtk_widget_get_first_child( GTK_WIDGET( self ) ) ) )
	{
		gtk_widget_unparent( child );
	}

	G_OBJECT_CLASS( foobar_notification_widget_parent_class )->dispose( object );
}

//
// Instance cleanup for notifications.
//
void foobar_notification_widget_finalize( GObject* object )
{
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)object;

	g_clear_object( &self->notification );
	g_clear_pointer( &self->time_format, g_free );

	G_OBJECT_CLASS( foobar_notification_widget_parent_class )->finalize( object );
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------------------------------

//
// Create a new notification widget instance.
//
GtkWidget* foobar_notification_widget_new( void )
{
	return g_object_new( FOOBAR_TYPE_NOTIFICATION_WIDGET, NULL );
}

//
// Get the notification model currently displayed.
//
FoobarNotification* foobar_notification_widget_get_notification( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), NULL );
	return self->notification;
}

//
// Get the action invoked when the user wants to "close" a notification.
//
FoobarNotificationCloseAction foobar_notification_widget_get_close_action( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->close_action;
}

//
// Get the time format string used to display the notification's timestamp.
//
gchar const* foobar_notification_widget_get_time_format( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), NULL );
	return self->time_format;
}

//
// Get the minimum height of the notification (excluding the inset).
//
gint foobar_notification_widget_get_min_height( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->min_height;
}

//
// Get the offset of the close button within the notification (may be negative, possibly increasing the actual inset).
//
gint foobar_notification_widget_get_close_button_inset( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->close_button_inset;
}

//
// Get the margin for the start edge of the notification.
//
gint foobar_notification_widget_get_inset_start( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->inset_start;
}

//
// Get the margin for the end edge of the notification.
//
gint foobar_notification_widget_get_inset_end( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->inset_end;
}

//
// Get the margin for the top edge of the notification.
//
gint foobar_notification_widget_get_inset_top( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->inset_top;
}

//
// Get the margin for the bottom edge of the notification.
//
gint foobar_notification_widget_get_inset_bottom( FoobarNotificationWidget* self )
{
	g_return_val_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ), 0 );
	return self->inset_bottom;
}

//
// Update the notification model currently displayed.
//
void foobar_notification_widget_set_notification(
	FoobarNotificationWidget* self,
	FoobarNotification*       value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	if ( self->notification != value )
	{
		g_clear_object( &self->notification );
		if ( value ) { self->notification = g_object_ref( value ); }
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_NOTIFICATION] );
	}
}

//
// Update the action invoked when the user wants to "close" a notification.
//
void foobar_notification_widget_set_close_action(
	FoobarNotificationWidget*     self,
	FoobarNotificationCloseAction value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	if ( self->close_action != value )
	{
		self->close_action = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CLOSE_ACTION] );
	}
}

//
// Update the time format string used to display the notification's timestamp.
//
void foobar_notification_widget_set_time_format(
	FoobarNotificationWidget* self,
	gchar const*              value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	if ( g_strcmp0( self->time_format, value ) )
	{
		g_clear_pointer( &self->time_format, g_free );
		self->time_format = g_strdup( value );
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_TIME_FORMAT] );
	}
}

//
// Update the minimum height of the notification (excluding the inset).
//
void foobar_notification_widget_set_min_height(
	FoobarNotificationWidget* self,
	gint                      value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	value = MAX( value, 0 );
	if ( self->min_height != value )
	{
		self->min_height = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_MIN_HEIGHT] );
		gtk_widget_set_size_request( self->content, -1, self->min_height );
	}
}

//
// Update the offset of the close button within the notification (may be negative, possibly increasing the actual inset).
//
void foobar_notification_widget_set_close_button_inset(
	FoobarNotificationWidget* self,
	gint                      value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	if ( self->close_button_inset != value )
	{
		self->close_button_inset = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_CLOSE_BUTTON_INSET] );
		foobar_notification_widget_update_margins( self );
	}
}

//
// Update the margin for the start edge of the notification.
//
void foobar_notification_widget_set_inset_start(
	FoobarNotificationWidget* self,
	gint                      value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	value = MAX( value, 0 );
	if ( self->inset_start != value )
	{
		self->inset_start = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_START] );
		foobar_notification_widget_update_margins( self );
	}
}

//
// Update the margin for the end edge of the notification.
//
void foobar_notification_widget_set_inset_end(
	FoobarNotificationWidget* self,
	gint                      value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	value = MAX( value, 0 );
	if ( self->inset_end != value )
	{
		self->inset_end = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_END] );
		foobar_notification_widget_update_margins( self );
	}
}

//
// Update the margin for the top edge of the notification.
//
void foobar_notification_widget_set_inset_top(
	FoobarNotificationWidget* self,
	gint                      value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	value = MAX( value, 0 );
	if ( self->inset_top != value )
	{
		self->inset_top = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_TOP] );
		foobar_notification_widget_update_margins( self );
	}
}

//
// Update the margin for the bottom edge of the notification.
//
void foobar_notification_widget_set_inset_bottom(
	FoobarNotificationWidget* self,
	gint                      value )
{
	g_return_if_fail( FOOBAR_IS_NOTIFICATION_WIDGET( self ) );

	value = MAX( value, 0 );
	if ( self->inset_bottom != value )
	{
		self->inset_bottom = value;
		g_object_notify_by_pspec( G_OBJECT( self ), props[PROP_INSET_BOTTOM] );
		foobar_notification_widget_update_margins( self );
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Signal Handlers
// ---------------------------------------------------------------------------------------------------------------------

//
// Called when the mouse cursor enters the notification.
//
// We will temporarily block the dismissal timeout for the notification and show the close button.
//
void foobar_notification_widget_handle_enter(
	GtkEventControllerMotion* controller,
	gdouble                   x,
	gdouble                   y,
	gpointer                  userdata )
{
	(void)controller;
	(void)x;
	(void)y;
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)userdata;

	gtk_widget_set_visible( self->close_button, TRUE );
	if ( self->notification ) { foobar_notification_block_timeout( self->notification ); }
}

//
// Called when the mouse cursor leaves the notification.
//
// We will unblock the dismissal timeout for the notification and hide the close button.
//
void foobar_notification_widget_handle_leave(
	GtkEventControllerMotion* controller,
	gpointer                  userdata )
{
	(void)controller;
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)userdata;

	gtk_widget_set_visible( self->close_button, FALSE );
	if ( self->notification ) { foobar_notification_resume_timeout( self->notification ); }
}

//
// Called when the user has clicked the "close" button.
//
// This either marks the notification as dismissed or removes it.
//
void foobar_notification_widget_handle_close_clicked(
	GtkButton* button,
	gpointer   userdata )
{
	(void)button;
	FoobarNotificationWidget* self = (FoobarNotificationWidget*)userdata;

	if ( self->notification )
	{
		switch ( self->close_action )
		{
			case FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_REMOVE:
				foobar_notification_close( self->notification );
				break;
			case FOOBAR_TYPE_NOTIFICATION_CLOSE_ACTION_DISMISS:
				foobar_notification_dismiss( self->notification );
				break;
			default:
				g_warn_if_reached( );
				break;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Value Converters
// ---------------------------------------------------------------------------------------------------------------------

//
// Derive the timestamp label value from the notification's timestamp and the selected time format.
//
gchar* foobar_notification_widget_compute_time_label(
	GtkExpression* expression,
	GDateTime*     time,
	gchar const*   format,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return format != NULL ? g_date_time_format( time, format ) : NULL;
}

//
// Derive the visibility of the notification icon from its value.
//
gboolean foobar_notification_widget_compute_icon_visible(
	GtkExpression* expression,
	GIcon*         icon,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return icon != NULL;
}

//
// Derive the visibility of the notification body label from its value.
//
gboolean foobar_notification_widget_compute_body_visible(
	GtkExpression* expression,
	gchar const*   body,
	gpointer       userdata )
{
	(void)expression;
	(void)userdata;

	return body != NULL && g_strcmp0( body, "" );
}

// ---------------------------------------------------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------------------------------------------------

//
// Update the margins of the content widget and close button to the current inset values.
//
void foobar_notification_widget_update_margins( FoobarNotificationWidget* self )
{
	gint inset = MAX( self->close_button_inset, 0 );
	gint overrun = MAX( -self->close_button_inset, 0 );
	gint actual_inset_end = MAX( self->inset_end, overrun );
	gint actual_inset_top = MAX( self->inset_top, overrun );

	gtk_widget_set_margin_start( self->container, self->inset_start );
	gtk_widget_set_margin_end( self->container, actual_inset_end - overrun );
	gtk_widget_set_margin_top( self->container, actual_inset_top - overrun );
	gtk_widget_set_margin_bottom( self->container, self->inset_bottom );

	gtk_widget_set_margin_end( self->content, overrun );
	gtk_widget_set_margin_top( self->content, overrun );

	gtk_widget_set_margin_end( self->close_button, inset );
	gtk_widget_set_margin_top( self->close_button, inset );
}
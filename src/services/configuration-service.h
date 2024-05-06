#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define FOOBAR_TYPE_SCREEN_EDGE                  foobar_screen_edge_get_type( )
#define FOOBAR_TYPE_ORIENTATION                  foobar_orientation_get_type( )
#define FOOBAR_TYPE_STATUS_ITEM                  foobar_status_item_get_type( )
#define FOOBAR_TYPE_PANEL_ITEM_KIND              foobar_panel_item_kind_get_type( )
#define FOOBAR_TYPE_PANEL_ITEM_ACTION            foobar_panel_item_action_get_type( )
#define FOOBAR_TYPE_PANEL_ITEM_POSITION          foobar_panel_item_position_get_type( )
#define FOOBAR_TYPE_CONTROL_CENTER_ROW           foobar_control_center_row_get_type( )
#define FOOBAR_TYPE_CONTROL_CENTER_ALIGNMENT     foobar_control_center_alignment_get_type( )
#define FOOBAR_TYPE_GENERAL_CONFIGURATION        foobar_general_configuration_get_type( )
#define FOOBAR_TYPE_PANEL_ITEM_CONFIGURATION     foobar_panel_item_configuration_get_type( )
#define FOOBAR_TYPE_PANEL_CONFIGURATION          foobar_panel_configuration_get_type( )
#define FOOBAR_TYPE_LAUNCHER_CONFIGURATION       foobar_launcher_configuration_get_type( )
#define FOOBAR_TYPE_CONTROL_CENTER_CONFIGURATION foobar_control_center_configuration_get_type( )
#define FOOBAR_TYPE_NOTIFICATION_CONFIGURATION   foobar_notification_configuration_get_type( )
#define FOOBAR_TYPE_CONFIGURATION                foobar_configuration_get_type( )
#define FOOBAR_TYPE_CONFIGURATION_SERVICE        foobar_configuration_service_get_type( )

typedef enum
{
	FOOBAR_SCREEN_EDGE_LEFT,
	FOOBAR_SCREEN_EDGE_RIGHT,
	FOOBAR_SCREEN_EDGE_TOP,
	FOOBAR_SCREEN_EDGE_BOTTOM,
} FoobarScreenEdge;

GType foobar_screen_edge_get_type( void );

typedef enum
{
	FOOBAR_ORIENTATION_HORIZONTAL,
	FOOBAR_ORIENTATION_VERTICAL,
} FoobarOrientation;

GType foobar_orientation_get_type( void );

typedef enum
{
	FOOBAR_STATUS_ITEM_NETWORK,
	FOOBAR_STATUS_ITEM_BLUETOOTH,
	FOOBAR_STATUS_ITEM_BATTERY,
	FOOBAR_STATUS_ITEM_BRIGHTNESS,
	FOOBAR_STATUS_ITEM_AUDIO,
	FOOBAR_STATUS_ITEM_NOTIFICATIONS,
} FoobarStatusItem;

GType foobar_status_item_get_type( void );

typedef enum
{
	FOOBAR_PANEL_ITEM_KIND_ICON,
	FOOBAR_PANEL_ITEM_KIND_CLOCK,
	FOOBAR_PANEL_ITEM_KIND_WORKSPACES,
	FOOBAR_PANEL_ITEM_KIND_STATUS,
} FoobarPanelItemKind;

GType foobar_panel_item_kind_get_type( void );

typedef enum
{
	FOOBAR_PANEL_ITEM_ACTION_NONE,
	FOOBAR_PANEL_ITEM_ACTION_LAUNCHER,
	FOOBAR_PANEL_ITEM_ACTION_CONTROL_CENTER,
} FoobarPanelItemAction;

GType foobar_panel_item_action_get_type( void );

typedef enum
{
	FOOBAR_PANEL_ITEM_POSITION_START,
	FOOBAR_PANEL_ITEM_POSITION_CENTER,
	FOOBAR_PANEL_ITEM_POSITION_END,
} FoobarPanelItemPosition;

GType foobar_panel_item_position_get_type( void );

typedef enum
{
	FOOBAR_CONTROL_CENTER_ROW_CONNECTIVITY,
	FOOBAR_CONTROL_CENTER_ROW_AUDIO_OUTPUT,
	FOOBAR_CONTROL_CENTER_ROW_AUDIO_INPUT,
	FOOBAR_CONTROL_CENTER_ROW_BRIGHTNESS,
} FoobarControlCenterRow;

GType foobar_control_center_row_get_type( void );

typedef enum
{
	FOOBAR_CONTROL_CENTER_ALIGNMENT_START,
	FOOBAR_CONTROL_CENTER_ALIGNMENT_CENTER,
	FOOBAR_CONTROL_CENTER_ALIGNMENT_END,
	FOOBAR_CONTROL_CENTER_ALIGNMENT_FILL,
} FoobarControlCenterAlignment;

GType foobar_control_center_alignment_get_type( void );

typedef struct _FoobarGeneralConfiguration FoobarGeneralConfiguration;

GType                       foobar_general_configuration_get_type      ( void );
FoobarGeneralConfiguration* foobar_general_configuration_new           ( void );
FoobarGeneralConfiguration* foobar_general_configuration_copy          ( FoobarGeneralConfiguration const* self );
void                        foobar_general_configuration_free          ( FoobarGeneralConfiguration*       self );
gboolean                    foobar_general_configuration_equal         ( FoobarGeneralConfiguration const* a,
                                                                         FoobarGeneralConfiguration const* b );
gchar const*                foobar_general_configuration_get_stylesheet( FoobarGeneralConfiguration const* self );
void                        foobar_general_configuration_set_stylesheet( FoobarGeneralConfiguration*       self,
                                                                         gchar const*                      value );

typedef struct _FoobarPanelItemConfiguration FoobarPanelItemConfiguration;

GType                         foobar_panel_item_configuration_get_type    ( void );
FoobarPanelItemConfiguration* foobar_panel_item_configuration_copy        ( FoobarPanelItemConfiguration const* self );
void                          foobar_panel_item_configuration_free        ( FoobarPanelItemConfiguration*       self );
gboolean                      foobar_panel_item_configuration_equal       ( FoobarPanelItemConfiguration const* a,
                                                                            FoobarPanelItemConfiguration const* b );
FoobarPanelItemKind           foobar_panel_item_configuration_get_kind    ( FoobarPanelItemConfiguration const* self );
gchar const*                  foobar_panel_item_configuration_get_name    ( FoobarPanelItemConfiguration const* self );
FoobarPanelItemPosition       foobar_panel_item_configuration_get_position( FoobarPanelItemConfiguration const* self );
FoobarPanelItemAction         foobar_panel_item_configuration_get_action  ( FoobarPanelItemConfiguration const* self );
void                          foobar_panel_item_configuration_set_name    ( FoobarPanelItemConfiguration*       self,
                                                                            gchar const*                        value );
void                          foobar_panel_item_configuration_set_position( FoobarPanelItemConfiguration*       self,
                                                                            FoobarPanelItemPosition             value );
void                          foobar_panel_item_configuration_set_action  ( FoobarPanelItemConfiguration*       self,
                                                                            FoobarPanelItemAction               value );

FoobarPanelItemConfiguration* foobar_panel_item_icon_configuration_new          ( void );
gchar const*                  foobar_panel_item_icon_configuration_get_icon_name( FoobarPanelItemConfiguration const* self );
void                          foobar_panel_item_icon_configuration_set_icon_name( FoobarPanelItemConfiguration*       self,
                                                                                  gchar const*                        value );

FoobarPanelItemConfiguration* foobar_panel_item_clock_configuration_new       ( void );
gchar const*                  foobar_panel_item_clock_configuration_get_format( FoobarPanelItemConfiguration const* self );
void                          foobar_panel_item_clock_configuration_set_format( FoobarPanelItemConfiguration*       self,
                                                                                gchar const*                        value );

FoobarPanelItemConfiguration* foobar_panel_item_workspaces_configuration_new            ( void );
gint                          foobar_panel_item_workspaces_configuration_get_button_size( FoobarPanelItemConfiguration const* self );
gint                          foobar_panel_item_workspaces_configuration_get_spacing    ( FoobarPanelItemConfiguration const* self );
void                          foobar_panel_item_workspaces_configuration_set_button_size( FoobarPanelItemConfiguration*       self,
                                                                                          gint                                value );
void                          foobar_panel_item_workspaces_configuration_set_spacing    ( FoobarPanelItemConfiguration*       self,
                                                                                          gint                                value );

FoobarPanelItemConfiguration* foobar_panel_item_status_configuration_new                 ( void );
FoobarStatusItem const*       foobar_panel_item_status_configuration_get_items           ( FoobarPanelItemConfiguration const* self,
                                                                                           gsize*                              out_count );
gint                          foobar_panel_item_status_configuration_get_spacing         ( FoobarPanelItemConfiguration const* self );
gboolean                      foobar_panel_item_status_configuration_get_show_labels     ( FoobarPanelItemConfiguration const* self );
gboolean                      foobar_panel_item_status_configuration_get_enable_scrolling( FoobarPanelItemConfiguration const* self );
void                          foobar_panel_item_status_configuration_set_items           ( FoobarPanelItemConfiguration*       self,
                                                                                           FoobarStatusItem const*             value,
                                                                                           gsize                               value_count );
void                          foobar_panel_item_status_configuration_set_spacing         ( FoobarPanelItemConfiguration*       self,
                                                                                           gint                                value );
void                          foobar_panel_item_status_configuration_set_show_labels     ( FoobarPanelItemConfiguration*       self,
                                                                                           gboolean                            value );
void                          foobar_panel_item_status_configuration_set_enable_scrolling( FoobarPanelItemConfiguration*       self,
                                                                                           gboolean                            value );

typedef struct _FoobarPanelConfiguration FoobarPanelConfiguration;

GType                                      foobar_panel_configuration_get_type         ( void );
FoobarPanelConfiguration*                  foobar_panel_configuration_new              ( void );
FoobarPanelConfiguration*                  foobar_panel_configuration_copy             ( FoobarPanelConfiguration const*            self );
void                                       foobar_panel_configuration_free             ( FoobarPanelConfiguration*                  self );
gboolean                                   foobar_panel_configuration_equal            ( FoobarPanelConfiguration const*            a,
                                                                                         FoobarPanelConfiguration const*            b );
FoobarScreenEdge                           foobar_panel_configuration_get_position     ( FoobarPanelConfiguration const*            self );
gint                                       foobar_panel_configuration_get_margin       ( FoobarPanelConfiguration const*            self );
gint                                       foobar_panel_configuration_get_padding      ( FoobarPanelConfiguration const*            self );
gint                                       foobar_panel_configuration_get_size         ( FoobarPanelConfiguration const*            self );
gint                                       foobar_panel_configuration_get_spacing      ( FoobarPanelConfiguration const*            self );
gboolean                                   foobar_panel_configuration_get_multi_monitor( FoobarPanelConfiguration const*            self );
FoobarPanelItemConfiguration const* const* foobar_panel_configuration_get_items        ( FoobarPanelConfiguration const*            self,
                                                                                         gsize*                                     out_count );
FoobarPanelItemConfiguration* const*       foobar_panel_configuration_get_items_mut    ( FoobarPanelConfiguration*                  self,
                                                                                         gsize*                                     out_count );
void                                       foobar_panel_configuration_set_position     ( FoobarPanelConfiguration*                  self,
                                                                                         FoobarScreenEdge                           value );
void                                       foobar_panel_configuration_set_margin       ( FoobarPanelConfiguration*                  self,
                                                                                         gint                                       value );
void                                       foobar_panel_configuration_set_padding      ( FoobarPanelConfiguration*                  self,
                                                                                         gint                                       value );
void                                       foobar_panel_configuration_set_size         ( FoobarPanelConfiguration*                  self,
                                                                                         gint                                       value );
void                                       foobar_panel_configuration_set_spacing      ( FoobarPanelConfiguration*                  self,
                                                                                         gint                                       value );
void                                       foobar_panel_configuration_set_multi_monitor( FoobarPanelConfiguration*                  self,
                                                                                         gboolean                                   value );
void                                       foobar_panel_configuration_set_items        ( FoobarPanelConfiguration*                  self,
                                                                                         FoobarPanelItemConfiguration const* const* value,
                                                                                         gsize                                      value_count );

typedef struct _FoobarLauncherConfiguration FoobarLauncherConfiguration;

GType                        foobar_launcher_configuration_get_type      ( void );
FoobarLauncherConfiguration* foobar_launcher_configuration_new           ( void );
FoobarLauncherConfiguration* foobar_launcher_configuration_copy          ( FoobarLauncherConfiguration const* self );
void                         foobar_launcher_configuration_free          ( FoobarLauncherConfiguration*       self );
gboolean                     foobar_launcher_configuration_equal         ( FoobarLauncherConfiguration const* a,
                                                                           FoobarLauncherConfiguration const* b );
gint                         foobar_launcher_configuration_get_width     ( FoobarLauncherConfiguration const* self );
gint                         foobar_launcher_configuration_get_position  ( FoobarLauncherConfiguration const* self );
gint                         foobar_launcher_configuration_get_max_height( FoobarLauncherConfiguration const* self );
void                         foobar_launcher_configuration_set_width     ( FoobarLauncherConfiguration*       self,
                                                                           gint                               value );
void                         foobar_launcher_configuration_set_position  ( FoobarLauncherConfiguration*       self,
                                                                           gint                               value );
void                         foobar_launcher_configuration_set_max_height( FoobarLauncherConfiguration*       self,
                                                                           gint                               value );

typedef struct _FoobarControlCenterConfiguration FoobarControlCenterConfiguration;

GType                             foobar_control_center_configuration_get_type       ( void );
FoobarControlCenterConfiguration* foobar_control_center_configuration_new            ( void );
FoobarControlCenterConfiguration* foobar_control_center_configuration_copy           ( FoobarControlCenterConfiguration const* self );
void                              foobar_control_center_configuration_free           ( FoobarControlCenterConfiguration*       self );
gboolean                          foobar_control_center_configuration_equal          ( FoobarControlCenterConfiguration const* a,
                                                                                       FoobarControlCenterConfiguration const* b );
gint                              foobar_control_center_configuration_get_width      ( FoobarControlCenterConfiguration const* self );
gint                              foobar_control_center_configuration_get_height     ( FoobarControlCenterConfiguration const* self );
FoobarScreenEdge                  foobar_control_center_configuration_get_position   ( FoobarControlCenterConfiguration const* self );
gint                              foobar_control_center_configuration_get_offset     ( FoobarControlCenterConfiguration const* self );
gint                              foobar_control_center_configuration_get_padding    ( FoobarControlCenterConfiguration const* self );
gint                              foobar_control_center_configuration_get_spacing    ( FoobarControlCenterConfiguration const* self );
FoobarOrientation                 foobar_control_center_configuration_get_orientation( FoobarControlCenterConfiguration const* self );
FoobarControlCenterAlignment      foobar_control_center_configuration_get_alignment  ( FoobarControlCenterConfiguration const* self );
FoobarControlCenterRow const*     foobar_control_center_configuration_get_rows       ( FoobarControlCenterConfiguration const* self,
                                                                                       gsize*                                  out_count );
void                              foobar_control_center_configuration_set_width      ( FoobarControlCenterConfiguration*       self,
                                                                                       gint                                    value );
void                              foobar_control_center_configuration_set_height     ( FoobarControlCenterConfiguration*       self,
                                                                                       gint                                    value );
void                              foobar_control_center_configuration_set_position   ( FoobarControlCenterConfiguration*       self,
                                                                                       FoobarScreenEdge                        value );
void                              foobar_control_center_configuration_set_offset     ( FoobarControlCenterConfiguration*       self,
                                                                                       gint                                    value );
void                              foobar_control_center_configuration_set_padding    ( FoobarControlCenterConfiguration*       self,
                                                                                       gint                                    value );
void                              foobar_control_center_configuration_set_spacing    ( FoobarControlCenterConfiguration*       self,
                                                                                       gint                                    value );
void                              foobar_control_center_configuration_set_orientation( FoobarControlCenterConfiguration*       self,
                                                                                       FoobarOrientation                       value );
void                              foobar_control_center_configuration_set_rows       ( FoobarControlCenterConfiguration*       self,
                                                                                       FoobarControlCenterRow const*           value,
                                                                                       gsize                                   value_count );
void                              foobar_control_center_configuration_set_alignment  ( FoobarControlCenterConfiguration*       self,
                                                                                       FoobarControlCenterAlignment            value );

typedef struct _FoobarNotificationConfiguration FoobarNotificationConfiguration;

GType                            foobar_notification_configuration_get_type              ( void );
FoobarNotificationConfiguration* foobar_notification_configuration_new                   ( void );
FoobarNotificationConfiguration* foobar_notification_configuration_copy                  ( FoobarNotificationConfiguration const* self );
void                             foobar_notification_configuration_free                  ( FoobarNotificationConfiguration*       self );
gboolean                         foobar_notification_configuration_equal                 ( FoobarNotificationConfiguration const* a,
                                                                                           FoobarNotificationConfiguration const* b );
gint                             foobar_notification_configuration_get_width             ( FoobarNotificationConfiguration const* self );
gint                             foobar_notification_configuration_get_min_height        ( FoobarNotificationConfiguration const* self );
gint                             foobar_notification_configuration_get_spacing           ( FoobarNotificationConfiguration const* self );
gint                             foobar_notification_configuration_get_close_button_inset( FoobarNotificationConfiguration const* self );
gchar const*                     foobar_notification_configuration_get_time_format       ( FoobarNotificationConfiguration const* self );
void                             foobar_notification_configuration_set_width             ( FoobarNotificationConfiguration*       self,
                                                                                           gint                                   value );
void                             foobar_notification_configuration_set_min_height        ( FoobarNotificationConfiguration*       self,
                                                                                           gint                                   value );
void                             foobar_notification_configuration_set_spacing           ( FoobarNotificationConfiguration*       self,
                                                                                           gint                                   value );
void                             foobar_notification_configuration_set_close_button_inset( FoobarNotificationConfiguration*       self,
                                                                                           gint                                   value );
void                             foobar_notification_configuration_set_time_format       ( FoobarNotificationConfiguration*       self,
                                                                                           gchar const*                           value );

typedef struct _FoobarConfiguration FoobarConfiguration;

GType                                   foobar_configuration_get_type              ( void );
FoobarConfiguration*                    foobar_configuration_new                   ( void );
FoobarConfiguration*                    foobar_configuration_copy                  ( FoobarConfiguration const* self );
void                                    foobar_configuration_free                  ( FoobarConfiguration*       self );
gboolean                                foobar_configuration_equal                 ( FoobarConfiguration const* a,
                                                                                     FoobarConfiguration const* b );
FoobarGeneralConfiguration const*       foobar_configuration_get_general           ( FoobarConfiguration const* self );
FoobarGeneralConfiguration*             foobar_configuration_get_general_mut       ( FoobarConfiguration*       self );
FoobarPanelConfiguration const*         foobar_configuration_get_panel             ( FoobarConfiguration const* self );
FoobarPanelConfiguration*               foobar_configuration_get_panel_mut         ( FoobarConfiguration*       self );
FoobarLauncherConfiguration const*      foobar_configuration_get_launcher          ( FoobarConfiguration const* self );
FoobarLauncherConfiguration*            foobar_configuration_get_launcher_mut      ( FoobarConfiguration*       self );
FoobarControlCenterConfiguration const* foobar_configuration_get_control_center    ( FoobarConfiguration const* self );
FoobarControlCenterConfiguration*       foobar_configuration_get_control_center_mut( FoobarConfiguration*       self );
FoobarNotificationConfiguration const*  foobar_configuration_get_notifications     ( FoobarConfiguration const* self );
FoobarNotificationConfiguration*        foobar_configuration_get_notifications_mut ( FoobarConfiguration*       self );


G_DECLARE_FINAL_TYPE( FoobarConfigurationService, foobar_configuration_service, FOOBAR, CONFIGURATION_SERVICE, GObject )

FoobarConfigurationService* foobar_configuration_service_new        ( void );
FoobarConfiguration const*  foobar_configuration_service_get_current( FoobarConfigurationService* self );

G_END_DECLS
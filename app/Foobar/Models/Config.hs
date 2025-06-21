{-# LANGUAGE DuplicateRecordFields #-}

-- |
-- Module     : Foobar.Models.Config
-- Description: Data stored in the configuration file.

module Foobar.Models.Config
  (  -- * Top-Level Configuration
    Config(..)
  , GeneralConfig(..)
  , PanelConfig(..)
  , LauncherConfig(..)
  , ControlCenterConfig(..)
  , NotificationConfig(..)
  , -- * Common Data Structures
    ScreenEdge(..)
  , Orientation(..)
  , Action(..)
  , -- * Panel Configuration
    PanelItem(..)
  , PanelItemConfiguration(..)
  , PanelIconConfiguration(..)
  , PanelClockConfiguration(..)
  , PanelWorkspacesConfiguration(..)
  , PanelStatusConfiguration(..)
  , PanelStatusItem(..)
  , PanelItemPosition(..)
  , -- * Control Center Configuration
    ControlCenterRow(..)
  , ControlCenterAlignment(..)
  ) where

import Data.Text

-- | Main configuration structure containing the configuration state for all components of the application.
data Config
  = Config { general       :: !GeneralConfig       -- ^ The general application configuration.
           , panel         :: !PanelConfig         -- ^ The panel configuration.
           , launcher      :: !LauncherConfig      -- ^ The launcher configuration.
           , controlCenter :: !ControlCenterConfig -- ^ The control center configuration.
           , notifications :: !NotificationConfig  -- ^ The notification configuration.
           }
  deriving (Show)

-- | General application settings not specific to a single component.
data GeneralConfig
  = GeneralConfig { stylesheet :: !Text -- ^ URI to the CSS stylesheet to use -- this may be a resource that's bundled
                                        --   with Foobar or a "file:" URI.
                  }
  deriving (Show)

-- | Configuration for the panel shown at the edge of a screen.
data PanelConfig
  = PanelConfig { position     :: !ScreenEdge  -- ^ The screen edge which the panel should appear on.
                , margin       :: !Int         -- ^ Offset of the panel from all screen edges.
                , padding      :: !Int         -- ^ Inset of the panel along its orientation.
                , size         :: !Int         -- ^ Size of the panel (depending on the orientation, this can be either
                                               --   the width or the height).
                , spacing      :: !Int         -- ^ Spacing between panel items.
                , multiMonitor :: !Bool        -- ^ Flag to enable the panel on all monitors.
                , items        :: ![PanelItem] -- ^ Items configured to be displayed in the panel.
                }
  deriving (Show)

-- | Configuration for the application launcher.
data LauncherConfig
  = LauncherConfig { width     :: !Int -- ^ Horizontal size of the launcher.
                   , position  :: !Int -- ^ Offset from the top of the screen.
                   , maxHeight :: !Int -- ^ Maximum allowed height for the launcher before scrolling is enabled.
                   }
  deriving (Show)

-- | Configuration for the control center.
data ControlCenterConfig
  = ControlCenterConfig { width       :: !Int                    -- ^ Horizontal size of the control center.
                        , height      :: !Int                    -- ^ Vertical size of the control center.
                        , position    :: !ScreenEdge             -- ^ The screen edge which the control center should
                                                                 --   be attached to.
                        , offset      :: !Int                    -- ^ Offset from the attached screen edge.
                        , padding     :: !Int                    -- ^ Spacing between the edges of the control center
                                                                 --   and its items.
                        , spacing     :: !Int                    -- ^ Spacing between the items.
                        , orientation :: !Orientation            -- ^ The orientation used to arrange the controls and
                                                                 --   notifications sections.
                        , alignment   :: !ControlCenterAlignment -- ^ Alignment along the attached screen edge.
                        , rows        :: ![ControlCenterRow]     -- ^ Ordered list of rows to display in the controls
                                                                 --   section.
                        }
  deriving (Show)

-- | Configuration for notifications and the notification area shown in the corner of the screen.
data NotificationConfig
  = NotificationConfig { width            :: !Int  -- ^ Horizontal size of notifications in the notification area.
                       , minHeight        :: !Int  -- ^ Minimum height for each notification.
                       , spacing          :: !Int  -- ^ Spacing between notifications and from the screen edges.
                       , closeButtonInset :: !Int  -- ^ Inset of the close button within the notification (may be
                                                   --   negative).
                       , timeFormat       :: !Text -- ^ The time format string as used by g_date_time_format.
                       }
  deriving (Show)

-- | An edge of the screen.
data ScreenEdge
  = ScreenEdgeLeft
  | ScreenEdgeRight
  | ScreenEdgeTop
  | ScreenEdgeBottom
  deriving (Show, Enum, Bounded, Eq)

-- | An orientation value.
data Orientation
  = OrientationHorizontal
  | OrientationVertical
  deriving (Show, Enum, Bounded, Eq)

-- | The action to be executed when a panel item is clicked.
data Action
  = ActionNone
  | ActionLauncher
  | ActionControlCenter
  deriving (Show, Enum, Bounded, Eq)

-- | Configuration for an item in the panel, including a general item configuration and an item-specific configuration.
data PanelItem
  = PanelItemIcon !PanelItemConfiguration !PanelIconConfiguration
  | PanelItemClock !PanelItemConfiguration !PanelClockConfiguration
  | PanelItemWorkspaces !PanelItemConfiguration !PanelWorkspacesConfiguration
  | PanelItemStatus !PanelItemConfiguration !PanelStatusConfiguration
  deriving (Show)

-- | General configuration for items in the panel.
data PanelItemConfiguration
  = PanelItemConfiguration { name     :: !Text              -- ^ Name of the panel item (this is derived from the
                                                            --   section name, which has the form "panel.[name]").
                           , position :: !PanelItemPosition -- ^ Position where the item should be placed item
                                                            --   within the panel.
                           }
  deriving (Show)

-- | Configuration for icon items in the panel.
data PanelIconConfiguration
  = PanelIconConfiguration { iconName :: !Text   -- ^ The GTK icon to use for the item.
                           , action   :: !Action -- ^ Action invoked when the user clicks the item.
                           }
  deriving (Show)

-- | Configuration for clock items in the panel.
data PanelClockConfiguration
  = PanelClockConfiguration { format :: !Text   -- ^ The time format string as used by g_date_time_format.
                            , action :: !Action -- ^ Action invoked when the user clicks the item.
                            }
  deriving (Show)

-- | Configuration for workspace items in the panel.
data PanelWorkspacesConfiguration
  = PanelWorkspacesConfiguration { buttonSize :: !Int -- ^ Size of each workspace button.
                                 , spacing    :: !Int -- ^ Inner spacing between the workspace buttons.
                                 }
  deriving (Show)

-- | Configuration for status items in the panel.
data PanelStatusConfiguration
  = PanelStatusConfiguration { items           :: ![PanelStatusItem] -- ^ Ordered list of status items to display in
                                                                     --   the item.
                             , spacing         :: !Int               -- ^ Inner spacing between the status items.
                             , showLabels      :: !Bool              -- ^ Indicates whether text labels should be shown
                                                                     --   next to the status icons.
                             , enableScrolling :: !Bool              -- ^ If set to true, some settings like volume or
                                                                     --   brightness can be adjusted by scrolling while
                                                                     --   hovering over the status item.
                             , action          :: !Action            -- ^ Action invoked when the user clicks the item.
                             }
  deriving (Show)

-- | An item to display in a status panel item.
data PanelStatusItem
  = PanelStatusItemNetwork
  | PanelStatusItemBluetooth
  | PanelStatusItemBattery
  | PanelStatusItemBrightness
  | PanelStatusItemAudio
  | PanelStatusItemNotifications
  deriving (Show, Enum, Bounded, Eq)

-- | Position of a panel item in the panel.
data PanelItemPosition
  = PanelItemPositionStart
  | PanelItemPositionCenter
  | PanelItemPositionEnd
  deriving (Show, Enum, Bounded, Eq)

-- | A row in the "controls" section of the control center.
data ControlCenterRow
  = ControlCenterRowConnectivity
  | ControlCenterRowAudioOutput
  | ControlCenterRowAudioInput
  | ControlCenterRowBrightness
  deriving (Show, Enum, Bounded, Eq)

-- | Alignment of the control center along its screen edge.
data ControlCenterAlignment
  = ControlCenterAlignmentStart
  | ControlCenterAlignmentCenter
  | ControlCenterAlignmentEnd
  | ControlCenterAlignmentFill
  deriving (Show, Enum, Bounded, Eq)

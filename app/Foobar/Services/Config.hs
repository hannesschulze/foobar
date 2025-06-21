{-# LANGUAGE OverloadedStrings, OverloadedRecordDot, OverloadedLabels, DuplicateRecordFields, ScopedTypeVariables, FlexibleInstances, UndecidableInstances #-}

-- |
-- Module     : Foobar.Services.Config
-- Description: Retrieval of data in the configuration file.

module Foobar.Services.Config
  ( observeConfiguration
  , loadConfiguration
  , initConfiguration
  , defaultConfiguration
  ) where

import Data.Text hiding (filter, elem)
import Data.IORef (newIORef, readIORef, writeIORef)
import Data.GI.Base
import Data.GI.Base.Signals (SignalHandlerId, disconnectSignalHandler)
import System.FilePath ((</>))
import Control.Exception (try, catch, Exception, throw)
import Control.Monad (void, forM_, (>=>), when, join)
import Foreign.C (CString, CChar, peekCString, withCString)
import Foreign.Marshal.Alloc (free)
import Foreign (Ptr, nullPtr)
import Foobar.Models.Config

import qualified GI.GLib as GLib
import qualified GI.Gio as Gio

foreign import ccall "realpath" cRealpath :: CString -> CString -> IO (Ptr CChar)

getRealPath :: FilePath -> IO FilePath
getRealPath p = withCString p $ \cPath -> do
                  cRes <- cRealpath cPath nullPtr
                  res <- if cRes /= nullPtr then peekCString cRes else pure p
                  free cRes
                  pure res

data MonitoringData
  = MonitoringData { actualPath      :: !FilePath
                   , actualMonitor   :: !Gio.FileMonitor
                   , actualHandlerId :: !SignalHandlerId }

observeConfiguration :: IO () -> IO ()
observeConfiguration handler = do
  path <- getConfigPath
  actualData <- newIORef (Nothing :: Maybe MonitoringData)
  let createMonitor :: FilePath -> IO (Maybe Gio.FileMonitor)
      createMonitor p = do
        file <- Gio.fileNewForPath p
        monitor <- try $ file.monitor [] (Nothing :: Maybe Gio.Cancellable) :: IO (Either GError Gio.FileMonitor)
        pure $ case monitor of
          Left _  -> Nothing
          Right m' -> Just m'
  let actualMonitorHandler :: Gio.File -> Maybe Gio.File -> Gio.FileMonitorEvent -> IO ()
      actualMonitorHandler _ _ eventType = case eventType of
                                             Gio.FileMonitorEventCreated -> handler
                                             Gio.FileMonitorEventChanged -> handler
                                             _                           -> pure ()
  let clearActualMonitor :: IO ()
      clearActualMonitor = do
        actualData' <- readIORef actualData
        case actualData' of
          Just d  -> disconnectSignalHandler d.actualMonitor d.actualHandlerId
          Nothing -> pure ()
  let setActualMonitor :: Maybe FilePath -> IO ()
      setActualMonitor actualPath' = do
        actualMonitor' <- join <$> mapM createMonitor actualPath'
        case (actualMonitor', actualPath') of
          (Just m, Just p)  -> do h <- on m #changed actualMonitorHandler
                                  writeIORef actualData $ Just $ MonitoringData { actualPath      = p
                                                                                , actualMonitor   = m
                                                                                , actualHandlerId = h }
          _                 -> writeIORef actualData Nothing
  let updateActualMonitor :: IO ()
      updateActualMonitor = do
        realPath <- getRealPath path
        let actualPath' = if realPath == path then Nothing else Just realPath
        actualData' <- readIORef actualData
        when ((actualPath <$> actualData') == actualPath') $
          do clearActualMonitor
             setActualMonitor actualPath'
  let monitorHandler :: Gio.File -> Maybe Gio.File -> Gio.FileMonitorEvent -> IO ()
      monitorHandler file otherFile eventType = do
        updateActualMonitor
        actualMonitorHandler file otherFile eventType
  monitor <- createMonitor path
  case monitor of
    Just m  -> do _ <- on m #changed monitorHandler
                  updateActualMonitor
                  void $ disownManagedPtr m
    Nothing -> pure ()


loadConfiguration :: IO Config
loadConfiguration = do
  path <- getConfigPath
  f <- GLib.keyFileNew
  let process = do convertError (throw . ValidationError) $ f.loadFromFile path []
                   loadSection f ""
  let handleError (ValidationError msg) = do putStrLn $ "Could not read config: " ++ unpack msg
                                             pure defaultConfiguration
  catch process handleError

initConfiguration :: IO Config
initConfiguration = do
  path <- getConfigPath
  exists <- GLib.fileTest path [GLib.FileTestExists]
  if exists then loadConfiguration
            else do f <- GLib.keyFileNew
                    storeSection f "" defaultConfiguration
                    void (try $ f.saveToFile (pack path) :: IO (Either GError ()))
                    pure defaultConfiguration

defaultConfiguration :: Config
defaultConfiguration =
  Config { general       = GeneralConfig { stylesheet = "resource:///foobar/styles/default.css" }
         , panel         = PanelConfig { position     = ScreenEdgeLeft
                                       , margin       = 16
                                       , padding      = 12
                                       , size         = 48
                                       , spacing      = 12
                                       , multiMonitor = True
                                       , items        = panelItems }
         , launcher      = LauncherConfig { width     = 600
                                          , position  = 300
                                          , maxHeight = 400 }
         , controlCenter = ControlCenterConfig { width       = 400
                                               , height      = 600
                                               , position    = ScreenEdgeLeft
                                               , offset      = 16
                                               , padding     = 24
                                               , spacing     = 12
                                               , orientation = OrientationVertical
                                               , alignment   = ControlCenterAlignmentCenter
                                               , rows        = controlCenterRows }
         , notifications = NotificationConfig { width            = 400
                                              , minHeight        = 48
                                              , spacing          = 16
                                              , closeButtonInset = -6
                                              , timeFormat       = "%H:%M" } }
  where panelItems = [ PanelItemIcon (PanelItemConfiguration { name     = "launcher"
                                                             , position = PanelItemPositionStart })
                                     (PanelIconConfiguration { iconName = "fluent-grid-dots-symbolic"
                                                             , action   = ActionLauncher })
                     , PanelItemWorkspaces (PanelItemConfiguration { name     = "workspaces"
                                                                   , position = PanelItemPositionStart })
                                           (PanelWorkspacesConfiguration { buttonSize = 20
                                                                         , spacing    = 6 })
                     , PanelItemClock (PanelItemConfiguration { name     = "clock"
                                                              , position = PanelItemPositionCenter })
                                      (PanelClockConfiguration { format = "%H\n%M"
                                                               , action = ActionNone })
                     , PanelItemStatus (PanelItemConfiguration { name     = "status"
                                                               , position = PanelItemPositionEnd })
                                       (PanelStatusConfiguration { items           = statusItems
                                                                 , spacing         = 6
                                                                 , showLabels      = False
                                                                 , enableScrolling = False
                                                                 , action          = ActionControlCenter }) ]
        statusItems = [ PanelStatusItemBattery
                      , PanelStatusItemBrightness
                      , PanelStatusItemAudio
                      , PanelStatusItemNetwork
                      , PanelStatusItemBluetooth
                      , PanelStatusItemNotifications ]
        controlCenterRows = [ ControlCenterRowConnectivity
                            , ControlCenterRowAudioOutput
                            , ControlCenterRowBrightness ]

getConfigPath :: IO FilePath
getConfigPath = do
  dir <- GLib.getUserConfigDir
  pure (dir </> "foobar.conf")

newtype ValidationError = ValidationError Text
  deriving (Show)

type Validator a = a -> IO (Maybe Text)

instance Exception ValidationError

throwValidationError :: Text -> Text -> Text -> IO a
throwValidationError s k description = throw $ ValidationError $ s <> ":" <> k <> ": " <> description

class KeyFileSection a where
  loadSection  :: GLib.KeyFile -> Text -> IO a
  storeSection :: GLib.KeyFile -> Text -> a -> IO ()

class KeyFileValue a where
  loadValue  :: GLib.KeyFile -> Text -> Text -> [Validator a] -> IO a
  storeValue :: GLib.KeyFile -> Text -> Text -> a -> IO ()

instance KeyFileSection Config where
  loadSection f _ = Config <$> loadSection f "general"
                           <*> loadSection f "panel"
                           <*> loadSection f "launcher"
                           <*> loadSection f "control-center"
                           <*> loadSection f "notifications"
  storeSection f _ c = do storeSection f "general"        c.general
                          storeSection f "panel"          c.panel
                          storeSection f "launcher"       c.launcher
                          storeSection f "control-center" c.controlCenter
                          storeSection f "notifications"  c.notifications

instance KeyFileSection GeneralConfig where
  loadSection f s = GeneralConfig <$> loadValue f s "stylesheet" [ validateFileURL ]
  storeSection f s c = do storeValue f s "stylesheet" c.stylesheet

instance KeyFileSection PanelConfig where
  loadSection f s = PanelConfig <$> loadValue f s "position"      []
                                <*> loadValue f s "margin"        [ validateNonNegative ]
                                <*> loadValue f s "padding"       [ validateNonNegative ]
                                <*> loadValue f s "size"          [ validateNonNegative, validateNonZero ]
                                <*> loadValue f s "spacing"       [ validateNonNegative ]
                                <*> loadValue f s "multi-monitor" []
                                <*> (mapM (loadSection f) =<< itemSections)
    where itemSections = filter (sectionPrefix `isPrefixOf`) . fst <$> f.getGroups
          sectionPrefix = s <> "."
  storeSection f s c = do storeValue f s "position"      c.position
                          storeValue f s "margin"        c.margin
                          storeValue f s "padding"       c.padding
                          storeValue f s "size"          c.size
                          storeValue f s "spacing"       c.spacing
                          storeValue f s "multi-monitor" c.multiMonitor
                          mapM_ storeItemSection c.items
    where storeItemSection c' = storeSection f (sectionPrefix <> itemName c') c'
          sectionPrefix = s <> "."
          itemName (PanelItemIcon       c' _) = c'.name
          itemName (PanelItemClock      c' _) = c'.name
          itemName (PanelItemWorkspaces c' _) = c'.name
          itemName (PanelItemStatus     c' _) = c'.name

instance KeyFileSection LauncherConfig where
  loadSection f s = LauncherConfig <$> loadValue f s "width"      [ validateNonNegative, validateNonZero ]
                                   <*> loadValue f s "position"   [ validateNonNegative ]
                                   <*> loadValue f s "max-height" [ validateNonNegative, validateNonZero ]
  storeSection f s c = do storeValue f s "width"      c.width
                          storeValue f s "position"   c.position
                          storeValue f s "max-height" c.maxHeight

instance KeyFileSection ControlCenterConfig where
  loadSection f s = ControlCenterConfig <$> loadValue f s "width"       [ validateNonNegative, validateNonZero ]
                                        <*> loadValue f s "height"      [ validateNonNegative, validateNonZero ]
                                        <*> loadValue f s "position"    []
                                        <*> loadValue f s "offset"      [ validateNonNegative ]
                                        <*> loadValue f s "padding"     [ validateNonNegative ]
                                        <*> loadValue f s "spacing"     [ validateNonNegative ]
                                        <*> loadValue f s "orientation" []
                                        <*> loadValue f s "alignment"   []
                                        <*> loadValue f s "rows"        [ validateDistinct ]
  storeSection f s c = do storeValue f s "width"       c.width
                          storeValue f s "height"      c.height
                          storeValue f s "position"    c.position
                          storeValue f s "offset"      c.offset
                          storeValue f s "padding"     c.padding
                          storeValue f s "spacing"     c.spacing
                          storeValue f s "orientation" c.orientation
                          storeValue f s "alignment"   c.alignment
                          storeValue f s "rows"        c.rows

instance KeyFileSection NotificationConfig where
  loadSection f s = NotificationConfig <$> loadValue f s "width"              [ validateNonNegative, validateNonZero ]
                                       <*> loadValue f s "min-height"         [ validateNonNegative ]
                                       <*> loadValue f s "spacing"            [ validateNonNegative ]
                                       <*> loadValue f s "close-button-inset" []
                                       <*> loadValue f s "time-format"        []
  storeSection f s c = do storeValue f s "width"              c.width
                          storeValue f s "min-height"         c.minHeight
                          storeValue f s "spacing"            c.spacing
                          storeValue f s "close-button-inset" c.closeButtonInset
                          storeValue f s "time-format"        c.timeFormat

instance KeyFileSection PanelItem where
  loadSection f s = do x <- loadSection f s
                       kind <- loadValue f s "kind" []
                       case kind :: PanelItemKind of
                         PanelItemKindIcon       -> PanelItemIcon       x <$> loadSection f s
                         PanelItemKindClock      -> PanelItemClock      x <$> loadSection f s
                         PanelItemKindWorkspaces -> PanelItemWorkspaces x <$> loadSection f s
                         PanelItemKindStatus     -> PanelItemStatus     x <$> loadSection f s
  storeSection f s c = case c of
                         PanelItemIcon       x y -> store PanelItemKindIcon       x y
                         PanelItemClock      x y -> store PanelItemKindClock      x y
                         PanelItemWorkspaces x y -> store PanelItemKindWorkspaces x y
                         PanelItemStatus     x y -> store PanelItemKindStatus     x y
    where store kind x y = do storeValue f s "kind" kind
                              storeSection f s x
                              storeSection f s y

instance KeyFileSection PanelItemConfiguration where
  loadSection f s = PanelItemConfiguration n <$> loadValue f s "position" []
    where n = Data.Text.drop 1 $ snd $ breakOn "." s
  storeSection f s c = do storeValue f s "position" c.position

instance KeyFileSection PanelIconConfiguration where
  loadSection f s = PanelIconConfiguration <$> loadValue f s "icon-name" []
                                           <*> loadValue f s "action"    []
  storeSection f s c = do storeValue f s "icon-name" c.iconName
                          storeValue f s "action"    c.action

instance KeyFileSection PanelClockConfiguration where
  loadSection f s = PanelClockConfiguration <$> loadValue f s "format" []
                                            <*> loadValue f s "action" []
  storeSection f s c = do storeValue f s "format" c.format
                          storeValue f s "action" c.action

instance KeyFileSection PanelWorkspacesConfiguration where
  loadSection f s = PanelWorkspacesConfiguration <$> loadValue f s "button-size" [ validateNonNegative, validateNonZero ]
                                                 <*> loadValue f s "spacing"     [ validateNonNegative ]
  storeSection f s c = do storeValue f s "button-size" c.buttonSize
                          storeValue f s "spacing"     c.spacing

instance KeyFileSection PanelStatusConfiguration where
  loadSection f s = PanelStatusConfiguration <$> loadValue f s "items"            [ validateDistinct ]
                                             <*> loadValue f s "spacing"          [ validateNonNegative ]
                                             <*> loadValue f s "show-labels"      []
                                             <*> loadValue f s "enable-scrolling" []
                                             <*> loadValue f s "action"           []
  storeSection f s c = do storeValue f s "items"            c.items
                          storeValue f s "spacing"          c.spacing
                          storeValue f s "show-labels"      c.showLabels
                          storeValue f s "enable-scrolling" c.enableScrolling
                          storeValue f s "action"           c.action

class (Bounded o, Enum o, Eq o) => Option o where
  readOption :: Text -> Maybe o
  showOption :: o -> Text

instance Option ScreenEdge where
  readOption "left"   = Just ScreenEdgeLeft
  readOption "right"  = Just ScreenEdgeRight
  readOption "top"    = Just ScreenEdgeTop
  readOption "bottom" = Just ScreenEdgeBottom
  readOption _        = Nothing
  showOption ScreenEdgeLeft   = "left"
  showOption ScreenEdgeRight  = "right"
  showOption ScreenEdgeTop    = "top"
  showOption ScreenEdgeBottom = "bottom"

instance Option Orientation where
  readOption "vertical"   = Just OrientationVertical
  readOption "horizontal" = Just OrientationHorizontal
  readOption _            = Nothing
  showOption OrientationVertical   = "vertical"
  showOption OrientationHorizontal = "horizontal"

instance Option ControlCenterAlignment where
  readOption "start"  = Just ControlCenterAlignmentStart
  readOption "center" = Just ControlCenterAlignmentCenter
  readOption "end"    = Just ControlCenterAlignmentEnd
  readOption "fill"   = Just ControlCenterAlignmentFill
  readOption _        = Nothing
  showOption ControlCenterAlignmentStart  = "start"
  showOption ControlCenterAlignmentCenter = "center"
  showOption ControlCenterAlignmentEnd    = "end"
  showOption ControlCenterAlignmentFill   = "fill"

instance Option ControlCenterRow where
  readOption "connectivity" = Just ControlCenterRowConnectivity
  readOption "audio-output" = Just ControlCenterRowAudioOutput
  readOption "audio-input"  = Just ControlCenterRowAudioInput
  readOption "brightness"   = Just ControlCenterRowBrightness
  readOption _              = Nothing
  showOption ControlCenterRowConnectivity = "connectivity"
  showOption ControlCenterRowAudioOutput  = "audio-output"
  showOption ControlCenterRowAudioInput   = "audio-input"
  showOption ControlCenterRowBrightness   = "brightness"

instance Option PanelItemPosition where
  readOption "start"  = Just PanelItemPositionStart
  readOption "center" = Just PanelItemPositionCenter
  readOption "end"    = Just PanelItemPositionEnd
  readOption _        = Nothing
  showOption PanelItemPositionStart  = "start"
  showOption PanelItemPositionCenter = "center"
  showOption PanelItemPositionEnd    = "end"

instance Option PanelStatusItem where
  readOption "network"       = Just PanelStatusItemNetwork
  readOption "bluetooth"     = Just PanelStatusItemBluetooth
  readOption "battery"       = Just PanelStatusItemBattery
  readOption "brightness"    = Just PanelStatusItemBrightness
  readOption "audio"         = Just PanelStatusItemAudio
  readOption "notifications" = Just PanelStatusItemNotifications
  readOption _               = Nothing
  showOption PanelStatusItemNetwork       = "network"
  showOption PanelStatusItemBluetooth     = "bluetooth"
  showOption PanelStatusItemBattery       = "battery"
  showOption PanelStatusItemBrightness    = "brightness"
  showOption PanelStatusItemAudio         = "audio"
  showOption PanelStatusItemNotifications = "notifications"

instance Option Action where
  readOption "none"           = Just ActionNone
  readOption "launcher"       = Just ActionLauncher
  readOption "control-center" = Just ActionControlCenter
  readOption _                = Nothing
  showOption ActionNone          = "none"
  showOption ActionLauncher      = "launcher"
  showOption ActionControlCenter = "control-center"

instance Option Bool where
  readOption "true"  = Just True
  readOption "false" = Just False
  readOption _       = Nothing
  showOption True  = "true"
  showOption False = "false"

data PanelItemKind
  = PanelItemKindIcon
  | PanelItemKindClock
  | PanelItemKindWorkspaces
  | PanelItemKindStatus
  deriving (Show, Enum, Bounded, Eq)

instance Option PanelItemKind where
  readOption "icon"       = Just PanelItemKindIcon
  readOption "clock"      = Just PanelItemKindClock
  readOption "workspaces" = Just PanelItemKindWorkspaces
  readOption "status"     = Just PanelItemKindStatus
  readOption _            = Nothing
  showOption PanelItemKindIcon       = "icon"
  showOption PanelItemKindClock      = "clock"
  showOption PanelItemKindWorkspaces = "workspaces"
  showOption PanelItemKindStatus     = "status"

instance KeyFileValue Text where
  loadValue f s k validators = do
    val <- convertError (throwValidationError s k) $ f.getString s k
    validate s k validators val
  storeValue f = f.setString

instance KeyFileValue Int where
  loadValue f s k validators = do
    val <- convertError (throwValidationError s k) $ f.getInteger s k
    validate s k validators (fromIntegral val)
  storeValue f s k val = f.setInteger s k (fromIntegral val)

instance Option o => KeyFileValue [o] where
  loadValue f s k validators = do
    (strs, _) <- convertError (throwValidationError s k) $ f.getStringList s k
    vals <- mapM (parseOption s k) strs
    validate s k validators vals
  storeValue f s k vals = f.setString s k $ intercalate ";" (fmap showOption vals)

instance {-# OVERLAPS #-} Option o => KeyFileValue o where
  loadValue f s k validators = do
    str <- convertError (throwValidationError s k) $ f.getString s k
    val <- parseOption s k str
    validate s k validators val
  storeValue f s k val = f.setString s k (showOption val)

convertError :: (Text -> IO a) -> IO a -> IO a
convertError t a = catch a (gerrorMessage >=> t)

parseOption :: forall o. Option o => Text -> Text -> Text -> IO o
parseOption s k str = case readOption str of
                        Just o  -> pure o
                        Nothing -> let values = [minBound..] :: [o] in
                                   let allowed = intercalate ", " (fmap showOption values) in
                                   let msg = "Invalid enum value \"" <> str <> "\" (allowed values: " <> allowed <> ")" in
                                   throwValidationError s k msg

validate :: Text -> Text -> [Validator a] -> a -> IO a
validate s k validators val = val <$ forM_ validators (singleValidate val)
  where singleValidate val' v = v val' >>= mapM_ (throwValidationError s k)

validateFileURL :: Validator Text
validateFileURL url = do
  f <- Gio.fileNewForUri url
  exists <- f.queryExists (Nothing :: Maybe Gio.Cancellable)
  pure $ if exists then Nothing
                   else Just $ "A file with the URL \"" <> url <> "\" does not exist."

validateNonNegative :: Validator Int
validateNonNegative x = pure $ if x >= 0 then Nothing
                                         else Just "Value is not allowed to be negative."

validateNonZero :: Validator Int
validateNonZero x = pure $ if x /= 0 then Nothing
                                     else Just "Value is not allowed to be 0."

validateDistinct :: Option o => Validator [o]
validateDistinct (x:xs) = if x `elem` xs then pure $ Just $ "Duplicate value \"" <> showOption x <> "\" not allowed."
                                         else validateDistinct xs
validateDistinct []     = pure Nothing

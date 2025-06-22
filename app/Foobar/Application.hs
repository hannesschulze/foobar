{-# LANGUAGE GHC2021 #-}
{-# LANGUAGE OverloadedStrings, OverloadedLabels, OverloadedRecordDot, ImplicitParams #-}

module Foobar.Application
  ( mkApplication
  ) where

import Data.Maybe (fromJust)
import Data.GI.Base
import GHC.Generics (Generic)
import Declarative.Gtk
import Foobar.Services.Config
import Foobar.Models.Config

import qualified GI.Gtk as Gtk
import qualified GI.Gdk as Gdk
import qualified GI.Gio as Gio
import qualified GI.Foobar as FB

newtype AppState
  = AppState { config :: Config }
  deriving (Generic)

data Message = MsgConfigChanged

mkApplication :: IO Gtk.Application
mkApplication = new Gtk.Application [ #applicationId := "com.github.hannesschulze.foobar-hs"
                                    , On #activate (onActivate ?self) ]

appView :: View AppState Message Gtk.Widget
appView state _ = widget Gtk.Label [ #label :<~ lbl ]
  where lbl = fmap ("Stylesheet: " <>) state.config.general.stylesheet

appHandler :: Handler Message AppState
appHandler MsgConfigChanged s = do newConfig <- loadConfig
                                   s.config <~ newConfig

onActivate :: Gtk.Application -> IO ()
onActivate app = do
  initialState <- AppState <$> initConfig
  provider <- new Gtk.CssProvider []
  provider.loadFromResource "/foobar/styles/default.css"
  disp <- Gdk.displayGetDefault
  Gtk.styleContextAddProviderForDisplay (fromJust disp)
                                        provider
                                        (fromIntegral Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
  (view, emit, _) <- fullMVH initialState appView appHandler
  observeConfig (emit MsgConfigChanged)
  window <- new Gtk.ApplicationWindow [ #application := app
                                      , #title := "Test"
                                      , #child := view ]
  _ <- FB.serverProxyNewForBusSync Gio.BusTypeSession
                                   []
                                   "com.github.hannesschulze.foobar"
                                   "/com/github/hannesschulze/foobar/server"
                                   (Nothing :: Maybe Gio.Cancellable)
  window.show

{-# LANGUAGE GHC2021 #-}
{-# LANGUAGE OverloadedStrings, OverloadedLabels, OverloadedRecordDot, ImplicitParams #-}

module Foobar.Application
  ( mkApplication
  ) where

import Data.Maybe (fromJust)
import Data.GI.Base
import Data.Text
import GHC.Generics (Generic)
import Declarative.Gtk

import qualified GI.Gtk as Gtk
import qualified GI.Gdk as Gdk
import qualified GI.Gio as Gio
import qualified GI.Foobar as FB

mkApplication :: IO Gtk.Application
mkApplication = new Gtk.Application [ #applicationId := "com.github.hannesschulze.foobar-hs"
                                    , On #activate (onActivate ?self) ]

newtype AppState
  = AppState { stateText :: Text }
  deriving (Generic)

data Message = MsgQuit | MsgDoSomething

initialState :: AppState
initialState = AppState { stateText = "Do something" }

appView :: View AppState Message Gtk.Widget
appView state emit = widget DeclarativeBox [ #children := children
                                           , #orientation := Gtk.OrientationVertical
                                           , #spacing := 12]
  where children = [ widget Gtk.Button [ #label :<~ state.stateText
                                       , On #clicked (emit MsgDoSomething) ]
                   , widget Gtk.Button [ #label := "Quit"
                                       , On #clicked (emit MsgQuit) ] ]

appHandler :: Handler Message AppState
appHandler MsgQuit _ = pure ()
appHandler MsgDoSomething s = s.stateText <~ "Did something!"

onActivate :: Gtk.Application -> IO ()
onActivate app = do
  provider <- new Gtk.CssProvider []
  provider.loadFromResource "/foobar/styles/default.css"
  disp <- Gdk.displayGetDefault
  Gtk.styleContextAddProviderForDisplay (fromJust disp)
                                        provider
                                        (fromIntegral Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
  view <- mvh initialState appView appHandler
  window <- new Gtk.ApplicationWindow [ #application := app
                                      , #title := "Test"
                                      , #child := view ]
  _ <- FB.serverProxyNewForBusSync Gio.BusTypeSession
                                   []
                                   "com.github.hannesschulze.foobar"
                                   "/com/github/hannesschulze/foobar/server"
                                   (Nothing :: Maybe Gio.Cancellable)
  window.show

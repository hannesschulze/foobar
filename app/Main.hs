{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE OverloadedLabels #-}
{-# LANGUAGE OverloadedRecordDot #-}
{-# LANGUAGE ImplicitParams #-}

import Control.Monad (void)
import Control.Exception (bracket_)
import Data.Maybe (fromJust)
import Data.GI.Base

import qualified GI.Gtk as Gtk
import qualified GI.Gdk as Gdk
import qualified GI.Gio as Gio
import qualified GI.Foobar as FB

activate :: Gtk.Application -> IO ()
activate app = do
  provider <- new Gtk.CssProvider []
  provider.loadFromResource "/foobar/styles/default.css"
  disp <- Gdk.displayGetDefault
  Gtk.styleContextAddProviderForDisplay (fromJust disp)
                                        provider
                                        (fromIntegral Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
  button <- new Gtk.Button [ #label := "Hello, world!" ]
  window <- new Gtk.ApplicationWindow [ #application := app
                                      , #title := "Test"
                                      , #child := button ]
  _ <- FB.serverProxyNewForBusSync Gio.BusTypeSession
                                   []
                                   "com.github.hannesschulze.foobar"
                                   "/com/github/hannesschulze/foobar/server"
                                   (Nothing :: Maybe Gio.Cancellable)
  window.show

main :: IO ()
main = do
  bracket_
    (do FB.stylesRegisterResource; FB.iconsRegisterResource)
    (do FB.stylesUnregisterResource; FB.iconsUnregisterResource) $
    do app <- new Gtk.Application [ #applicationId := "com.github.hannesschulze.foobar-hs"
                                  , On #activate (activate ?self) ]
       void $ app.run Nothing

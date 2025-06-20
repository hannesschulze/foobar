{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE OverloadedLabels #-}
{-# LANGUAGE OverloadedRecordDot #-}
{-# LANGUAGE ImplicitParams #-}

import Control.Monad (void)
import Data.GI.Base
import qualified GI.Gtk as Gtk
import qualified GI.Gio as Gio
import qualified GI.Foobar as FB

activate :: Gtk.Application -> IO ()
activate app = do
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
  app <- new Gtk.Application [ #applicationId := "com.github.hannesschulze.foobar-hs"
                             , On #activate (activate ?self) ]
  void $ app.run Nothing

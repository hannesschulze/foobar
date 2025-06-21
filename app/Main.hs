{-# LANGUAGE OverloadedStrings, OverloadedRecordDot #-}

import Control.Monad (void)
import Control.Exception (bracket_)
import System.Environment (getArgs)
import Foobar.Application (mkApplication)

import qualified GI.Foobar as FB

main :: IO ()
main = bracket_ FB.stylesRegisterResource FB.stylesUnregisterResource $
       bracket_ FB.iconsRegisterResource  FB.iconsUnregisterResource  $
         do args <- getArgs
            app  <- mkApplication
            void $ app.run (Just ("com.github.hannesschulze.foobar" : args))

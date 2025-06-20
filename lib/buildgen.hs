{-# LANGUAGE OverloadedStrings, DeriveGeneric #-}

-- Based on:
-- https://raw.githubusercontent.com/haskell-gi/haskell-gi/refs/heads/master/bindings/genBuildInfo.hs

-- Generate the cabal info for the given subdirectories, assuming
-- the existence of appropriate "pkg.info" files.

import System.Directory (createDirectory, doesPathExist, makeAbsolute)
import System.Environment (getArgs)
import System.FilePath ((</>), (<.>), takeBaseName, takeDirectory)
import System.IO (hPutStrLn, stderr, hFlush, stdout)
import System.Exit (exitFailure)

#if !MIN_VERSION_base(4,13,0)
import Data.Monoid ((<>))
#endif

import GHC.Generics
import qualified Data.Aeson as A
import qualified Data.ByteString as B
import Data.Maybe (fromMaybe, fromJust)
import qualified Data.Set as S
import qualified Data.Text.Encoding as TE
import qualified Data.Text as T
import Data.Text (Text)

import Data.GI.CodeGen.CabalHooks (configureDryRun)
import qualified Data.GI.CodeGen.ProjectInfo as PI
import Data.GI.CodeGen.Util (ucFirst)

import qualified Data.ByteString.Lazy as LB

data ProjectInfo = ProjectInfo {
      name              :: T.Text -- ^ Name of the haskell package
    , version           :: T.Text -- ^ Its version
    , author            :: Maybe T.Text -- ^ Auhor. If not specified the
                                        -- current maintainer (from
                                        -- Data.GI.CodeGen.ProjectInfo)
                                        -- will be used.
    , synopsis          :: T.Text
    , description       :: T.Text
    , giDepends         :: [T.Text] -- ^ dependencies on other
                                    -- haskell-gi generated modules.
    , baseVersion       :: T.Text -- ^ Constraint on base for this module.

    , girName           :: T.Text         -- ^ GIR file (without version)
    , girVersion        :: T.Text         -- ^ Its version
    , girOverrides      :: Maybe FilePath   -- ^ Possibly an overrides file
    , compat            :: Maybe T.Text
    } deriving (Show, Generic)

instance A.FromJSON ProjectInfo
instance A.ToJSON ProjectInfo

readGIRInfo :: FilePath -> IO ProjectInfo
readGIRInfo fname = do
  buf <- LB.readFile fname
  case A.eitherDecode buf of
    Left err -> error ("Could not parse \"" <> fname <> "\": " <> err)
    Right info -> return info

writeCabal :: FilePath -> FilePath -> ProjectInfo -> [Text] -> IO ()
writeCabal fname lib info exposed =
    B.writeFile fname $ TE.encodeUtf8 $ T.unlines $
       [ "name:                 " <> name info
       , "version:              " <> version info
       , "synopsis:             " <> synopsis info
       , "description:          " <> description info
       , "homepage:             " <> PI.homepage
       , "license:              " <> PI.license
       , "author:               " <> fromMaybe PI.maintainers (author info)
       , "maintainer:           " <> PI.maintainers
       , "category:             " <> PI.category
       , "build-type:           Custom"
       , "cabal-version:        2.0"
       , let commonFiles = "README.md ChangeLog.md stack.yaml"
         in case girOverrides info of
           Nothing -> "\nextra-source-files: " <> commonFiles <> "\n"
           Just ov -> "\nextra-source-files: " <> commonFiles <> " "
                      <> T.pack ov <> "\n"
       , "custom-setup"
       , "      setup-depends: " <>
         T.intercalate ",\n                     "
           ([ "base >= 4.11 && < 5"
            , "Cabal >= 1.24 && < 4"
            , "haskell-gi >= 0.26.14 && < 0.27"]
            <> giDepends info)
       , ""
       , "library"
       , "      default-language: " <> PI.defaultLanguage
       , "      default-extensions: " <> T.intercalate ", " PI.defaultExtensions
       , "      other-extensions: " <> T.intercalate ", " PI.otherExtensions
       , "      ghc-options: " <> T.intercalate " " PI.ghcOptions
       , ""
       , "      extra-lib-dirs: " <> T.pack (takeDirectory lib)
       , "      extra-libraries: " <> fromJust (T.stripPrefix "lib" $ T.pack $ takeBaseName lib)
       , "      build-depends: " <>
         T.intercalate ",\n                     "
              ([ baseVersion info
               , "haskell-gi-base >= 0.26 && < 0.27"
               -- Workaround for cabal new-build not picking up
               -- setup-depends dependencies when constructing the
               -- build plan.
               , "haskell-gi >= 0.26.14 && < 0.27"
               -- See https://github.com/haskell-gi/haskell-gi/issues/124
               -- for the reasoning behind this.
               , "haskell-gi-overloading < 1.1" ]
               <> giDepends info <> PI.standardDeps)
       , ""
       -- GHC 8.2.x panics when building the overloaded bindings
       -- https://ghc.haskell.org/trac/ghc/ticket/14382
       , "      -- Disable overloading when compiling under GHC 8.2.x"
       , "      -- see https://ghc.haskell.org/trac/ghc/ticket/14382"
       , "      if impl(ghc == 8.2.*)"
       , "              build-depends: haskell-gi-overloading == 0.0"
       , ""
       , "      -- Note that the following list of exposed modules and autogen"
       , "      -- modules is for documentation purposes only, so that some"
       , "      -- documentation appears in hackage. The actual list of modules"
       , "      -- to be built will be built at configure time, based on the"
       , "      -- available introspection data."
       , ""
       , "      exposed-modules: " <>
         T.intercalate ",\n                       " exposed
       , ""
       , "      autogen-modules: " <>
         T.intercalate ",\n                       " exposed
       ]

writeSetup :: FilePath -> ProjectInfo -> S.Set Text -> IO ()
writeSetup fname info deps =
    B.writeFile fname $ TE.encodeUtf8 $ T.unlines
           [ "{-# LANGUAGE OverloadedStrings #-}"
           , ""
           , "import Data.GI.CodeGen.CabalHooks (setupBinding, TaggedOverride(..))"
           , ""
           , T.unlines (map buildInfo (S.toList deps))
           , ""
           , "main :: IO ()"
           , "main = setupBinding name version pkgName pkgVersion verbose overridesFile inheritedOverrides outputDir"
           , "  where name = " <> tshow (girName info)
           , "        version = " <> tshow (girVersion info)
           , "        pkgName = " <> tshow (name info)
           , "        pkgVersion = " <> tshow (version info)
           , "        overridesFile = " <> tshow (girOverrides info)
           , "        verbose = False"
           , "        outputDir = Nothing"
           , "        inheritedOverrides = ["
             <> T.intercalate ", " (map inheritedOverride (S.toList deps))
             <> "]"
           ]
    where tshow :: Show a => a -> Text
          tshow = T.pack . show

          buildInfo :: Text -> Text
          buildInfo dep = let capDep = ucFirst dep in
            "import qualified GI." <> capDep <> ".Config as " <> capDep

          inheritedOverride :: Text -> Text
          inheritedOverride dep = let capDep = ucFirst dep in
            "TaggedOverride \"inherited:" <> capDep <> "\" "
                      <> capDep <> ".overrides"

exposedModulesAndDeps :: FilePath -> ProjectInfo -> IO ([Text], S.Set Text)
exposedModulesAndDeps dir info =
  configureDryRun (girName info) (girVersion info) (name info) (version info)
                  ((dir </>) <$> girOverrides info) []

-- | Write a "compat" package re-exporting all symbols from the
-- current pakage.
writeCompatPkg :: FilePath -> Text -> ProjectInfo -> IO ()
writeCompatPkg dir compatPkg info = do
  alreadyThere <- doesPathExist dir
  if alreadyThere
    then return ()
    else do
      putStr $ " [+ " <> T.unpack compatPkg <> " ]"
      hFlush stdout
      createDirectory dir
      writeCompatProject (dir </> "cabal.project") compatPkg
      writeCompatCabal (dir </> T.unpack compatPkg <.> ".cabal") compatPkg info
      writeCompatSetup (dir </> "Setup.hs") info
      writeCompatReadme (dir </> "README.md") info

-- | The .cabal file for the compatibility package.
writeCompatCabal :: FilePath -> Text -> ProjectInfo -> IO ()
writeCompatCabal fname compatPkg info =
  B.writeFile fname $ TE.encodeUtf8 $ T.unlines
  [   "name:           " <> compatPkg
    , "version:        " <> version info
    , "synopsis:       " <> synopsis info <> " (compatibility layer)"
    , "description:    This package re-exports (for backward compatibility)"
    , "                the haskell-gi generated bindings in the " <> name info <> " package."
    , "homepage:       " <> PI.homepage
    , "license:        " <> PI.license
    , "license-file:   LICENSE"
    , "author:         " <> fromMaybe PI.maintainers (author info)
    , "maintainer:     " <> PI.maintainers
    , "category:       " <> PI.category
    , "build-type:     Custom"
    , "cabal-version:  2.0"
    , ""
    , "extra-source-files: README.md"
    , ""
    , "custom-setup"
    , " setup-depends:"
    , "   base >= 4.11 && <5,"
    -- For compatibility with stack, see
    -- https://github.com/haskell-gi/haskell-gi/issues/463
    , "   Cabal >= 1.24 && < 4,"
    , "   haskell-gi ^>= 0.26.14,"
    , "   " <> name info <> " ^>= " <> version info
    , ""
    , "library"
    , "    ghc-options: -Wall"
    , ""
    , "    build-depends: base >= 4.11 && <5,"
    , "                   " <> name info <> " ^>= " <> version info
    , ""
    , "    default-language: Haskell2010"
    ]

writeCompatProject :: FilePath -> Text -> IO ()
writeCompatProject fname compatPkg =
  B.writeFile fname $ TE.encodeUtf8 $ "packages: " <> compatPkg <> ".cabal"

writeCompatSetup :: FilePath -> ProjectInfo -> IO ()
writeCompatSetup fname info =
  let cfgMod = "GI." <> ucFirst (girName info) <> ".Config"
  in B.writeFile fname $ TE.encodeUtf8 $ T.unlines
  [ "import Data.GI.CodeGen.CabalHooks (setupCompatWrapper)"
  , "import qualified " <> cfgMod <> " as Cfg"
  , ""
  , "main :: IO ()"
  , "main = setupCompatWrapper \"" <> name info <> "\" Cfg.modules"
  ]

writeCompatReadme :: FilePath -> ProjectInfo -> IO ()
writeCompatReadme fname info =
  let link = "[" <> name info <> "](/package/" <> name info <> ")"
  in B.writeFile fname $ TE.encodeUtf8 $ T.unlines
  [ "# Information"
  , "This is a compatibility package. For newer projects we recommend that you use " <> link <> " instead."
  ]

genBindingsInDir :: FilePath -> FilePath -> IO ()
genBindingsInDir dir lib = do
  info <- readGIRInfo (dir </> "pkg.info")
  putStr $ "Generating " <> T.unpack (name info) <> "-" <> T.unpack (version info) <> " ..."
  hFlush stdout
  (exposed, deps) <- exposedModulesAndDeps dir info
  writeCabal (dir </> T.unpack (name info) <.> "cabal") lib info exposed
  writeSetup (dir </> "Setup.hs") info deps
  case compat info of
    Nothing -> return ()
    Just compatPkg -> writeCompatPkg (dir </> "compat") compatPkg info
  putStrLn " done."

main :: IO ()
main = do
  args <- getArgs

  case args of
    [lib, srcdir] -> do absLib <- makeAbsolute lib
                        genBindingsInDir srcdir absLib
    _ -> do hPutStrLn stderr "usage: buildgen <lib> <srcdir>"
            exitFailure

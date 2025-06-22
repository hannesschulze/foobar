{-# LANGUAGE OverloadedRecordDot, DuplicateRecordFields #-}

-- |
-- Module     : Foobar.Services.Coalescing
-- Description: Coalesce multiple events using a timeout.

module Foobar.Services.Coalescing
  ( EventScheduler
  , mkEventScheduler
  , scheduleEvent
  ) where

import Data.IORef (IORef, newIORef, readIORef, writeIORef)
import Data.Word (Word32)

import qualified GI.GLib as GLib

-- | Management structure for coalescing events.
data EventScheduler
  = EventScheduler { delay    :: !Int
                   , sourceId :: !(IORef (Maybe Word32)) }

-- | Create a new event scheduler for coalescing events.
mkEventScheduler
  :: Int               -- ^ Delay for scheduling events in milliseconds.
  -> IO EventScheduler -- ^ The event scheduler structure.
mkEventScheduler delay' = EventScheduler delay' <$> newIORef Nothing

-- | Schedule an event.
--
-- If there is already an event scheduled, the two are coalesced.
scheduleEvent
  :: EventScheduler -- ^ The event scheduler.
  -> IO ()          -- ^ The event to be scheduled.
  -> IO ()
scheduleEvent s event = do
  existingId <- readIORef s.sourceId
  let sourceFunc :: GLib.SourceFunc
      sourceFunc = do
        event
        writeIORef s.sourceId Nothing
        pure GLib.SOURCE_REMOVE
  case existingId of
    Just _  -> pure ()
    Nothing -> do newId <- GLib.timeoutAdd GLib.PRIORITY_DEFAULT (fromIntegral s.delay) sourceFunc
                  writeIORef s.sourceId (Just newId)

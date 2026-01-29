# Architectural Refactoring: Sequencer Optimization

## Issue
The original `Sequencer::onTick` method performed a deep copy of the entire `Song` structure on every clock tick (e.g., 960 times per second at 120BPM/480PPQN). This `malloc` inside the high-priority audio thread would eventually cause severe timing jitter and dropouts as the song size increased.

## Solution: Playback Queue
We have refactored the Sequencer to separate the **Editing Model** from the **Playback Model**.

1.  **Playback Queue:**
    - A specialized `std::vector<PlaybackEvent>` is now created when `play()` is called.
    - These events are "flattened" (absolute timestamp = item start + event relative tick) and sorted by time.
    - This converts the complex $O(N)$ lookup across tracks and items into a simple $O(1)$ queue consumption.

2.  **Lock-Free Playback (Partial):**
    - `onTick` no longer locks the main `song_` mutex.
    - `onTick` iterates the pre-computed `playbackQueue_`, which is invariant during playback.

3.  **Trade-offs:**
    - **Edit-While-Playing:** Changes made to the Song during playback will not be heard until the sequencer is stopped and restarted. This is acceptable for the target "V1" linear workflow.

## Next Steps
- Verify timing stability with a large number of events.
- Implement "Panic" (All Notes Off) on Stop.
- Add "Pause" functionality (which should not clear the queue, but just stop the clock).

### Fix Playback Channel (2026-01-28)
- Problem: Sequencer was ignoring Track channel and using Event channel (usually 0).
- Fix: Updated `Sequencer::buildPlaybackQueue` to override playback event channel with `Track::channel`.

### Refactor TrackRowView (2026-01-28)
- Extracted `getPixelsPerTick` and `getItemRect` helpers to remove duplicated layout logic in `draw` and `handle`.
- Introduced `HEADER_WIDTH constant`.

### Refactor Constants (2026-01-29)
- Introduced `DEFAULT_PPQN` (480) and `DEFAULT_BPM` (120.0) in `Types.h`.
- Replaced hardcoded values across `Clock`, `EventList`, `TrackView`, and `MainWindow`.

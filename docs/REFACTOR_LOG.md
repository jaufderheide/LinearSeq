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

## Event Handling Architecture Overhaul (2026-01-30)

### Issue
Copy/paste functionality (Ctrl+C/V) was broken after implementing delete features. The paste operation would:
- Work after adding a new track
- Work when EventList had focus
- Fail when TrackView/TrackRowView had focus

Root cause: FLTK's event delivery model was allowing child widgets (specifically `Fl_Int_Input` for channel numbers) to consume keyboard shortcuts before application-level handlers could process them.

### Solution: Multi-Layer Event Interception

1. **Global Event Handler**
   - Registered via `Fl::add_handler()` to intercept events BEFORE widget delivery
   - Handles `FL_SHORTCUT`, `FL_KEYDOWN`, and `FL_KEYBOARD` event types
   - Provides fallback when widget-level handling fails

2. **Widget-Level Pass-Through**
   - `TrackRowView`, `TrackView`: Check for Ctrl+C/V/X/Z BEFORE calling `Fl_Group::handle()`
   - Return 0 immediately for application shortcuts to let them bubble up
   - Prevents `Fl_Int_Input` from consuming Ctrl+V for its built-in paste

3. **Duplicate Event Filtering**
   - FLTK can send multiple event types for a single keypress (FL_KEYDOWN, FL_KEYBOARD, event type 0)
   - Implemented `lastHandledShortcutKey_` tracking in MainWindow
   - Only handle each key once per Ctrl press, reset on Ctrl release
   - Excludes FL_KEYUP to avoid processing release events

4. **Delete Key Handling**
   - Separated from Ctrl shortcuts (doesn't require Ctrl modifier)
   - Checks focus ancestry to only trigger when TrackView or EventList is focused
   - Prevents interference with text input widgets

5. **Focus Management**
   - Automatically transfer focus to TrackView after creating items
   - Ensures Delete key works immediately without extra clicks

### Technical Details
- Event type 0 (FL_NO_EVENT) observed in certain focus states
- Event type 8 (FL_KEYDOWN) standard for key presses
- Event type 9 (FL_KEYUP) for key releases
- Event type 12 (FL_SHORTCUT) for application shortcuts
- Different widgets receive different event types for same keypress

### Outcome
- Consistent copy/paste behavior across all focus states
- No duplicate operations from multiple event deliveries
- Application-level shortcuts properly prioritized over widget defaults
- Delete key works immediately after adding items

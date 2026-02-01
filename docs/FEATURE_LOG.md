# Feature Log

## 2026-01-27

### App window (FLTK shell)
- Status: Functional
- Scope: Launches a window with Track View and Event List regions.
- Limitations: No interactive controls yet.
- How to verify: Run ./LinearSeq and confirm the window appears.

### Event List rendering
- Status: Partial
- Scope: Renders MIDI event rows from an in-memory demo Song.
- Limitations: Read-only; no in-grid editing yet.
- How to verify: Launch the app and confirm event rows show timestamps, status, data, and duration.

### Track View rendering + selection
- Status: Partial
- Scope: Renders track rows and item blocks from a demo Song. Clicking a track filters the Event List to that track.
- Limitations: No drag/move/resize for items; no zoom or scroll.
- How to verify: Launch the app, click a track row, and confirm Event List updates.

### ALSA output driver (note on/off)
- Status: Partial
- Scope: Sequencer can emit note on/off events to ALSA if provided a Song.
- Limitations: No song scheduling UI; no MIDI routing or clock control yet.
- How to verify: Unit test or manual wiring required (not exposed in UI).

### Transport controls + recording
- Status: Partial
- Scope: Play/Stop/Rec buttons drive Sequencer. Recording captures ALSA input into the active track/item.
- Limitations: No device selection UI; recording is basic and updates view on stop. ALSA may be unavailable in WSL.
- How to verify: Connect a MIDI device to ALSA, press Rec*, play notes, press Rec to stop, and confirm new events appear in Event List.

### ALSA status + simulated take
- Status: Partial
- Scope: UI shows ALSA availability and provides a Sim button to add demo events when no MIDI input is present.
- Limitations: Simulation is not real-time and does not test ALSA I/O.
- How to verify: Click Sim and confirm new events appear in the Event List.

### Item/note creation controls
- Status: Partial
- Scope: +Item creates a new MIDI item on the selected track. +Note adds a note to the active item.
- Limitations: Fixed note/velocity and auto placement; no editing yet.
- How to verify: Click +Item then +Note; confirm new item and rows appear.

### Track name editing
- Status: Partial
- Scope: Track name is editable via the toolbar input for the selected track.
- Limitations: No validation and no per-track label list yet.

### Delete Item
- Status: Functional
- Scope: Deletes selected MIDI items from tracks using Delete/Backspace keys. Support for multi-selection.
- Limitations: Requires TrackView or EventList to have focus.
- How to verify: Select one or multiple items in TrackView and press Delete.

- How to verify: Select a track row, edit the Track input, and confirm the Track View label updates.

### Add Track
- Status: Partial
- Scope: +Track adds a new track and selects it.
- Limitations: No per-track MIDI routing yet.
- How to verify: Click +Track and confirm a new track row appears.

### Track/Event list scrolling
- Status: Partial
- Scope: Track View and Event List scroll vertically based on content.
- Limitations: No horizontal scrolling yet.
- How to verify: Add enough tracks/notes to exceed the view and scroll.

### Song JSON save/load
- Status: Partial
- Scope: Save and load the in-memory Song model to a JSON file.
- Limitations: No UI wiring yet; format may evolve.
- How to verify: Call `SongJson::saveToFile()` / `SongJson::loadFromFile()` with a test path.

### File menu (kebab)
- Status: Partial
- Scope: Toolbar kebab menu provides Save/Load JSON.
- Limitations: No MIDI export yet; no recent files.
- How to verify: Click the kebab menu and choose Save/Load JSON.

### Feature: MIDI Out Selector (2026-01-28)
- Added `AlsaDriver::listOutputPorts` and `connectOutput`.
- Added `Fl_Choice` dropdown to Top Toolbar.
- Wiring: Selecting an item in the dropdown calls `snd_seq_subscribe_port`.

### Feature: Save/Load MIDI Device (2026-01-28)
- Added `midiDevice` to `Song` struct and JSON serialization.
- UI now saves the active MIDI device name and attempts to restore/connect on load.

### Feature: BPM/PPQN Controls (2026-01-29)
- Added `Fl_Spinner` for BPM and `Fl_Choice` for PPQN in top toolbar.
- Wired to update `Song` and `Sequencer` in real-time.

### Feature: Track Deletion (2026-01-29)
- Added `Delete Track` button to Toolbar.
- Logic removes track, updates pointers, handles last-track scenarios, and refreshes UI.

### Fix: Copy/Paste Event Handling (2026-01-30)
- Fixed broken Ctrl+V paste functionality that was inconsistent based on widget focus.
- Implemented multi-layered event handling strategy:
  - Global event handler for application-level shortcuts
  - Widget-level pass-through for Ctrl shortcuts before children consume them
  - Duplicate event filtering to handle FLTK's multiple event types per keypress
- Prevents child widgets (Fl_Int_Input) from consuming Ctrl+V before application can process it.
- Fixed focus issue: Added automatic focus to TrackView after creating items.
- Result: Consistent copy/paste behavior regardless of focus state.

### Feature: Playback from Playhead Position (2026-01-30)
- Modified `Sequencer::play()` to accept starting tick parameter.
- Updated `Clock::start()` to begin from specified tick instead of always starting at 0.
- `MainWindow` now passes `currentTick_` to sequencer when play is pressed.
- Playback queue skips events before startTick for efficient seeking.
- Enables working on specific sections of a composition without restarting from beginning.

### Feature: All Notes Off / MIDI Panic (2026-01-30)
- Implemented `AlsaDriver::sendAllNotesOff()` using MIDI CC 123 on all 16 channels.
- `Sequencer::stop()` now calls `allNotesOff()` to prevent stuck notes.
- Clears pending note-off events when stopping playback.
- Ensures clean stop regardless of where playback is interrupted.
- Critical for live performance and composition workflow.

### Feature: Snap Playhead to Measure (2026-01-30)
- Clicking in timeline now automatically snaps playhead to nearest measure boundary.
- Uses standard rounding: clicks in first half of measure snap to start, second half snap to next measure.
- Hold Shift while clicking for fine positioning without snapping.
- Simplifies workflow when working with measure-aligned compositions.
- Snap logic: `(tick + ticksPerMeasure/2) / ticksPerMeasure * ticksPerMeasure`

### Feature: Control Change and Program Change Support (2026-01-30)
- Implemented full MIDI Control Change (CC) and Program Change (PC) support.
- Added `AlsaDriver::sendProgramChange()` for patch/instrument selection.
- `AlsaDriver::sendControlChange()` now used for both CC events and All Notes Off.
- Sequencer playback queue now processes CC and PC events alongside notes.
- EventList allows changing event type: press 'n' for Note On, 'c' for CC, 'p' for PC.
- Event type changes automatically set reasonable defaults (e.g., PC clears data2).
- Enables setting up synth patches and controlling parameters via MIDI.
- Essential for consistent playback experience across sessions.

### Feature: Auto-Stop at End of Sequence (2026-01-30)
- Playback now automatically stops when all events have been played.
- Waits for pending note-off events to complete before stopping.
- Sends All Notes Off for clean shutdown.
- Prevents infinite playback loop on empty timeline sections.
- Implemented in `Sequencer::onTick()` - checks when `playbackIndex_ >= playbackQueue_.size()`.
- Fixed deadlock issue by using `stopRequested_` flag checked on UI thread.

### Feature: Rewind Button (2026-01-30)
- Added Rewind button to toolbar between Play and Stop buttons.
- Uses Font Awesome backward icon (\\uf04a).
- Resets playhead to time 0:1:0 (start of sequence).
- Automatically scrolls timeline view back to show the beginning.
- Enables quick return to start without manual scrolling.
- Simple workflow for iterating on compositions.

### Feature: Transport Button Visual Sync (2026-01-31)
- Play button now shows visual "pressed" state during playback.
- Background changes to green when sequence is playing.
- Automatically clears when stopped (manual stop or auto-stop at end).
- Provides clear visual feedback of playback status.
- Implemented in `MainToolbar::setPlaying()` with color change to `fl_rgb_color(0, 150, 0)`.

### Feature: Auto-Scroll During Playback (2026-01-31)
- Timeline automatically scrolls when playhead moves past visible area.
- Scrolls to show current measure at the left edge of the viewport.
- Calculation based on measure boundaries (100 pixels per measure).
- Smooth following during playback without manual scrolling.
- Implemented in `MainWindow::playTimer()` with viewport boundary detection.
- Enables comfortable playback of long sequences without losing sight of playhead.

### Feature: Status Bar (2026-01-31)
- Added dedicated status bar at bottom of window (24px height).
- **Left side**: Current playhead position in M:B:T (Measure:Beat:Tick) format.
- **Right side**: ALSA connection status with color coding (green=ready, red=unavailable).
- Updates in real-time during playback (33ms refresh rate).
- Removed status display from toolbar to free up space.
- Dark background (FL_DARK1) with white text for better contrast.
- Connection status shows device name when connected to specific MIDI port.

### Feature: Event Multi-Select (2026-01-31) - Issue #10
- Added multi-selection support in EventList with visual feedback.
- **Click**: Single selection (selects clicked row).
- **Shift+Click**: Range selection from cursor to clicked row.
- **Ctrl+Click**: Toggle individual row in/out of selection.
- Selected rows highlighted with blue-gray background.
- Delete key deletes all selected events (in reverse order to preserve indices).
- Selection cleared after operations like delete or paste.

### Feature: Event Copy/Paste with Cross-Item Support (2026-01-31) - Issue #11
- Full copy/paste workflow for MIDI events with multi-select support.
- **Copy (Ctrl+C)**: Copies selected events (or cursor row if no selection).
  - Events normalized to relative ticks (first event at tick 0).
  - Clipboard is item-agnostic - can paste into any item.
- **Paste (Ctrl+V)**: Pastes at cursor position or virtual blank row.
  - Cursor position: Pastes at exact tick of cursor row.
  - Virtual blank row: Pastes at next beat after last event (for appending).
  - Pasted events adopt target track's channel.
  - Newly pasted events automatically selected for easy repositioning.
- **Cross-item paste**: Copy from one item, click different item, paste.
- Overwrite mode: Events can coexist at same tick (normal MIDI behavior).
- Virtual blank row always shown after last event labeled "(insert here)".
- Supports copying events between different tracks and items seamlessly.

### Feature: Click Item to Move Playhead (2026-02-01) - Issue #9
- Clicking on any MIDI item in TrackView automatically moves playhead to item start.
- Provides quick navigation to specific sections of the sequence.
- Works with all item selection modes (single, Ctrl+Click, Shift+Click).
- Playhead position immediately visible in timeline and status bar.

### Feature: Solo Track (2026-02-01) - Issue #8
- Added solo button ("S") to each track row before track name.
- **Yellow highlight**: Track is soloed and will play exclusively.
- **Solo mode logic**: When any track is soloed, only soloed tracks play.
- Multiple tracks can be soloed simultaneously for isolation of specific parts.
- Solo state persists in song file (`.lseq` format).
- Playback queue filtered in `Sequencer::buildPlaybackQueue()` based on solo state.
- Essential for composition workflow and live performance mixing.

### Feature: File Edit Status Tracking (2026-02-01) - Issue #12
- Window title shows file edit state: `LinearSeq - filename [modified]`.
- Tracks modifications to song, tracks, items, events, channels, solo state, etc.
- Modified flag set on any change: add/delete track or item, edit events, change channel.
- Modified flag cleared on save or load operations.
- Filename extracted from file path (without `.lseq` extension).
- Provides clear visual feedback for unsaved changes.

### Feature: Confirm Save on Exit (2026-02-01) - Issue #13
- Dialog appears when closing window with unsaved changes.
- Three options: **Save** (saves then exits), **Don't Save** (exits without saving), **Cancel** (returns to app).
- Prevents accidental data loss during composition sessions.
- Integrated with file edit status tracking (Issue #12).
- Uses FLTK `fl_choice()` for native dialog appearance.

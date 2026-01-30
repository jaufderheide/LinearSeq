# Backlog

## UI / Usability
- [ ] **Multi Select Copy/Paste**: Currently Copy/Paste only works with single MidiItems

## Core Features
- [x] **Track Deletion**: Ability to delete selected tracks.
- [x] **Copy/Paste Items**: Fixed event handling for consistent Ctrl+C/V behavior.
- [x] **Send MIDI All Notes Off on Stop**: Prevents stuck notes when stopping playback mid-sequence.
- [x] **Start playback from the playhead location**: Playback now begins at current playhead position.
- [x] **Snap playhead to measure start**: Clicking in timeline snaps to nearest measure. Hold Shift for fine positioning.
- [ ] **Allow Program Change Event Type**: We'll need to implement a dropdown in the event list for an enumeration of valid event types.
                                           (This will allow us to set up synths)
- [ ] **Allow Conrol Change Event Type**: This likely means we need LSB and MSB columns as well (This will allow us to set up synths)

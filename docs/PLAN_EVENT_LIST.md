# Implementation Plan: Interactive Event List

## Goal
Implement a keyboard-centric, spreadsheet-style editor for MIDI events, similar to Cakewalk (DOS).

## Phase 1: Data Structure & Rendering (Read-Only Cursor)
**Objective:** Establish a stable mapping between the visual rows and the underlying `MidiEvent` data, and implement a visual cursor.
- [ ] **Refactor `EventList` Data Model:**
    - Create a persistent `std::vector<EventRow>` member in `EventList`.
    - `EventRow` struct should contain:
        - `Track* track`
        - `MidiItem* item`
        - `MidiEvent* event` (pointer to the actual event in the item)
        - `int eventIndex`
        - `uint64_t absoluteTick`
    - Implement `rebuildRows()` which populates this vector based on the current Song and filters.
- [ ] **Implement Visual Cursor:**
    - Add `int cursorRow_` and `int cursorCol_` state variables.
    - Update `draw()` to render a selection rectangle around the cell at `(cursorRow_, cursorCol_)`.
    - Columns: Timestamp, Status, Data1, Data2, Duration.

## Phase 2: Navigation & Focus
**Objective:** Allow the user to move the cursor and ensure the view scrolls to keep it visible.
- [ ] **Keyboard Handling (`handle()`):**
    - `FL_Focus`: Accept focus.
    - `FL_KeyDown`:
        - `Up/Down`: Increment/Decrement `cursorRow_`.
        - `Left/Right`: Increment/Decrement `cursorCol_`.
        - `PageUp/PageDown`: Jump by view height.
        - `Home/End`: Jump to start/end of list.
- [ ] **Auto-Scroll:**
    - Calculate if the new cursor position is out of view.
    - If so, adjust the parent `Fl_Scroll` position (requires callback or access to parent).

## Phase 3: In-Place Editing (The "Input Overlay")
**Objective:** Allow modifying cell values using a temporary input widget.
- [ ] **Edit Mechanics:**
    - `Enter` key triggers specific logic based on `cursorCol_`.
    - **Numeric Columns (Data1, Data2, Duration):**
        - Spawn `Fl_Int_Input` exactly over the cell.
        - On `FL_Enter` or loss of focus: Parse value, update `MidiEvent`, remove input, `redraw()`.
    - **Enum Columns (Status):**
        - Cycle through Status types or spawn `Fl_Choice`.
    - **Timestamp Column:**
        - Complex. Maybe start with editing discrete tick values? Or Measure/Beat/Tick fields individually? (Let's stick to raw tick or simple text parsing for V1).

## Phase 4: Data Write-Back & Commit
**Objective:** ensure edits propagate back to the Engine.
- [ ] **Update Logic:**
    - Modifying an event's **Time** requires re-sorting the Item's event list.
    - Modifying other fields is in-place.
- [ ] **Integration:**
    - Call `Sequencer::buildPlaybackQueue()` (or similar signal) to refresh the audio engine after edits.

## Phase 5: Quality of Life
- [ ] **Mouse Support:** Clicking a cell moves the cursor there.
- [ ] **Insert/Delete:** `Insert` key adds a default note. `Delete` key removes the row.

---

**Next Immediate Step:** Phase 1 (Refactor Data Model & Draw Cursor).

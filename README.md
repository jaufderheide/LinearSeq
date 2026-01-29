# Project Specification: LinearSeq

**Version:** 0.1 (Draft)
**Date:** 2024-05-21
**Target Platform:** Alpine Linux (x86_64 / ARMv7 / ARM64)
**License:** MIT (Proposed)

## 1. Overview
**LinearSeq** is a barebones, high-performance MIDI sequencer designed for a "keyboard, mouse, and monitor" workflow on minimal Linux distributions. It rejects modern loop-based/pattern-based paradigms (MPC/Ableton) in favor of a strictly **linear timeline workflow** inspired by early Cakewalk (DOS/Win) and Roland MC hardware sequencers.

The project prioritizes timing stability, low resource usage, and "spreadsheet-style" precision editing over graphical flourish.

## 2. Target Environment
* **Operating System:** Alpine Linux (Musl libc based).
* **Hardware:** Low-spec compatible (Raspberry Pi 3+ or generic x86).
* **Peripherals:** Full QWERTY keyboard (numpad heavily utilized), Mouse, HDMI Monitor.
* **Driver Layer:** ALSA (Advanced Linux Sound Architecture) - Raw MIDI. No JACK/PipeWire requirement for Version 1.0.

## 3. Core Architecture
* **Language:** C++17 (Balance of modern features and low overhead).
* **GUI Toolkit:** **FLTK (Fast Light Toolkit)**.
    * *Reasoning:* Statically linkable, extremely low RAM footprint, native-looking on bare X11, widgets are optimized for data density (perfect for Event Lists).
* **Timing Engine:** High-resolution internal clock (PPQN adjustable: 96, 192, 480) driven by POSIX high-resolution timers, directly addressing ALSA ports.

## 4. Functional Requirements

### 4.1. The "Track View" (Main Arrangement)
A linear representation of time moving left-to-right.
* **Tracks:** Vertical list of tracks. Each track is assigned a specific MIDI Port and Channel.
* **MIDI Items:** Data is encapsulated in "Items" (clips) that exist on the timeline.
    * Items are **not** loops by default; they are containers for MIDI events.
    * Items can be moved, cut, and pasted.
* **Transport:** Standard linear transport (Play, Stop, Pause, Rewind, Fast Forward, Record).

### 4.2. The "Event List" (Editor)
The primary method for editing MIDI data within an Item.
* **Visual Style:** Spreadsheet / Grid.
* **Columns:**
    * `Measure:Beat:Tick` (Timestamp)
    * `Status` (Note On, CC, Pitch Bend, Prog Change)
    * `Data 1` (Note Name/Number)
    * `Data 2` (Velocity/Value)
    * `Duration` (Ticks)
* **Interaction:** Keyboard-centric data entry (Arrow keys to navigate cells, Numpad to enter values, Enter to commit).

### 4.3. I/O & Hardware
* **Plug-and-Play:** Application scans ALSA `seq` client list on startup and hot-plug events.
* **Routing:** Simple 1:1 mapping of Track -> ALSA Output Port.

## 5. Roadmap

* **Phase 1: The Engine (Headless)**
    * Establish C++ build system on Alpine.
    * ALSA Sequencer backend (open port, send MIDI note).
    * Internal Clock implementation (stable BPM).
* **Phase 2: The Data Structure**
    * Implement `Track`, `MidiEvent`, and `MidiItem` classes.
    * Import/Export Standard MIDI File (SMF Type 1).
* **Phase 3: The View (GUI)**
    * FLTK Window setup.
    * Render the Event List (Grid).
    * Render the Track Timeline.
* **Phase 4: Interaction**
    * Connect GUI events to the Engine.
    * Implement Recording (ALSA Input -> Memory).

---

# Project Skeleton (Directory Structure)

This structure separates the low-level system logic (ALSA/Timing) from the UI code, allowing for easier testing.

```text
linear-seq/
├── CMakeLists.txt          # Main build configuration
├── README.md               # This specification
├── LICENSE
├── docs/                   # Documentation and Assets
│   └── DEV_SETUP.md
├── src/
│   ├── main.cpp            # Entry point (initializes UI and Engine)
│   ├── core/               # Non-GUI logic (The "Brain")
│   │   ├── Clock.cpp       # High-res timer logic
│   │   ├── Clock.h
│   │   ├── Sequencer.cpp   # Main playback engine
│   │   ├── Sequencer.h
│   │   └── Types.h         # Structs: MidiEvent, Track, Song
│   ├── audio/              # Hardware Abstraction Layer
│   │   ├── AlsaDriver.cpp  # Wrapper for libasound
│   │   └── AlsaDriver.h
│   ├── ui/                 # FLTK User Interface
│   │   ├── MainWindow.cpp  # Main application window
│   │   ├── MainWindow.h
│   │   ├── TrackView.cpp   # Custom widget: The Timeline
│   │   ├── TrackView.h
│   │   ├── EventList.cpp   # Custom widget: The Spreadsheet
│   │   └── EventList.h
│   └── utils/
│       └── SmfParser.cpp   # Standard MIDI File handling
└── tests/                  # Unit tests
    ├── test_clock.cpp
    └── test_alsa.cpp
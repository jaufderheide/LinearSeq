#include "ui/MainWindow.h"

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "core/Types.h"
#include "ui/AppIcon.h"
#include "ui/EventList.h"
#include "ui/MainToolbar.h"
#include "ui/TrackView.h"
#include "utils/SongJson.h"

namespace fs = std::filesystem;

namespace linearseq {

namespace {

Song makeDemoSong() {
	Song song;
	song.ppqn = DEFAULT_PPQN;
	song.bpm = DEFAULT_BPM;

	Track track;
	track.name = "Track 1";
	track.channel = 0;

	// MidiItem item;
	// item.startTick = 0;
	// item.lengthTicks = song.ppqn * 4 * 2;

	// MidiEvent event1;
	// event1.tick = 0;
	// event1.status = MidiStatus::NoteOn;
	// event1.channel = 0;
	// event1.data1 = 60;
	// event1.data2 = 100;
	// event1.duration = song.ppqn;
	// item.events.push_back(event1);

	// MidiEvent event2 = event1;
	// event2.tick = song.ppqn;
	// event2.data1 = 64;
	// item.events.push_back(event2);

	// MidiEvent event3 = event1;
	// event3.tick = song.ppqn * 2;
	// event3.data1 = 67;
	// item.events.push_back(event3);

	// MidiEvent event4 = event1;
	// event4.tick = song.ppqn * 3;
	// event4.data1 = 72;
	// item.events.push_back(event4);

	// track.items.push_back(item);
	song.tracks.push_back(track);

	return song;
}

} // namespace

// Static member for global handler
MainWindow* MainWindow::instanceForHandler_ = nullptr;

// Global event handler for application-level shortcuts
// This intercepts events BEFORE they reach widgets, ensuring consistent shortcut handling
int MainWindow::globalEventHandler(int event) {
	if (!instanceForHandler_) return 0;
	
	// Handle keyboard events (shortcuts, keydown, and keyboard events from focused widgets)
	if (event != FL_SHORTCUT && event != FL_KEYDOWN && event != FL_KEYBOARD) return 0;
	
	int key = Fl::event_key();
	
	// Check for Ctrl key modifier
	if (!(Fl::event_state() & FL_CTRL)) return 0;
	
	// Handle Ctrl+C (Copy)
	if (key == 'c') {
		instanceForHandler_->onCopy();
		return 1; // Event handled
	}
	
	// Handle Ctrl+V (Paste)
	if (key == 'v') {
		instanceForHandler_->onPaste();
		return 1; // Event handled
	}
	
	return 0; // Event not handled
}

MainWindow::MainWindow(int w, int h, const char* title)
	: Fl_Window(w, h, title),
      toolbar_(nullptr),
	  trackScroll_(nullptr),
	  trackView_(nullptr),
	  eventList_(nullptr),
	  activeItemIndex_(-1),
	  initialScrollSet_(false),
	  pendingScrollFixes_(0) {
	begin();
	auto* appIcon = linearseq_app_icon();
	Fl_Window::default_icon(appIcon);
	icon(appIcon);

	const int toolbarHeight = 32;
	const int splitter = h / 2;

    // Register Font Awesome (Globally used)
    Fl::set_font(FL_FREE_FONT, "FontAwesome"); //lubunto 24
	// Fl::set_font(FL_FREE_FONT, "Font Awesome 6 Free"); //Alpine WSL
	

    // Toolbar
    toolbar_ = new MainToolbar(0, 0, w, toolbarHeight);
    toolbar_->setOnPlay([this] { onPlay(); });
    toolbar_->setOnStop([this] { onStop(); });
    toolbar_->setOnRewind([this] { onRewind(); });
    toolbar_->setOnRecord([this] { onRecord(); });
    toolbar_->setOnAddTrack([this] { onAddTrack(); });
    toolbar_->setOnDeleteTrack([this] { onDeleteTrack(); });
    toolbar_->setOnAddItem([this] { onAddItem(); });
    toolbar_->setOnFileSave([this] { onFileSave(); });
    toolbar_->setOnFileLoad([this] { onFileLoad(); });
    toolbar_->setOnMidiOutSelect([this](int idx) { onMidiOutSelect(idx); });
    toolbar_->setOnBpmChanged([this](double bpm) { onBpmChanged(bpm); });
    toolbar_->setOnPpqnChanged([this](int ppqn) { onPpqnChanged(ppqn); });
    toolbar_->setOnTrackNameChanged([this](std::string name) { onTrackNameChanged(name); });
    
	Fl::set_font(FL_HELVETICA, "Helvetica");

	trackScroll_ = new Fl_Scroll(0, toolbarHeight, w, splitter - toolbarHeight);
	trackScroll_->type(Fl_Scroll::BOTH);
	trackScroll_->begin();
	trackView_ = new TrackView(0, toolbarHeight, w, splitter - toolbarHeight);
	trackScroll_->end();

    // Event List is now self-contained with its own scroll
	eventList_ = new EventList(0, splitter, w, h - splitter);
  
	song_ = makeDemoSong();
	sequencer_.setSong(song_);
	trackView_->setSong(song_);
	eventList_->setSong(song_);
    
	eventList_->setOnSongChanged([this](const Song& song) {
		song_ = song;
		sequencer_.setSong(song_);
	});
	trackView_->setChannelChanged([this](int index, int channel) {
		if (index < 0 || index >= static_cast<int>(song_.tracks.size())) {
			return;
		}
		song_.tracks[index].channel = static_cast<uint8_t>(channel - 1);
		sequencer_.setSong(song_);
	});

	trackView_->setSelectionChanged([this](int index) {
		eventList_->setTrackFilter(index);
		eventList_->setItemFilter(-1);
		activeItemIndex_ = -1;
		sequencer_.setActiveTrack(index);
		if (index >= 0 && index < static_cast<int>(song_.tracks.size())) {
			toolbar_->setTrackName(song_.tracks[index].name);
		}
	});

	trackView_->setItemSelectionChanged([this](int trackIndex, std::set<int> itemIndices) {
		if (trackIndex == trackView_->selectedTrack()) {
            // For EventList, we probably only want to show one item's events or merge them.
            // For now, if multiple are selected, show none or the "first" found?
            // Simple approach: if set size is 1, show it. Else show nothing (-1).
            if (itemIndices.size() == 1) {
			    activeItemIndex_ = *itemIndices.begin();
			    eventList_->setItemFilter(activeItemIndex_);
            } else {
                activeItemIndex_ = -1;
                eventList_->setItemFilter(-1);
            }
		}
	});

    trackView_->setItemsMoved([this](int trackIdx, const std::vector<std::pair<int, uint32_t>>& updates) {
         if (trackIdx >= 0 && trackIdx < static_cast<int>(song_.tracks.size())) {
            auto& track = song_.tracks[trackIdx];
            bool anyChanged = false;
            
            for (const auto& update : updates) {
                int itemIdx = update.first;
                uint32_t newTick = update.second;
                if (itemIdx >= 0 && itemIdx < static_cast<int>(track.items.size())) {
                     track.items[itemIdx].startTick = newTick;
                     anyChanged = true;
                }
            }
            
            if (anyChanged) {
                sequencer_.setSong(song_);
                // Update specific track in view or full refresh
                trackView_->setSong(song_); 
                // Also update event list if it's showing this item
                // Simplification for now: re-set filter if active item was implicated?
                // Just refreshing song on TrackView handles visuals.
                // EventList might need refresh if active item moved?
                eventList_->setSong(song_);
            }
         }
    });

    trackView_->setSetTime([this](uint32_t tick) {
        currentTick_ = tick;
        trackView_->setPlayheadTick(currentTick_);
    });

	trackView_->setSelectedTrack(0);
	eventList_->setTrackFilter(0);
	eventList_->setItemFilter(-1);
	sequencer_.setActiveTrack(0);
	toolbar_->setTrackName(song_.tracks.empty() ? "Track 1" : song_.tracks[0].name.c_str());
	updateScrollContent();

	ensureDriverOpen();
	sequencer_.setDriver(&driver_);
	refreshMidiDevices();
	updateStatus();

	resizable(eventList_);
    
    refreshViews();
	end();
	
	// Register global event handler for application-level shortcuts
	instanceForHandler_ = this;
	Fl::add_handler(globalEventHandler);
}

MainWindow::~MainWindow() {
	// Unregister global handler
	Fl::remove_handler(globalEventHandler);
	instanceForHandler_ = nullptr;
}

void MainWindow::show(int argc, char** argv) {
	Fl_Window::show(argc, argv);
	pendingScrollFixes_ = 2;
	Fl::add_timeout(0.01, &MainWindow::postInitScroll, this);
}

void MainWindow::ensureDriverOpen() {
	if (!driver_.isOpen()) {
		driver_.open();
	}
}

void MainWindow::refreshMidiDevices() {
	if (!driver_.isOpen()) {
		if (toolbar_) toolbar_->clearMidiPorts();
		return;
	}
	availablePorts_ = driver_.listOutputPorts();
	if (toolbar_) {
		toolbar_->clearMidiPorts();
		toolbar_->addMidiPort("Info: MIDI Out"); // Header/Placeholder
		for (const auto& port : availablePorts_) {
			toolbar_->addMidiPort(port.name.c_str());
		}
		if (!availablePorts_.empty()) {
			toolbar_->setMidiPortSelection(0); // Select first real one? Or kept at 0 (header)
			// Let's actually not select header if possible, or make it active.
			// Actually 0 is "Info: MIDI Out". 
			if (availablePorts_.size() > 0) toolbar_->setMidiPortSelection(1);
			// Auto connect first? 
			// Let's do nothing on refresh, wait for user interact.
			if (availablePorts_.size() > 0) {
				driver_.connectOutput(availablePorts_[0].client, availablePorts_[0].port);
			}
		} else {
			toolbar_->setMidiPortSelection(0);
		}
	}
}

void MainWindow::onMidiOutSelect(int idx) {
	if (idx <= 0) return; // Header or none
	if (idx - 1 < static_cast<int>(availablePorts_.size())) {
		const auto& port = availablePorts_[idx - 1];
		driver_.connectOutput(port.client, port.port);
		toolbar_->setStatus("Connected: " + port.name);
	}
}

void MainWindow::updateStatus() {
	if (driver_.isOpen()) {
		toolbar_->setStatus("ALSA: ready");
	} else {
		toolbar_->setStatus("ALSA: unavailable");
	}
}

void MainWindow::refreshViews() {
	song_ = sequencer_.song();
	trackView_->setSong(song_);
	eventList_->setSong(song_);
	toolbar_->setBpm(song_.bpm);	
	toolbar_->setPpqn(static_cast<int>(song_.ppqn));
	
	eventList_->setTrackFilter(trackView_->selectedTrack());
	eventList_->setItemFilter(activeItemIndex_);
    if (activeItemIndex_ >= 0) {
	    trackView_->setSelectedItems({activeItemIndex_});
    } else {
        trackView_->setSelectedItems({});
    }
	updateScrollContent();
}

void MainWindow::onBpmChanged(double bpm) {
	song_.bpm = bpm;
	sequencer_.setSong(song_);
}

void MainWindow::onPpqnChanged(int ppqn) {
	if (song_.ppqn != static_cast<uint32_t>(ppqn)) {
        song_.ppqn = static_cast<uint32_t>(ppqn);
        sequencer_.setSong(song_);
        refreshViews();
	}
}

int MainWindow::handle(int event) {
    // Reset tracking when Ctrl is released
    if (event == FL_KEYUP && !(Fl::event_state() & FL_CTRL)) {
        lastHandledShortcutKey_ = 0;
    }
    
    // Handle shortcuts when Ctrl is held, regardless of event type
    // But exclude FL_KEYUP (9) to avoid duplicate handling
    if ((Fl::event_state() & FL_CTRL) && event != FL_KEYUP) {
        int key = Fl::event_key();
        
        // Only handle each key once per press (prevent duplicate events)
        if (key == lastHandledShortcutKey_) {
            return 0; // Already handled this key
        }
        
        // Copy/Paste shortcuts - handle these at window level
        if (key == 'c') {
            lastHandledShortcutKey_ = key;
            onCopy();
            return 1;
        } else if (key == 'v') {
            lastHandledShortcutKey_ = key;
            onPaste();
            return 1;
        }
    }
    
    // Delete key - handle separately (doesn't require Ctrl)
    if ((event == FL_KEYDOWN || event == FL_KEYBOARD || event == FL_SHORTCUT) && event != FL_KEYUP) {
        int key = Fl::event_key();
        if (key == FL_Delete || key == FL_BackSpace) {
            // Only delete if focus is within TrackView or EventList
            // Avoids intercepting Backspace when typing in inputs (e.g. BPM)
            Fl_Widget* focus = Fl::focus();
            bool shouldDelete = false;
            Fl_Widget* w = focus;
            while (w) {
                if (w == trackView_ || w == eventList_) {
                   shouldDelete = true;
                   break;
                }
                w = w->parent();
            }

            if (shouldDelete) {
                onDelete();
                return 1;
            }
        }
    }
    
    // Let children handle other events
    return Fl_Window::handle(event);
}

void MainWindow::onCopy() {
    clipboardItems_.clear();
    int trackIdx = trackView_->selectedTrack();
    if (trackIdx < 0 || trackIdx >= static_cast<int>(song_.tracks.size())) return;
    
    auto& track = song_.tracks[trackIdx];
    std::set<int> selected = trackView_->selectedItems();
    if (selected.empty()) return;

    // Find min tick to normalize
    uint32_t minTick = UINT32_MAX;
    for (int idx : selected) {
        if (idx >= 0 && idx < static_cast<int>(track.items.size())) {
            if (track.items[idx].startTick < minTick) minTick = track.items[idx].startTick;
        }
    }
    
    for (int idx : selected) {
         if (idx >= 0 && idx < static_cast<int>(track.items.size())) {
             MidiItem item = track.items[idx];
             item.startTick -= minTick; // Normalize to 0 relative to first item
             clipboardItems_.push_back(item);
         }
    }
}

void MainWindow::onPaste() {
    if (clipboardItems_.empty()) return;

    // Use selected track or 0 if none
    int trackIdx = trackView_->selectedTrack();
    if (trackIdx < 0) trackIdx = 0;
    if (trackIdx >= static_cast<int>(song_.tracks.size())) return;
    
    auto& track = song_.tracks[trackIdx];
    
    // Paste at currentTick_
    std::set<int> newSelection;
    
    for (const auto& clipItem : clipboardItems_) {
        MidiItem newItem = clipItem;
        newItem.startTick += currentTick_; 
        
        track.items.push_back(newItem);
        newSelection.insert(static_cast<int>(track.items.size() - 1));
    }
    
    sequencer_.setSong(song_);
    refreshViews();
    trackView_->setSelectedTrack(trackIdx);
    trackView_->setSelectedItems(newSelection);
}

void MainWindow::onDelete() {
    int trackIdx = trackView_->selectedTrack();
    if (trackIdx < 0 || trackIdx >= static_cast<int>(song_.tracks.size())) return;

    auto& track = song_.tracks[trackIdx];
    std::set<int> selected = trackView_->selectedItems();
    if (selected.empty()) return;
    
    // Remove in reverse order to preserve indices of earlier items
    for (auto it = selected.rbegin(); it != selected.rend(); ++it) {
        int idx = *it;
        if (idx >= 0 && idx < static_cast<int>(track.items.size())) {
            track.items.erase(track.items.begin() + idx);
        }
    }
    
    // Clear selection
    trackView_->setSelectedItems({});
    activeItemIndex_ = -1;
    eventList_->setItemFilter(-1);
    
    sequencer_.setSong(song_);
    refreshViews();
}

void MainWindow::onPlay() {
	ensureDriverOpen();
	updateStatus();
	// Start playback from current playhead position
	sequencer_.play(currentTick_);
    Fl::add_timeout(0.033, playTimer, this);
}

void MainWindow::onStop() {
    Fl::remove_timeout(playTimer, this);
	sequencer_.stopRecording();
	sequencer_.stop();
	toolbar_->setRecording(false);
	updateStatus();
	refreshViews();
}

void MainWindow::onRewind() {
	// Reset playhead to time 0
	currentTick_ = 0;
	trackView_->setPlayheadTick(0);
	// Scroll timeline back to show time 0
	if (trackScroll_) {
		trackScroll_->scroll_to(0, trackScroll_->yposition());
	}
	redraw();
}

void MainWindow::playTimer(void* data) {
    MainWindow* mw = static_cast<MainWindow*>(data);
    if (!mw->sequencer_.isPlaying()) return;
    
    // Check if sequencer requested auto-stop (end of sequence)
    if (mw->sequencer_.shouldStop()) {
        mw->onStop();
        return;
    }
    
    mw->currentTick_ = static_cast<uint32_t>(mw->sequencer_.currentTick());
    mw->trackView_->setPlayheadTick(mw->currentTick_);
    Fl::repeat_timeout(0.033, playTimer, data);
}

void MainWindow::onRecord() {
	ensureDriverOpen();
	updateStatus();
	if (!driver_.isOpen()) {
		return;
	}
	if (sequencer_.isRecording()) {
		sequencer_.stopRecording();
		toolbar_->setRecording(false);
		refreshViews();
		return;
	}

	sequencer_.startRecording();
	if (sequencer_.isRecording()) {
		toolbar_->setRecording(true);
	}
}

void MainWindow::onAddTrack() {
	Track track;
	track.name = "Track " + std::to_string(song_.tracks.size() + 1);
	track.channel = 0;
	song_.tracks.push_back(track);

	const int newIndex = static_cast<int>(song_.tracks.size() - 1);
	
	toolbar_->setTrackName(track.name);
	activeItemIndex_ = -1;
	sequencer_.setActiveTrack(newIndex);
	sequencer_.setSong(song_);

    refreshViews();

    // Redundant call removed. refreshViews() -> trackView_->setSong() will
    // now correctly update and layout rows without needing a second nudge.
    
    // Explicitly set selection because refreshViews sets it to old selection?
    // Actually refreshViews uses trackView_->selectedTrack() to filter event list.
    // So we should set the selection in the View.
    trackView_->setSelectedTrack(newIndex);
    // And refresh again to propagate filter? Or just call callbacks?
    // Let's just manually trigger what we need.
    eventList_->setTrackFilter(newIndex);
    eventList_->setItemFilter(-1);
    
}

void MainWindow::onDeleteTrack() {
    int idx = trackView_->selectedTrack();
    if (idx < 0 || idx >= static_cast<int>(song_.tracks.size())) {
        return;
    }

    song_.tracks.erase(song_.tracks.begin() + idx);

    // Determine new selection logic
    int newSelection = -1;
    if (!song_.tracks.empty()) {
        if (idx < static_cast<int>(song_.tracks.size())) {
            newSelection = idx; 
        } else {
            newSelection = static_cast<int>(song_.tracks.size()) - 1;
        }
    }

    activeItemIndex_ = -1;
    sequencer_.setSong(song_);
    sequencer_.setActiveTrack(newSelection >= 0 ? newSelection : 0);
    
    // Update UI selection
    trackView_->setSong(song_); // Update song first so size is correct
    trackView_->setSelectedTrack(newSelection);
    
    refreshViews();
    
    // Update toolbar name
    if (newSelection >= 0) {
        toolbar_->setTrackName(song_.tracks[newSelection].name);
    } else {
        toolbar_->setTrackName("No Tracks");
    }
}

void MainWindow::updateScrollContent() {
	const int trackHeight = trackView_->contentHeight();
	const int trackWidth = std::max(trackScroll_->w(), trackView_->contentWidth());
	trackView_->size(trackWidth, trackHeight);

	normalizeScrollPositions();
	trackScroll_->redraw();
    // eventScroll_ removed
}

void MainWindow::normalizeScrollPositions() {
	const int trackTotal = trackView_->h();
	const int trackPage = trackScroll_->h();
	trackScroll_->scrollbar.value(0, trackPage, 0, trackTotal);
	trackScroll_->scroll_to(0, 0);
    
    // eventScroll_ logic removed
}

void MainWindow::postInitScroll(void* data) {
	auto* window = static_cast<MainWindow*>(data);
	if (!window) {
		return;
	}
	if (!window->shown()) {
		Fl::add_timeout(0.01, &MainWindow::postInitScroll, window);
		return;
	}
	window->updateScrollContent();
	window->normalizeScrollPositions();
	if (window->pendingScrollFixes_ > 0) {
		window->pendingScrollFixes_ -= 1;
		Fl::repeat_timeout(0.05, &MainWindow::postInitScroll, window);
		return;
	}
	window->initialScrollSet_ = true;
}

void MainWindow::ensureActiveTrack() {
	if (song_.tracks.empty()) {
		Track track;
		track.name = "Track 1";
		track.channel = 0;
		song_.tracks.push_back(track);
		trackView_->setSelectedTrack(0);
		sequencer_.setActiveTrack(0);
	}
}

void MainWindow::onAddItem() {
	ensureActiveTrack();
	const int trackIndex = trackView_->selectedTrack() >= 0 ? trackView_->selectedTrack() : 0;
	auto& items = song_.tracks[trackIndex].items;

	uint64_t maxEnd = 0;
	for (const auto& item : items) {
		maxEnd = std::max(maxEnd, static_cast<uint64_t>(item.startTick + item.lengthTicks));
	}

	MidiItem item;
	item.startTick = static_cast<uint32_t>(maxEnd );//+ song_.ppqn);
	item.lengthTicks = song_.ppqn * 4;// 1 measure
	items.push_back(item);

	activeItemIndex_ = static_cast<int>(items.size() - 1);
	sequencer_.setSong(song_);
	refreshViews();
	
	// Give focus to TrackView so Delete key works immediately
	trackView_->take_focus();
}

void MainWindow::onTrackNameChanged(std::string name) {
	const int trackIndex = trackView_->selectedTrack();
	if (trackIndex < 0 || trackIndex >= static_cast<int>(song_.tracks.size())) {
		return;
	}
	song_.tracks[trackIndex].name = name;
	sequencer_.setSong(song_);
	refreshViews();
}


void MainWindow::onFileSave() {
	if (toolbar_->getMidiPortSelection() >= 1) {
		const int idx = toolbar_->getMidiPortSelection() - 1; // 0 is Header
		if (idx < static_cast<int>(availablePorts_.size())) {
			song_.midiDevice = availablePorts_[idx].name;
		}
	} else {
		song_.midiDevice.clear();
	}
	// Sync song to sequencer (though they are pointers/copies, let's just use local song_)

	Fl_Native_File_Chooser chooser;
	chooser.title("Save LinearSeq JSON");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("LinearSeq JSON\t*.lseq");
	chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	if (chooser.show() != 0) {
		return;
	}
	const char* pathStr = chooser.filename();
	if (!pathStr) {
		return;
	}

    fs::path path{pathStr};
    if (path.extension() != ".lseq") {
        path += ".lseq";
        pathStr = path.string().c_str();
    }

	SongJson::saveToFile(song_, pathStr);
}

void MainWindow::onFileLoad() {
	Fl_Native_File_Chooser chooser;
	chooser.title("Open LinearSeq JSON");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("LinearSeq JSON\t*.lseq");
	if (chooser.show() != 0) {
		return;
	}
	const char* path = chooser.filename();
	if (!path) {
		return;
	}
	Song loaded;
	if (!SongJson::loadFromFile(path, loaded)) {
		return;
	}
	song_ = std::move(loaded);
	sequencer_.setSong(song_);
	activeItemIndex_ = -1;

	// Restore MIDI Device selection
	if (!song_.midiDevice.empty()) {
		refreshMidiDevices(); // Ensure list is up to date
		for (size_t i = 0; i < availablePorts_.size(); ++i) {
			if (availablePorts_[i].name == song_.midiDevice) {
				if (toolbar_) {
                    toolbar_->setMidiPortSelection(static_cast<int>(i) + 1);
                    toolbar_->setStatus("Connected: " + song_.midiDevice);
                }
				driver_.connectOutput(availablePorts_[i].client, availablePorts_[i].port);
				break;
			}
		}
	}

	const int trackIndex = song_.tracks.empty() ? -1 : 0;
	trackView_->setSong(song_);
	trackView_->setSelectedTrack(trackIndex);
	eventList_->setSong(song_);
	eventList_->setTrackFilter(trackIndex);
	eventList_->setItemFilter(-1);
	sequencer_.setActiveTrack(trackIndex < 0 ? 0 : trackIndex);
	toolbar_->setTrackName(song_.tracks.empty() ? "Track 1" : song_.tracks[0].name.c_str());
	refreshViews();
}

} // namespace linearseq

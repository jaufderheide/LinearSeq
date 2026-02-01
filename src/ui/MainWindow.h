#pragma once

#include <FL/Fl_Box.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Window.H>

#include <vector>

#include "audio/AlsaDriver.h"
#include "core/Sequencer.h"

namespace linearseq {

class EventList;
class TrackView;
class MainToolbar;

class MainWindow : public Fl_Window {
public:
	MainWindow(int w, int h, const char* title);
	~MainWindow() override;    
	void show(int argc, char** argv);

private:
	void onPlay();
	void onStop();
	void onRewind();
	void onRecord();
	void onAddTrack();
	void onAddItem();
	void onTrackNameChanged(std::string name);
	void onTrackPitchShift(); // Shift pitch of selected track by semitones
	void onFileSave();
	void onFileLoad();
	void onMidiOutSelect(int index);
	void onBpmChanged(double bpm);
	void onPpqnChanged(int ppqn);
	void ensureDriverOpen();
	void updateStatus();
    void refreshViews();
	void ensureActiveTrack();
	void updateScrollContent();
	void normalizeScrollPositions();
	static void postInitScroll(void* data);
    static void playTimer(void* data);
	void updateChannelInputs();
	void updateWindowTitle();
	void setModified(bool modified);
	bool isModified() const { return modified_; }
	void onClose();
	static void onChannelInput(Fl_Widget* widget, void* data);
	void refreshMidiDevices();
	
	static int globalEventHandler(int event);
	static MainWindow* instanceForHandler_;

	MainToolbar* toolbar_;
    Fl_Box* statusBar_;
    Fl_Box* tickDisplay_;
    Fl_Box* connectionStatus_;
    
	Fl_Scroll* trackScroll_;
	TrackView* trackView_;
	EventList* eventList_;

	std::vector<Fl_Int_Input*> channelInputs_;
	int activeItemIndex_;
	bool initialScrollSet_;
	int pendingScrollFixes_;
	Song song_;
	Sequencer sequencer_;
	AlsaDriver driver_;
	std::vector<AlsaDriver::PortInfo> availablePorts_;
    
    // Clipboard interaction
    std::vector<MidiItem> clipboardItems_;
    
    // Playback state
    uint32_t currentTick_ = 0;
    
    // File state
    std::string currentFilename_;
    bool modified_ = false;
    
    // Shortcut handling state (to prevent duplicate handling)
    int lastHandledShortcutKey_ = 0;
    
    int handle(int event) override;
    
    void onCopy();
    void onPaste();
    void onDelete();
    void onDeleteTrack();
};

} // namespace linearseq

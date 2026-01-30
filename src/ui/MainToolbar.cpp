#include "ui/MainToolbar.h"
#include "ui/LseqMenuButton.h"
#include "core/Types.h" // For defaults if needed

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cstdio> // for debug prints if needed

// Ensure FL_FREE_FONT is defined if not picked up
#ifndef FL_FREE_FONT
#define FL_FREE_FONT FL_FREE_FONT
#endif

namespace linearseq {

MainToolbar::MainToolbar(int x, int y, int w, int h) : Fl_Group(x, y, w, h) {
    // Background box?
    box(FL_FLAT_BOX);
    color(FL_LIGHT1); // Slightly different background than window?
    
    int toolX = x;

    // Font Awesome assumed to be loaded by MainWindow or Main
    
	fileMenuButton_ = new LseqMenuButton(toolX, y + 4, 12, 24, "\uf142");
	fileMenuButton_->labelfont(FL_FREE_FONT);
	fileMenuButton_->box(FL_FLAT_BOX);
    fileMenuButton_->down_box(FL_FLAT_BOX);
	fileMenuButton_->color(FL_LIGHT2);
	fileMenuButton_->labelsize(16);
	fileMenuButton_->add("Save", 0, [](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
		if (self->onFileSave_) self->onFileSave_();
	}, this);
	fileMenuButton_->add("Open", 0, [](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
		if (self->onFileLoad_) self->onFileLoad_();
	}, this);

	playButton_ = new Fl_Button(toolX += 20, y + 4, 24, 24, "\uf04b");
    playButton_->labelfont(FL_FREE_FONT);
    playButton_->labelsize(14);
    playButton_->tooltip("Play");
    playButton_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onPlay_) self->onPlay_();
    }, this);

	stopButton_ = new Fl_Button(toolX += 30, y + 4, 24, 24, "\uf04d");
    stopButton_->labelfont(FL_FREE_FONT);
    stopButton_->labelsize(14);
    stopButton_->tooltip("Stop");
    stopButton_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onStop_) self->onStop_();
    }, this);

	recordButton_ = new Fl_Button(toolX += 30, y + 4, 24, 24, "\uf111");
    recordButton_->labelfont(FL_FREE_FONT);
    recordButton_->labelsize(14);
    recordButton_->tooltip("Record");
    recordButton_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onRecord_) self->onRecord_();
    }, this);

    // Switching font for text buttons
    // Note: Fl_Button stores its own font, so valid scopes matters mostly for label measurement if not explicit.
    // LseqMenuButton and Icons use FL_FREE_FONT.
    
	addTrackButton_ = new Fl_Button(toolX += 34, y + 4, 60, 24, "+Track");
    addTrackButton_->tooltip("Add Track");
    addTrackButton_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onAddTrack_) self->onAddTrack_();
    }, this);

	deleteTrackButton_ = new Fl_Button(toolX += 68, y + 4, 60, 24, "-Track");
    deleteTrackButton_->tooltip("Delete Track");
    deleteTrackButton_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onDeleteTrack_) self->onDeleteTrack_();
    }, this);


	playButton_->box(FL_FLAT_BOX);
	stopButton_->box(FL_FLAT_BOX);
	recordButton_->box(FL_FLAT_BOX);
	addTrackButton_->box(FL_FLAT_BOX);
    deleteTrackButton_->box(FL_FLAT_BOX);

	playButton_->color(FL_LIGHT2);
	stopButton_->color(FL_LIGHT2);
	recordButton_->color(FL_LIGHT2);
	addTrackButton_->color(FL_LIGHT2);
    deleteTrackButton_->color(FL_LIGHT2);

    trackNameInput_ = new Fl_Input(toolX += 152, y + 4, 140, 24, "Track Name");
	trackNameInput_->value("Track 1");
	trackNameInput_->box(FL_FLAT_BOX);
    trackNameInput_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onTrackNameChanged_) self->onTrackNameChanged_(self->trackNameInput_->value());
    }, this);

    addItemButton_ = new Fl_Button(toolX += 148, y + 4, 60, 24, "+Item");
    addItemButton_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onAddItem_) self->onAddItem_();
    }, this);
    addItemButton_->box(FL_FLAT_BOX);
    addItemButton_->color(FL_LIGHT2);

	midiOutChoice_ = new Fl_Choice(toolX += 68, y + 4, 128, 24);
	midiOutChoice_->box(FL_FLAT_BOX);
	midiOutChoice_->down_box(FL_FLAT_BOX);
	midiOutChoice_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onMidiOutSelect_) self->onMidiOutSelect_(self->midiOutChoice_->value());
	}, this);

	bpmSpinner_ = new Fl_Spinner(toolX += 164, y + 4, 60, 24, "BPM");
	bpmSpinner_->range(20, 300);
	bpmSpinner_->value(120.0); // Default
	bpmSpinner_->step(1.0);
	bpmSpinner_->box(FL_FLAT_BOX);
	bpmSpinner_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        if (self->onBpmChanged_) self->onBpmChanged_(self->bpmSpinner_->value());
	}, this);
	bpmSpinner_->tooltip("BPM");

	ppqnInput_ = new Fl_Int_Input(toolX += 106, y + 4, 60, 24, "PPQN");
	ppqnInput_->box(FL_FLAT_BOX);
	ppqnInput_->value("96"); // Default
	ppqnInput_->callback([](Fl_Widget*, void* data) {
        auto* self = static_cast<MainToolbar*>(data);
        // Parse int
        int val = 96;
        try { val = std::stoi(self->ppqnInput_->value()); } catch(...) {}
        if (self->onPpqnChanged_) self->onPpqnChanged_(val);
	}, this);
	ppqnInput_->tooltip("PPQN");



    statusLabel_ = new Fl_Box(x + w - 160, y + 4, 150, 24, "Status");
    statusLabel_->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    statusLabel_->box(FL_FLAT_BOX); 
    statusLabel_->labelsize(12);
    
    end();
}

void MainToolbar::setOnPlay(std::function<void()> cb) { onPlay_ = std::move(cb); }
void MainToolbar::setOnStop(std::function<void()> cb) { onStop_ = std::move(cb); }
void MainToolbar::setOnRecord(std::function<void()> cb) { onRecord_ = std::move(cb); }
void MainToolbar::setOnAddTrack(std::function<void()> cb) { onAddTrack_ = std::move(cb); }
void MainToolbar::setOnDeleteTrack(std::function<void()> cb) { onDeleteTrack_ = std::move(cb); }
void MainToolbar::setOnAddItem(std::function<void()> cb) { onAddItem_ = std::move(cb); }
void MainToolbar::setOnFileSave(std::function<void()> cb) { onFileSave_ = std::move(cb); }
void MainToolbar::setOnFileLoad(std::function<void()> cb) { onFileLoad_ = std::move(cb); }
void MainToolbar::setOnMidiOutSelect(std::function<void(int)> cb) { onMidiOutSelect_ = std::move(cb); }
void MainToolbar::setOnBpmChanged(std::function<void(double)> cb) { onBpmChanged_ = std::move(cb); }
void MainToolbar::setOnPpqnChanged(std::function<void(int)> cb) { onPpqnChanged_ = std::move(cb); }
void MainToolbar::setOnTrackNameChanged(std::function<void(std::string)> cb) { onTrackNameChanged_ = std::move(cb); }

void MainToolbar::setBpm(double bpm) { bpmSpinner_->value(bpm); }
void MainToolbar::setPpqn(int ppqn) { ppqnInput_->value(std::to_string(ppqn).c_str()); }
void MainToolbar::setTrackName(const std::string& name) { trackNameInput_->value(name.c_str()); }
void MainToolbar::setStatus(const std::string& status) { statusLabel_->copy_label(status.c_str()); }

void MainToolbar::setRecording(bool recording) {
    if (recording) {
        recordButton_->label("\uf111"); // Active color change or icon change? 
        // Logic in MainWindow was: recordButton_->label("\uf111"); 
        // Wait, MainWindow made it red? Or just tooltip?
        // Ah, FLTK buttons usually toggle value() if type(FL_TOGGLE_BUTTON).
        // Here we just change color or label. MainWindow code might have had logic.
        // Let's assume passed in label or just color.
        recordButton_->labelcolor(FL_RED);
    } else {
        recordButton_->labelcolor(FL_BLACK);
    }
    recordButton_->redraw();
}

void MainToolbar::clearMidiPorts() {
    midiOutChoice_->clear();
}

void MainToolbar::addMidiPort(const std::string& name) {
    midiOutChoice_->add(name.c_str());
}

void MainToolbar::setMidiPortSelection(int index) {
    midiOutChoice_->value(index);
}

int MainToolbar::getMidiPortSelection() const {
    return midiOutChoice_->value();
}

double MainToolbar::getBpm() const {
    return bpmSpinner_->value();
}

int MainToolbar::getPpqn() const {
    try {
        return std::stoi(ppqnInput_->value());
    } catch (...) {
        return 96;
    }
}

std::string MainToolbar::getTrackName() const {
    return trackNameInput_->value();
}

} // namespace linearseq

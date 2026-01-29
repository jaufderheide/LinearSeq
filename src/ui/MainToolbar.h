#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Box.H>
#include <functional>
#include <string>
#include <vector>

namespace linearseq {

class LseqMenuButton;

class MainToolbar : public Fl_Group {
public:
    MainToolbar(int x, int y, int w, int h);
    
    // Callbacks
    void setOnPlay(std::function<void()> cb);
    void setOnStop(std::function<void()> cb);
    void setOnRecord(std::function<void()> cb);
    void setOnAddTrack(std::function<void()> cb);
    void setOnAddItem(std::function<void()> cb);
    void setOnDeleteTrack(std::function<void()> cb);
    void setOnFileSave(std::function<void()> cb);
    void setOnFileLoad(std::function<void()> cb);
    void setOnMidiOutSelect(std::function<void(int)> cb); // passes index
    void setOnBpmChanged(std::function<void(double)> cb);
    void setOnPpqnChanged(std::function<void(int)> cb);
    void setOnTrackNameChanged(std::function<void(std::string)> cb);

    // Update UI
    void setBpm(double bpm);
    void setPpqn(int ppqn);
    void setTrackName(const std::string& name);
    void setStatus(const std::string& status);
    void setRecording(bool recording);
    
    void clearMidiPorts();
    void addMidiPort(const std::string& name);
    void setMidiPortSelection(int index);
    int getMidiPortSelection() const;
    
    // Getters for values
    double getBpm() const;
    int getPpqn() const;
    std::string getTrackName() const;

private:
    LseqMenuButton* fileMenuButton_;
    Fl_Button* playButton_;
    Fl_Button* stopButton_;
    Fl_Button* recordButton_;
    Fl_Button* addTrackButton_;
    Fl_Button* deleteTrackButton_;
    Fl_Button* addItemButton_;
    Fl_Choice* midiOutChoice_;
    Fl_Spinner* bpmSpinner_;
    Fl_Int_Input* ppqnInput_;
    Fl_Input* trackNameInput_;
    Fl_Box* statusLabel_;

    std::function<void()> onPlay_;
    std::function<void()> onStop_;
    std::function<void()> onRecord_;
    std::function<void()> onAddTrack_;
    std::function<void()> onDeleteTrack_;
    std::function<void()> onAddItem_;
    std::function<void()> onFileSave_;
    std::function<void()> onFileLoad_;
    std::function<void(int)> onMidiOutSelect_;
    std::function<void(double)> onBpmChanged_;
    std::function<void(int)> onPpqnChanged_;
    std::function<void(std::string)> onTrackNameChanged_;
};

} // namespace linearseq

#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Scroll.H>
#include <vector>
#include <set>
#include <functional>

#include "core/Types.h"

namespace linearseq {

struct EventRow {
	uint64_t absTick = 0;
	int trackIndex = -1;
	int itemIndex = -1;
	int eventIndex = -1;
	MidiEvent* event = nullptr;
};

class EventRowsWidget; // Forward declaration

class EventList : public Fl_Group {
public:
	EventList(int x, int y, int w, int h);
    ~EventList(); // Need destructor possibly

	void setSong(const Song& song);
	void setTrackFilter(int trackIndex);
	void setItemFilter(int itemIndex);
	int contentHeight() const;

	void setOnSongChanged(std::function<void(const Song&)> cb);

    // Editing Commands
    void insertEvent();
    void insertMultipleEvents(int noteValue); // noteValue: 1=whole, 2=half, 4=quarter, 8=eighth, 16=sixteenth
    void deleteSelectedEvent();
    void copySelected();
    void pasteEvents();
    bool hasClipboardData() const { return !clipboardEvents_.empty(); }

	void draw() override;
	int handle(int event) override;
    void resize(int x, int y, int w, int h) override;

private:
	void rebuildRows();
	int getColumnX(int col) const;
	int getColumnWidth(int col) const;
	void ensureCursorVisible();
	void moveCursor(int dRow, int dCol); // New helper
	uint8_t getDefaultPitch() const; // Get last note from track or middle C

	void startEdit(const char* initialValue = nullptr);
	void stopEdit(bool save);
	static void editCallback(Fl_Widget* w, void* v);

	Song song_{};
	int trackFilter_ = -1;
	int itemFilter_ = -1;
	
	std::vector<EventRow> rows_;
	int cursorRow_ = 0;
	int cursorCol_ = 0;
	std::set<int> selectedRows_; // Multi-select support
	std::vector<MidiEvent> clipboardEvents_; // Copy/paste clipboard
    
    // UI Structure:
    // EventList (Fl_Group)
    //  |- insertButton_, deleteButton_ (Header elements)
    //  |- scroll_ (Fl_Scroll)
    //      |- rowsWidget_ (EventRowsWidget - draws rows)
    //  |- editInput_ (Overlay on rowsWidget? No, must be sibling of scroll or inside rowsWidget)
    //     Ideally editInput_ should be inside rowsWidget so complications with coordinates are handled by scroll.

    Fl_Scroll* scroll_ = nullptr;
    EventRowsWidget* rowsWidget_ = nullptr;

	Fl_Input* editInput_ = nullptr;
    Fl_Menu_Button* insertButton_ = nullptr;
    Fl_Button* deleteButton_ = nullptr;

	std::function<void(const Song&)> onSongChanged_;
    
    friend class EventRowsWidget;
};

} // namespace linearseq

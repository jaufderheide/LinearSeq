#include "ui/EventList.h"

#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <functional>

namespace linearseq {

// Helper to parse note name (e.g., "C3", "F#4") to MIDI note number (0-127)
static int parseNoteName(const char* s) {
    if (!s || !*s) return 0;
    
    // Check if it's just a number
    char* end;
    long val = std::strtol(s, &end, 10);
    if (end > s && *end == 0) {
        return std::max(0, std::min(127, (int)val));
    }

    std::string str = s;
    // Basic parsing: Note Letter, optionally Accidental, Octave
    // C-1 to G9
    
    // Find note letter
    int noteBase = -1;
    char noteChar = std::toupper(str[0]);
    switch (noteChar) {
        case 'C': noteBase = 0; break;
        case 'D': noteBase = 2; break;
        case 'E': noteBase = 4; break;
        case 'F': noteBase = 5; break;
        case 'G': noteBase = 7; break;
        case 'A': noteBase = 9; break;
        case 'B': noteBase = 11; break;
        default: return 0; // Invalid
    }

    int idx = 1;
    if (idx >= (int)str.length()) return 0;

    // Check accidental
    if (str[idx] == '#') {
        noteBase++;
        idx++;
    } else if (str[idx] == 'b') {
        noteBase--;
        idx++;
    }

    // Parse octave
    if (idx >= (int)str.length()) return 0;
    
    std::string octaveStr = str.substr(idx);
    int octave = std::atoi(octaveStr.c_str());

    // MIDI Note = (Octave + 1) * 12 + noteBase
    int note = (octave + 1) * 12 + noteBase;
    return std::max(0, std::min(127, note));
}

static std::string getNoteName(uint8_t note) {
	static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	const int octave = static_cast<int>(note) / 12 - 1;
	const int index = static_cast<int>(note) % 12;
	char buffer[8] = {};
	std::snprintf(buffer, sizeof(buffer), "%s%d", names[index], octave);
	return std::string(buffer);
}

static uint64_t parseMBT(const char* s, uint32_t ppqn) {
    if (!s || !*s) return 0;
    
    // Check if raw ticks (only digits)
    bool raw = true;
    for (const char* p = s; *p; ++p) {
        if (!isdigit(*p)) { raw = false; break; }
    }
    if (raw) return std::strtoull(s, nullptr, 10);

    // M:B:T parser
    int measure = 1;
    int beat = 1;
    int tick = 0;
    
    std::sscanf(s, "%d:%d:%d", &measure, &beat, &tick);
    
    if (measure < 1) measure = 1;
    if (beat < 1) beat = 1;
    if (tick < 0) tick = 0;
    
    // Assuming 4/4 based on formatTick hardcoded assumption
    uint32_t beatsPerMeasure = 4;
    
    uint64_t total = 0;
    total += static_cast<uint64_t>(measure - 1) * beatsPerMeasure * ppqn;
    total += static_cast<uint64_t>(beat - 1) * ppqn;
    total += static_cast<uint64_t>(tick);
    
    return total;
}

static std::string formatMBT(uint64_t tick, uint32_t ppqn) {
    if (ppqn == 0) ppqn = DEFAULT_PPQN;
    const uint32_t beatsPerMeasure = 4;
    const uint64_t ticksPerMeasure = static_cast<uint64_t>(ppqn) * beatsPerMeasure;
    const uint64_t measure = tick / ticksPerMeasure + 1;
    const uint64_t beat = (tick / ppqn) % beatsPerMeasure + 1;
    const uint64_t tickR = tick % ppqn;
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%llu:%02llu:%03llu",
        static_cast<unsigned long long>(measure),
        static_cast<unsigned long long>(beat),
        static_cast<unsigned long long>(tickR));
    return std::string(buffer);
}

class GridInput : public Fl_Input {
public:
    using NavigationCallback = std::function<void(int)>;
    GridInput(int x, int y, int w, int h) : Fl_Input(x, y, w, h) {}
    
    void setNavigationCallback(NavigationCallback cb) { navCb_ = cb; }

    int handle(int event) override {
        if (event == FL_KEYDOWN) {
             int key = Fl::event_key();
             if (key == FL_Up || key == FL_Down || key == FL_Enter || key == FL_KP_Enter) {
                 if (navCb_) navCb_(key);
                 return 1;
             }
        }
        // Handle losing focus or other triggers if base Fl_Input doesn't callback
        return Fl_Input::handle(event);
    }
private:
    NavigationCallback navCb_;
};

class EventRowsWidget : public Fl_Widget {
public:
    EventRowsWidget(int x, int y, int w, int h, EventList* parentList) 
        : Fl_Widget(x, y, w, h), list_(parentList) {}

    void draw() override;
    int handle(int event) override;

    void resize(int x, int y, int w, int h) override {
        Fl_Widget::resize(x, y, w, h);
    }
private:
    EventList* list_;
};

EventList::EventList(int x, int y, int w, int h) : Fl_Group(x, y, w, h) {
    const int headerHeight = 24;
    
    // Header Buttons
    int btnW = 60;
    int btnH = 20;
    
    insertButton_ = new Fl_Button(0, 0, btnW, btnH, "Insert");
    insertButton_->labelsize(10); 
    insertButton_->box(FL_FLAT_BOX);
    insertButton_->color(FL_LIGHT2);
    insertButton_->tooltip("Insert Event (Insert Key)");
    insertButton_->callback([](Fl_Widget* w, void* v) {
        static_cast<EventList*>(v)->insertEvent();
    }, this);

    deleteButton_ = new Fl_Button(0, 0, btnW, btnH, "Delete");
    deleteButton_->labelsize(10);
    deleteButton_->box(FL_FLAT_BOX);
    deleteButton_->color(FL_LIGHT2);
    deleteButton_->tooltip("Delete Event (Delete Key)");
    deleteButton_->callback([](Fl_Widget* w, void* v) {
        static_cast<EventList*>(v)->deleteSelectedEvent();
    }, this);

    // Scroll
    scroll_ = new Fl_Scroll(x, y + headerHeight, w, h - headerHeight);
    scroll_->type(Fl_Scroll::VERTICAL); // Or BOTH if columns get too wide
    scroll_->begin();
    
    // Content Widget
    // Position it at scroll_->x(), scroll_->y() initially.
    rowsWidget_ = new EventRowsWidget(x, y + headerHeight, w, 100, this);
    
    // Input Overlay (child of Scroll)
    // CRITICAL: Initialize at valid coordinates inside the scroll area.
    // Initializing at (0,0) (Window coords) expands the scroll bounding box to the top-left of the window,
    // causing massive empty space above the list if the list is not at 0,0.
    auto* input = new GridInput(x, y + headerHeight, 0, 0);
    input->setNavigationCallback([this](int key) {
        this->stopEdit(true);
        // After stopping edit, perform navigation
        if (key == FL_Up) this->moveCursor(-1, 0);
        else if (key == FL_Down) this->moveCursor(1, 0);
        else if (key == FL_Enter || key == FL_KP_Enter) this->moveCursor(1, 0); 
    });
	editInput_ = input;
    
    editInput_->hide(); // Hidden initially
	editInput_->callback(editCallback, this);
	editInput_->when(FL_WHEN_NOT_CHANGED);

    scroll_->end();
    
    resizable(scroll_); // Scroll resizes with group
	end();
    
    // Initial layout fix
    resize(x, y, w, h);
}

EventList::~EventList() {
    // fltk deletes children automatically
}

void EventList::resize(int X, int Y, int W, int H) {
    // Let Fl_Group handle the resizing of the scroll view (as it is the resizable() widget)
    Fl_Group::resize(X, Y, W, H);
    
    // Manually enforce Button positioning (Fixed Header, Right aligned)
    // Fl_Group resize might try to scale them, but we want fixed size/margin.
    const int headerHeight = 24;
    int btnW = 60;
    int btnH = 20;
    int btnY = Y + (headerHeight - btnH) / 2;
    int right = X + W;
    
    if (deleteButton_) deleteButton_->resize(right - btnW - 2, btnY, btnW, btnH);
    if (insertButton_) insertButton_->resize(right - 2*btnW - 4, btnY, btnW, btnH);
    
    // Ensure content widget width tracks the scroll view width
    if (scroll_ && rowsWidget_) {
        // We only update size. Position is managed by Fl_Scroll logic.
        int targetW = std::max(scroll_->w(), 480);
        if (rowsWidget_->w() != targetW) {
             rowsWidget_->size(targetW, rowsWidget_->h());
        }
    }
}

// Delegate drawing
void EventList::draw() {
    // Draw Header Background
	const int headerHeight = 24;
	fl_push_clip(x(), y(), w(), headerHeight);
    
	fl_color(FL_DARK3);
	fl_rectf(x(), y(), w(), headerHeight);
	fl_color(FL_WHITE);
    fl_font(FL_HELVETICA, 12);
	const int headerTextY = y() + 16;
	fl_draw("Measure:Beat:Tick", x() + getColumnX(0), headerTextY);
	fl_draw("Event Type", x() + getColumnX(1), headerTextY);
	fl_draw("Data1", x() + getColumnX(2), headerTextY);
	fl_draw("Data2", x() + getColumnX(3), headerTextY);
	fl_draw("Duration", x() + getColumnX(4), headerTextY);
	fl_color(FL_DARK1);
	fl_line(x(), y() + headerHeight, x() + w(), y() + headerHeight);
    
    fl_pop_clip();

    // Draw Children (Scroll, Buttons)
	draw_children();
}

// EventRowsWidget handles the actual list content
void EventRowsWidget::draw() {
    // We are inside a scroll, so x() and y() are moved by the scroll offset.
    // Coordinates are absolute window coordinates.
    
    // Access parent data
    const auto& rows = list_->rows_;
    int cursorRow = list_->cursorRow_;
    int cursorCol = list_->cursorCol_;
    Fl_Widget* input = list_->editInput_;
    
    const int rowHeight = 18;
    
    // Calculate visible range optimization (simple culling)
    // Parent is Scroll.
    Fl_Scroll* scroll = (Fl_Scroll*)parent();
    int sy = scroll->y(); // top of clip region
    int sh = scroll->h();
    
    // Relative position of this widget
    int myY = y();

    // Which rows overlap [sy, sy+sh]?
    // row Y is myY + i*rowHeight
    // visible if (rowTop < sy + sh) && (rowBot > sy)
    
    int startRow = 0;
    if (myY < sy) {
        startRow = (sy - myY) / rowHeight;
    }
    if (startRow < 0) startRow = 0;
    
    int endRow = startRow + (sh / rowHeight) + 2; 
    if (endRow > (int)rows.size()) endRow = (int)rows.size();
    
    // Clear Background for the visible area
    fl_color(FL_DARK2);
    // Be careful with coordinates. We are drawing inside scroll clip.
    // But this widget is potentially smaller/larger than scroll port?
    // We should clear the rect covering startRow to endRow.
    int clearY = myY + (startRow * rowHeight);
    int clearH = (endRow - startRow) * rowHeight;
    // Extend clearance to bottom if needed or entire widget?
    // Since we only draw text for these rows, clearing just them is efficient.
    // But if we scroll, Fl_Scroll copies pixels.
    // If endRow < size, we are fine.
    fl_rectf(x(), clearY, w(), clearH);
	
    // Draw cursor if it exists
    if (!input->visible() && cursorRow >= 0 && cursorRow < static_cast<int>(rows.size())) {
        int cy = myY + (cursorRow * rowHeight); 
        int cx = list_->x() + list_->getColumnX(cursorCol) - 2; // X corresponds to Header X
        int cw = list_->getColumnWidth(cursorCol);
        
        fl_color(FL_SELECTION_COLOR);
        fl_rectf(cx, cy, cw, rowHeight);
        fl_color(FL_WHITE); // Text color on selection
    }

	for (int i = startRow; i < endRow; ++i) {
		const auto& row = rows[i];
		const std::string timestamp = formatMBT(row.absTick, 0); // Need ppqn access?
        // Actually update formatMBT to handle Song logic or pass ppqn from List
        // formatMBT uses 0->480 default. Good enough for display generally or we fetch from list.
        
		const char* status = linearseq::EventList::editCallback == nullptr ? "" : ""; // Dummy
        // We need statusName helper. Copy it or make it static?
        // Let's duplicate statusName switch for now or move to static.
        const char * statusStr = "Unknown";
        switch (row.event->status) {
			case MidiStatus::NoteOn: statusStr = "Note On"; break;
			case MidiStatus::NoteOff: statusStr = "Note Off"; break;
			case MidiStatus::ControlChange: statusStr = "CC"; break;
			case MidiStatus::ProgramChange: statusStr = "Program"; break;
			case MidiStatus::PitchBend: statusStr = "Pitch Bend"; break;
			default: statusStr = "Other"; break;
        }

		std::string data1;
		std::string data2;
		std::string duration;

		switch (row.event->status) {
			case MidiStatus::NoteOn:
			case MidiStatus::NoteOff:
				data1 = getNoteName(row.event->data1);
				data2 = std::to_string(row.event->data2);
				duration = row.event->duration > 0 ? std::to_string(row.event->duration) : "";
				break;
			case MidiStatus::ControlChange:
				data1 = std::to_string(row.event->data1);
				data2 = std::to_string(row.event->data2);
				break;
			case MidiStatus::ProgramChange:
				data1 = std::to_string(row.event->data1);
				break;
			case MidiStatus::PitchBend:
				data1 = std::to_string(row.event->data1);
				data2 = std::to_string(row.event->data2);
				break;
			default:
				data1 = std::to_string(row.event->data1);
				data2 = std::to_string(row.event->data2);
				break;
		}

		const int textY = myY + i * rowHeight + 13; // text baseline approx
        
        // Change color for selected row text? 
        if (i == cursorRow) {
            fl_color(FL_WHITE);
        } else {
            fl_color(FL_BLACK); // Or Light2... previously FL_LIGHT2 (greenish grey) on Dark2 bg?
            // Wait, previous background was Dark2.
            // We aren't drawing background here yet.
        }
        
        // Draw alternate background?
        // Let's just draw text for now to test.
        // Actually, we need to clear background if not standard.
        // Default FLTK window is specific color.
        
		fl_draw(timestamp.c_str(), list_->x() + list_->getColumnX(0), textY);
		fl_draw(statusStr, list_->x() + list_->getColumnX(1), textY);
		fl_draw(data1.c_str(), list_->x() + list_->getColumnX(2), textY);
		fl_draw(data2.c_str(), list_->x() + list_->getColumnX(3), textY);
		fl_draw(duration.c_str(), list_->x() + list_->getColumnX(4), textY);
	}
    
    // Draw children (Edit Input)
    // EventRowsWidget is Fl_Widget, not Fl_Group. 
    // Children (like EditInput) are managed by parent Fl_Scroll or are siblings.
}

int EventRowsWidget::handle(int event) {
    if (event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE) {
        if (list_->editInput_->visible()) {
            list_->stopEdit(true);
        }

        list_->take_focus(); // Ensure parent group (or logical focus) gets keys
        
        int localY = Fl::event_y() - y();
        const int rowHeight = 18;
        if (localY >= 0) {
            int r = localY / rowHeight;
            if (r >= 0 && r < static_cast<int>(list_->rows_.size())) {
                list_->cursorRow_ = r;
            }
        }
        
        // Clicked column?
        int localX = Fl::event_x() - list_->x(); // Relative to header X
        for (int c = 0; c < 5; ++c) {
            int cx = list_->getColumnX(c);
            int cw = list_->getColumnWidth(c);
            if (localX >= cx && localX < cx + cw) {
                list_->cursorCol_ = c;
                break;
            }
        }
        
        list_->ensureCursorVisible();
        redraw();
        // Don't consume return 1 here? If we do, parent might not see it?
        // Actually, we handled it.
        return 1; 
    }
    return Fl_Widget::handle(event);
}

int EventList::handle(int event) {
    // Delegate key events if we have focus
    if (event == FL_KEYDOWN) {
        // ... (Existing key logic)
        // Need to refactor key logic to use 'rowsWidget_' not 'this' for some things?
        // Key logic modifies cursorRow_, which is in EventList. So logic stays here.
        // Just need to ensure redraw calls rowsWidget_->redraw() or this->redraw().
        
        int key = Fl::event_key();
        if (rows_.empty()) return 1;

		if (key == FL_Enter || key == FL_KP_Enter) {
			startEdit();
			return 1;
		}

        if (key == FL_Delete) {
            deleteSelectedEvent();
            return 1;
        }

        if (key == FL_Insert) {
            insertEvent();
            return 1;
        }
        
         // Type-through support
        const char* text = Fl::event_text();
        if (text && text[0]) {
             // Event Type column (1) - cycle through types with key presses
             if (cursorCol_ == 1 && !rows_.empty() && cursorRow_ >= 0 && cursorRow_ < static_cast<int>(rows_.size())) {
                 char c = std::tolower(text[0]);
                 MidiEvent* evt = rows_[cursorRow_].event;
                 
                 if (c == 'n') {
                     evt->status = MidiStatus::NoteOn;
                     // Set reasonable defaults for notes
                     if (evt->data1 == 0) evt->data1 = 60; // Middle C
                     if (evt->data2 == 0) evt->data2 = 100; // Velocity
                     if (evt->duration == 0) evt->duration = song_.ppqn; // 1 beat
                 } else if (c == 'c') {
                     evt->status = MidiStatus::ControlChange;
                     // Set reasonable defaults for CC
                     evt->data1 = (evt->data1 > 127) ? 7 : evt->data1; // Volume if unset
                     evt->data2 = (evt->data2 > 127) ? 127 : evt->data2;
                     evt->duration = 0; // CC doesn't use duration
                 } else if (c == 'p') {
                     evt->status = MidiStatus::ProgramChange;
                     // Set reasonable defaults for PC
                     evt->data1 = (evt->data1 > 127) ? 0 : evt->data1; // Piano
                     evt->data2 = 0; // PC only uses data1
                     evt->duration = 0; // PC doesn't use duration
                 }
                 
                 if (onSongChanged_) {
                     onSongChanged_(song_);
                 }
                 redraw();
                 return 1;
             }
             
             // Other columns - start edit
             if (cursorCol_ == 0 || (cursorCol_ >= 2 && cursorCol_ <= 4)) {
                 if (std::isdigit(text[0]) || std::isalpha(text[0])) {
                    startEdit(text);
                    return 1;
                 }
             }
        }

		if (key == FL_Down) {
            moveCursor(1, 0);
			return 1;
		}
		if (key == FL_Up) {
            moveCursor(-1, 0);
			return 1;
		}
		if (key == FL_Left) {
            moveCursor(0, -1);
			return 1;
		}
		if (key == FL_Right) {
            moveCursor(0, 1);
			return 1;
		}
		if (key == FL_Page_Down) {
			const int visibleRows = (scroll_->h()) / 18;
            int newRow = std::min(static_cast<int>(rows_.size()) - 1, cursorRow_ + visibleRows);
			cursorRow_ = newRow;
			ensureCursorVisible();
		    if (rowsWidget_) rowsWidget_->redraw();
			return 1;
		}
		if (key == FL_Page_Up) {
			const int visibleRows = (scroll_->h()) / 18;
            int newRow = std::max(0, cursorRow_ - visibleRows);
			cursorRow_ = newRow;
			ensureCursorVisible();
		    if (rowsWidget_) rowsWidget_->redraw();
			return 1;
		}
		if (key == FL_Home) {
			cursorRow_ = 0;
			ensureCursorVisible();
		    if (rowsWidget_) rowsWidget_->redraw();
			return 1;
		}
		if (key == FL_End) {
			cursorRow_ = static_cast<int>(rows_.size()) - 1;
			ensureCursorVisible();
		    if (rowsWidget_) rowsWidget_->redraw();
			return 1;
		}
    }
    
    // Pass to children (Buttons, Scroll)
	return Fl_Group::handle(event);
}



void EventList::setOnSongChanged(std::function<void(const Song&)> cb) {
	onSongChanged_ = cb;
}

void EventList::setSong(const Song& song) {
	song_ = song;
	rebuildRows();
	redraw();
}

void EventList::setTrackFilter(int trackIndex) {
	trackFilter_ = trackIndex;
	rebuildRows();
	redraw();
}

void EventList::setItemFilter(int itemIndex) {
	itemFilter_ = itemIndex;
	rebuildRows();
	redraw();
}

int EventList::contentHeight() const {
	const int headerHeight = 24;
	const int rowHeight = 18;
	return headerHeight + rowHeight + static_cast<int>(rows_.size()) * rowHeight + 8;
}

void EventList::rebuildRows() {
	rows_.clear();
	for (size_t t = 0; t < song_.tracks.size(); ++t) {
		if (trackFilter_ >= 0 && static_cast<int>(t) != trackFilter_) {
			continue;
		}
		auto& track = song_.tracks[t];
		for (size_t i = 0; i < track.items.size(); ++i) {
			if (itemFilter_ >= 0 && static_cast<int>(i) != itemFilter_) {
				continue;
			}
			auto& item = track.items[i];
			const uint64_t itemStart = item.startTick;
			for (size_t e = 0; e < item.events.size(); ++e) {
				EventRow row;
				row.absTick = itemStart + item.events[e].tick;
				row.trackIndex = static_cast<int>(t);
				row.itemIndex = static_cast<int>(i);
				row.eventIndex = static_cast<int>(e);
				// Storing pointer to event in LOCAL song_ copy.
                // This is safe because `rows_` lives as long as `song_` isn't mutated
                // which only happens in setSong (which calls rebuildRows).
				row.event = &item.events[e];
				rows_.push_back(row);
			}
		}
	}

	std::sort(rows_.begin(), rows_.end(), [](const EventRow& a, const EventRow& b) {
		return a.absTick < b.absTick;
	});
	
	// Reset cursor if out of bounds
	if (rows_.empty()) {
	    cursorRow_ = 0;
	} else if (cursorRow_ >= static_cast<int>(rows_.size())) {
	    cursorRow_ = static_cast<int>(rows_.size() - 1);
	}

    // Update content widget size
    if (rowsWidget_ && scroll_) {
        // Calculate total column width
        int minW = 480; // Approx sum of columns (8+150+100+70+70+70 = 468)
        int w = std::max(scroll_->w(), minW);
        int h = static_cast<int>(rows_.size()) * 18;
        if (h < 1) h = 1; // Minimum height to avoid issues
        
        // Resize content widget (keeping its current position which Fl_Scroll manages)
        rowsWidget_->size(w, h);
    }
}

int EventList::getColumnX(int col) const {
	// 0: Timestamp (8)
	// 1: Status (160)
	// 2: Data1 (260)
	// 3: Data2 (330)
	// 4: Duration (400)
	switch (col) {
		case 0: return 8;
		case 1: return 160;
		case 2: return 260;
		case 3: return 330;
		case 4: return 400;
		default: return 0;
	}
}

int EventList::getColumnWidth(int col) const {
	switch (col) {
		case 0: return 150; // Timestamp
		case 1: return 100; // Status
		case 2: return 70;  // Data1
		case 3: return 70;  // Data2
		case 4: return 70;  // Duration
		default: return 0;
	}
}

void EventList::ensureCursorVisible() {
    if (!scroll_ || cursorRow_ < 0) return;

    const int rowHeight = 18;
    int contentTop = cursorRow_ * rowHeight;
    int contentBottom = contentTop + rowHeight;
    
    int scrollY = scroll_->yposition();
    int h = scroll_->h();
    
    if (contentTop < scrollY) {
        scroll_->scroll_to(scroll_->xposition(), contentTop);
    } else if (contentBottom > scrollY + h) {
        scroll_->scroll_to(scroll_->xposition(), contentBottom - h);
    }
}


void EventList::moveCursor(int dRow, int dCol) {
    if (rows_.empty()) return;

    if (dRow != 0) {
        int newRow = cursorRow_ + dRow;
        if (newRow < 0) newRow = 0;
        if (newRow >= static_cast<int>(rows_.size())) newRow = static_cast<int>(rows_.size()) - 1;
        cursorRow_ = newRow;
        ensureCursorVisible();
        redraw();
    }
    
    if (dCol != 0) {
        int newCol = cursorCol_ + dCol;
        if (newCol < 0) newCol = 0;
        if (newCol > 4) newCol = 4;
        cursorCol_ = newCol;
        redraw();
    }
}

void EventList::startEdit(const char* initialValue) {
	if (rows_.empty() || cursorRow_ < 0 || cursorRow_ >= static_cast<int>(rows_.size())) {
		return;
	}

	if (cursorCol_ == 1 || cursorCol_ > 4) {
		return;
	}

	const int rowHeight = 18;
    const int headerHeight = 24;
    // Calculate position
    // editInput_ is child of scroll_. 
    // It should be positioned relative to the content start, which is rowsWidget_->y().
    // Since editInput_ is also a child of scroll_, Fl_Scroll will manage its scrolling movement.
    // We just need to place it over the correct row initially.
    
    int cx = x() + getColumnX(cursorCol_);
    // Use rowsWidget_->y() (absolute window coord of content top) + offset
    int baseTop = (rowsWidget_) ? rowsWidget_->y() : (y() + headerHeight);
	int cy = baseTop + (cursorRow_ * rowHeight);

    editInput_->resize(cx, cy, getColumnWidth(cursorCol_), rowHeight);
    
    // Get current value
    const auto& row = rows_[cursorRow_];
    
    // Check if this is a "Note" column
    bool isNoteColumn = (cursorCol_ == 2 && (row.event->status == MidiStatus::NoteOn || row.event->status == MidiStatus::NoteOff));
    
    if (isNoteColumn || cursorCol_ == 0) {
        editInput_->type(FL_NORMAL_INPUT);
    } else {
        editInput_->type(FL_INT_INPUT);
    }
    
    if (initialValue) {
        editInput_->value(initialValue);
        editInput_->position(strlen(initialValue)); // cursor at end
    } else {
        std::string currentVal;
        
        if (cursorCol_ == 0) {
            currentVal = formatMBT(row.absTick, song_.ppqn);
        } else if (isNoteColumn) {
            currentVal = getNoteName(row.event->data1);
        } else {
            if (cursorCol_ == 2) currentVal = std::to_string(row.event->data1);
            else if (cursorCol_ == 3) currentVal = std::to_string(row.event->data2);
            else if (cursorCol_ == 4) currentVal = std::to_string(row.event->duration);
        }
        
        editInput_->value(currentVal.c_str());
        editInput_->position(0, currentVal.length()); // select all
    }

    editInput_->show();
    editInput_->take_focus();
}

void EventList::stopEdit(bool save) {
    if (!editInput_->visible()) return;
    
    if (save && cursorRow_ >= 0 && cursorRow_ < static_cast<int>(rows_.size())) {
        // Ensure we are operating on the correct row. 
        // Note: rows_ might have been sorted/rebuilt if we are not careful, causing index shift?
        // No, rows_ is only rebuilt on setSong/insert/delete or time edit completion.
        // We are currently IN the edit completion.
        
        MidiEvent* evt = rows_[cursorRow_].event;
        const char* sVal = editInput_->value();
        
        // Time logic
        if (cursorCol_ == 0) {
             uint32_t ppqn = song_.ppqn > 0 ? song_.ppqn : DEFAULT_PPQN;
             uint64_t newAbsTick = parseMBT(sVal, ppqn);
             
             // Check if value actually changed
             // Calculate current absTick to compare?
             // Or just proceed.
             
             // We need to resolve back to the item
             // Assuming rows_ constains pointers to current song_ structure
             int tIdx = rows_[cursorRow_].trackIndex;
             int iIdx = rows_[cursorRow_].itemIndex;
             
             if (tIdx >= 0 && tIdx < (int)song_.tracks.size()) {
                 auto& track = song_.tracks[tIdx];
                 if (iIdx >= 0 && iIdx < (int)track.items.size()) {
                     auto& item = track.items[iIdx];
                     
                     // Check if we need to expand the item start backward
                     if (newAbsTick < static_cast<uint64_t>(item.startTick)) {
                         uint32_t delta = item.startTick - static_cast<uint32_t>(newAbsTick);
                         item.startTick = static_cast<uint32_t>(newAbsTick);
                         item.lengthTicks += delta;
                         
                         // Shift all events forward relative to the new start to maintain their absolute position
                         for (auto& e : item.events) {
                             e.tick += delta;
                         }
                         // 'evt' is in this list, so it was shifted too, but we overwrite it below.
                     }

                     // Check if we need to extend the item length
                     uint64_t itemEnd = static_cast<uint64_t>(item.startTick) + item.lengthTicks;
                     if (newAbsTick >= itemEnd) {
                          // Expand length to cover the new event start
                          // Adding one beat buffer
                          uint32_t buffer = (song_.ppqn > 0) ? song_.ppqn : DEFAULT_PPQN;
                          item.lengthTicks = static_cast<uint32_t>(newAbsTick - item.startTick) + buffer;
                     }

                     uint32_t newRelTick = static_cast<uint32_t>(newAbsTick - item.startTick);
                     evt->tick = newRelTick;
                     
                     rows_[cursorRow_].absTick = newAbsTick;
                 }
             }
        }
        // Data1 logic (could be Note or Int)
        else if (cursorCol_ == 2) {
             bool isNote = (evt->status == MidiStatus::NoteOn || evt->status == MidiStatus::NoteOff);
             int val = 0;
             if (isNote) {
                 val = parseNoteName(sVal);
             } else {
                 val = std::atoi(sVal);
             }
             if (val > 127) val = 127;
             // Don't clamp bottom for notes too hard? 0-127 is standard.
             if (val < 0) val = 0;
             evt->data1 = static_cast<uint8_t>(val);
        } 
        else if (cursorCol_ == 3) {
             int val = std::atoi(sVal);
             if (val > 127) val = 127; 
             if (val < 0) val = 0;
             evt->data2 = static_cast<uint8_t>(val);
        } 
        else if (cursorCol_ == 4) {
             int val = std::atoi(sVal);
             if (val < 0) val = 0;
             evt->duration = static_cast<uint32_t>(val);
        }
        
        if (onSongChanged_) {
            onSongChanged_(song_);
        }

        // If we edited time, we must rebuild rows to update cached absTicks and sort order
        if (cursorCol_ == 0) {
             MidiEvent* target = evt;
             rebuildRows();
             // Find cursor again
             for(size_t i=0; i<rows_.size(); ++i) {
                 if (rows_[i].event == target) {
                     cursorRow_ = static_cast<int>(i);
                     ensureCursorVisible();
                     break;
                 }
             }
        }
        redraw();
    }
    
    editInput_->hide();
    take_focus();
}

void EventList::editCallback(Fl_Widget* w, void* v) {
    EventList* el = static_cast<EventList*>(v);
    el->stopEdit(true);
}

void EventList::insertEvent() {
    // Only allow insertion if a specific item is selected (context safety)
    if (itemFilter_ < 0) return;

    if (song_.tracks.empty()) return;

    // Determine context
    int trackIndex = 0;
    int itemIndex = 0;
    uint32_t tick = 0;
    
    // If we have a cursor, use it
    if (!rows_.empty() && cursorRow_ >= 0 && cursorRow_ < (int)rows_.size()) {
        const auto& row = rows_[cursorRow_];
        trackIndex = row.trackIndex;
        itemIndex = row.itemIndex;
        // Insert AFTER the current event? Or AT?
        // Let's insert AT same time, then we can change it.
        // Actually, logic usually is "insert here".
        if (row.event) {
             tick = row.event->tick; 
        }
    } else {
        // Fallback: Use filters or defaults
        if (trackFilter_ >= 0) trackIndex = trackFilter_;
        // Find valid item
        if (trackIndex < (int)song_.tracks.size()) {
             if (!song_.tracks[trackIndex].items.empty()) {
                  itemIndex = 0; // Default to first item
                  if (itemFilter_ >= 0) itemIndex = itemFilter_;
             } else {
                  // No items in track. Cannot insert event without item.
                  // (Optionally create item here, but simpler to abort)
                  return;
             }
        } else {
            return;
        }
    }
    
    // Validate indices
    if (trackIndex < 0 || trackIndex >= (int)song_.tracks.size()) return;
    auto& track = song_.tracks[trackIndex];
    if (itemIndex < 0 || itemIndex >= (int)track.items.size()) return;
    auto& item = track.items[itemIndex];
    
    // Create Event
    MidiEvent evt;
    evt.tick = tick;
    evt.status = MidiStatus::NoteOn;
    evt.channel = track.channel; // Use track channel
    evt.data1 = 60; // Middle C
    evt.data2 = 100;
    evt.duration = song_.ppqn; // 1 beat
    
    item.events.push_back(evt);
    
    // Notify
    if (onSongChanged_) onSongChanged_(song_);
    rebuildRows();
    
    // Find our new event (it's at the end of the item list, but where in rows?)
    // It has same tick as inserted position.
    // Let's just try to find the event that matches address? 
    // Wait, push_back might reallocate vector, so address not stable? Yes, reallocate.
    // So we can't search by address of 'evt' (stack) or reference.
    // We roughly know it is the last event in that item.
    
    // Let's search for the event we just added:
    // It's at item.events.back().
    // We can't rely on pointers from before rebuildRows.
    // But we know trackIndex, itemIndex, and eventIndex = size-1.
    
    // Actually, rebuildRows sorts.
    // But we can scan rows_ for (trackIndex, itemIndex, eventIndex).
    int targetEventIdx = (int)item.events.size() - 1;
    
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].trackIndex == trackIndex && 
            rows_[i].itemIndex == itemIndex &&
            rows_[i].eventIndex == targetEventIdx) {
            cursorRow_ = (int)i;
            ensureCursorVisible();
            redraw();
            break;
        }
    }
    redraw();
}

void EventList::deleteSelectedEvent() {
    // Only allow deletion if a specific item is selected
    if (itemFilter_ < 0) return;

    if (rows_.empty() || cursorRow_ < 0 || cursorRow_ >= (int)rows_.size()) return;
    
    const auto& row = rows_[cursorRow_];
    int tIdx = row.trackIndex;
    int iIdx = row.itemIndex;
    int eIdx = row.eventIndex;
    
    // Sanity check
    if (tIdx < 0 || tIdx >= (int)song_.tracks.size()) return;
    auto& track = song_.tracks[tIdx];
    if (iIdx < 0 || iIdx >= (int)track.items.size()) return;
    auto& item = track.items[iIdx];
    if (eIdx < 0 || eIdx >= (int)item.events.size()) return;
    
    // Delete
    item.events.erase(item.events.begin() + eIdx);
    
    // Notify
    if (onSongChanged_) onSongChanged_(song_);
    rebuildRows(); // Pointers invalidated!
    
    // Fix cursor
    if (cursorRow_ >= (int)rows_.size()) {
        cursorRow_ = (int)rows_.size() - 1;
    }
    ensureCursorVisible();
    redraw();
}

} // namespace linearseq

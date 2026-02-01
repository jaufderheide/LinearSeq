#include "ui/TrackRowView.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace linearseq {

TrackRowView::TrackRowView(int x, int y, int w, int h)
	: Fl_Group(x, y, w, h), muteButton_(nullptr), soloButton_(nullptr), nameLabel_(nullptr), channelInput_(nullptr), trackIndex_(-1) {
    
    box(FL_NO_BOX);
	begin();
	
	// Mute button
	muteButton_ = new Fl_Button(x + MUTE_X, y + 6, MUTE_W, h - 12, "M");
	muteButton_->type(FL_TOGGLE_BUTTON);
	muteButton_->box(FL_FLAT_BOX);
	muteButton_->color(FL_DARK3);
	muteButton_->selection_color(fl_rgb_color(200, 0, 0)); // Red when active
	muteButton_->labelsize(10);
	muteButton_->labelcolor(FL_WHITE);
	muteButton_->callback([](Fl_Widget*, void* data) {
		static_cast<TrackRowView*>(data)->onMuteToggled();
	}, this);
	
	// Solo button
	soloButton_ = new Fl_Button(x + SOLO_X, y + 6, SOLO_W, h - 12, "S");
	soloButton_->type(FL_TOGGLE_BUTTON);
	soloButton_->box(FL_FLAT_BOX);
	soloButton_->color(FL_DARK3);
	soloButton_->selection_color(fl_rgb_color(255, 200, 0)); // Yellow when active
	soloButton_->labelsize(10);
	soloButton_->labelcolor(FL_WHITE);
	soloButton_->callback([](Fl_Widget*, void* data) {
		static_cast<TrackRowView*>(data)->onSoloToggled();
	}, this);
	
	nameLabel_ = new Fl_Box(x + NAME_LABEL_X, y + 6, NAME_LABEL_W, h - 12);
	nameLabel_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	nameLabel_->labelcolor(FL_WHITE);
    

	channelInput_ = new Fl_Int_Input(x + CHANNEL_INPUT_X, y + 6, CHANNEL_INPUT_W, h - 12);
	channelInput_->type(FL_INT_INPUT);
	channelInput_->box(FL_FLAT_BOX); // Ensure flat style
	channelInput_->color(FL_LIGHT2); // Background color
	channelInput_->selection_color(fl_rgb_color(0, 120, 215)); // Standard blue selection
	channelInput_->textcolor(FL_BLACK);
	channelInput_->callback([](Fl_Widget*, void* data) {
	 	static_cast<TrackRowView*>(data)->onChannelChanged();
    }, this);    
	end();
}

void TrackRowView::setTrack(const Track& track, int index, uint32_t ppqn) {
    track_ = &track;
	trackIndex_ = index;
    ppqn_ = ppqn;
    
	const char* name = track.name.empty() ? "Track" : track.name.c_str();
	nameLabel_->label(name);
	channelInput_->value(std::to_string(static_cast<int>(track.channel) + 1).c_str());
	
	// Update mute button state
	if (muteButton_) {
		muteButton_->value(track.mute ? 1 : 0);
	}
	
	// Update solo button state
	if (soloButton_) {
		soloButton_->value(track.solo ? 1 : 0);
	}
	
    redraw();
}

void TrackRowView::setSelected(bool selected) {
	selected_ = selected;
    redraw();
}

void TrackRowView::setSelectedItems(const std::set<int>& itemIndices) {
    selectedItems_ = itemIndices;
    redraw();
}

void TrackRowView::setChannelChangedCallback(std::function<void(int, int)> cb) {
	onChannelChanged_ = std::move(cb);    
}

void TrackRowView::setItemSelectionCallback(std::function<void(int, std::set<int>)> cb) {
    onItemSelectionChanged_ = std::move(cb);
}

void TrackRowView::setItemsMovedCallback(std::function<void(int, const std::vector<std::pair<int, uint32_t>>&)> cb) {
    onItemsMoved_ = std::move(cb);
}

void TrackRowView::setSetTimeCallback(std::function<void(uint32_t)> cb) {
    onSetTime_ = std::move(cb);
}

void TrackRowView::setMuteChangedCallback(std::function<void(int, bool)> cb) {
	onMuteChanged_ = std::move(cb);
}

void TrackRowView::setSoloChangedCallback(std::function<void(int, bool)> cb) {
	onSoloChanged_ = std::move(cb);
}

int TrackRowView::trackIndex() const {
	return trackIndex_;
}

void TrackRowView::resize(int x, int y, int w, int h) {
    Fl_Group::resize(x, y, w, h);
    // Enforce Layout
    if (muteButton_) muteButton_->resize(x + MUTE_X, y + 6, MUTE_W, h - 12);
    if (soloButton_) soloButton_->resize(x + SOLO_X, y + 6, SOLO_W, h - 12);
    if (nameLabel_) nameLabel_->resize(x + NAME_LABEL_X, y + 6, NAME_LABEL_W, h - 12);
    if (channelInput_) channelInput_->resize(x + CHANNEL_INPUT_X, y + 6, CHANNEL_INPUT_W, h - 12);
}

void TrackRowView::draw() {
    // 1. Draw Background
    fl_push_clip(x(), y(), w(), h());
    
    // Header Background
    fl_color(FL_GRAY); // Standard header bg
    if (selected_) {
        fl_color(FL_DARK2); // Active header
    }
    fl_rectf(x(), y(), HEADER_WIDTH, h());
    
    // Timeline Background
    fl_color(FL_DARK1); // Timeline bg
    fl_rectf(x() + HEADER_WIDTH, y(), w() - HEADER_WIDTH, h());
    
        
    // 1. Draw Children (Label, Input)
    draw_children();
    
    // 2 Draw Items (if track data exists)
    if (track_) {
        for (size_t i = 0; i < track_->items.size(); ++i) {
            const auto& item = track_->items[i];
            
            int itemX, itemY, itemW, itemH;
            getItemRect(item, itemX, itemY, itemW, itemH);
            
            bool isSelected = (selectedItems_.count(static_cast<int>(i)) > 0);
            
            // Color logic
            Fl_Color col = FL_DARK2; // Default item color
            if (isSelected) col = 92; // Highlight color
            else if (selected_) col = FL_DARK2; // Track selected but item not
            
            fl_color(col);
            fl_rectf(itemX, itemY, itemW, itemH);
            //fl_color(FL_DARK3);
            //fl_rect(itemX, itemY, itemW, itemH);
        }
    }

    // 3. Grid Lines (Measure Resolution)
    {
        const uint32_t ppqn = (ppqn_ > 0) ? ppqn_ : DEFAULT_PPQN;
        const uint32_t beatsPerMeasure = 4; // Hardcoded for V1
        const uint64_t ticksPerMeasure = static_cast<uint64_t>(ppqn) * beatsPerMeasure;
        const double pixelsPerTick = getPixelsPerTick();
        
        const double measureW = ticksPerMeasure * pixelsPerTick;
        if (measureW > 2.0) { // Only draw if visible
             const int startX = x() + HEADER_WIDTH;
             const int endX = x() + w();
             // Determine how many measures fit
             int m = 0;
             while(true) {
                 double mx = startX + m * measureW;
                 if (mx > endX) break;
                 fl_color(FL_LIGHT3); // Grid color
                 fl_line(static_cast<int>(mx), y(), static_cast<int>(mx), y() + h());
                 m++;
             }
        }
    }
    
    // 4. Separator Line
    fl_color(FL_DARK2);
    fl_line(x(), y() + h() - 1, x() + w(), y() + h() - 1);
    
    fl_pop_clip();
}

int TrackRowView::handle(int event) {
    // CRITICAL: Check for Ctrl shortcuts BEFORE giving children a chance
    // Handle any event type except keyup (to avoid duplicates)
    if ((Fl::event_state() & FL_CTRL) && event != FL_KEYUP) {
        int key = Fl::event_key();
        // These are application-level shortcuts - don't let children consume them
        if (key == 'c' || key == 'v' || key == 'x' || key == 'z') {
            return 0; // Let it bubble up immediately
        }
    }
    
    // Give children (like input box) a chance for other events
    if (Fl_Group::handle(event)) {
        return 1;
    }

    if (event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE) {
        take_focus();
        int localX = Fl::event_x() - x();
        
        if (localX > HEADER_WIDTH) {
             // Check items
            int clickedItem = -1;
            
            if (track_) {
                int mx = Fl::event_x();
                int my = Fl::event_y();
                for (size_t i = 0; i < track_->items.size(); ++i) {
                     const auto& item = track_->items[i];
                     int ix, iy, iw, ih;
                     getItemRect(item, ix, iy, iw, ih);
                     
                     if (mx >= ix && mx <= ix + iw &&
                         my >= iy && my <= iy + ih) {
                         clickedItem = static_cast<int>(i);
                         break;
                     }
                }
            }

            // Selection Logic
            if (clickedItem != -1) {
                // Move playhead to item start tick (Issue #9)
                if (onSetTime_ && clickedItem < static_cast<int>(track_->items.size())) {
                    onSetTime_(track_->items[clickedItem].startTick);
                }
                
                bool isCtrl = (Fl::event_state() & FL_CTRL) != 0;
                bool isSelected = selectedItems_.count(clickedItem) > 0;
                
                if (isCtrl) {
                    if (isSelected) {
                        // Normally toggle off, but if we drag immediately, it's awkward.
                        // Standard: toggle immediately on push.
                        selectedItems_.erase(clickedItem);
                        // If we erased it, we can't drag it. Or can we?
                        // Usually you can't drag something you just deselected.
                    } else {
                        selectedItems_.insert(clickedItem);
                    }
                } else {
                    if (!isSelected) {
                         // Click on unselected item without ctrl -> select only this
                         selectedItems_.clear();
                         selectedItems_.insert(clickedItem);
                    }
                    // If isSelected is true and no Ctrl, do nothing to selection (prepare for drag of group)
                    // If we don't drag, we should clear others on release? For now, skip that.
                }

                // Prepare Drag for ALL selected items
                if (selectedItems_.count(clickedItem)) {
                    isDragging_ = true;
                    dragFocusIndex_ = clickedItem;
                    initialDragTicks_.clear();
                    
                    // Store initial tick for everyone
                    for (int idx : selectedItems_) {
                        if (idx >= 0 && idx < static_cast<int>(track_->items.size())) {
                            initialDragTicks_[idx] = track_->items[idx].startTick;
                        }
                    }

                    // Calculate offset in ticks from focus item start
                    double pixelsPerTick = getPixelsPerTick();
                    if (pixelsPerTick > 0.0) {
                        int pixelOffset = Fl::event_x() - (x() + HEADER_WIDTH);
                        double tickAtMouse = pixelOffset / pixelsPerTick;
                        uint32_t itemStart = track_->items[dragFocusIndex_].startTick;
                        if (tickAtMouse >= itemStart)
                            dragOriginOffset_ = static_cast<uint32_t>(tickAtMouse - itemStart);
                        else 
                            dragOriginOffset_ = 0;
                    }
                } else {
                    isDragging_ = false;
                }
            } else {
                 // Clicked empty space
                 if (!(Fl::event_state() & FL_CTRL)) {
                     selectedItems_.clear();
                 }
                 
                 // Set Time
                 double pixelsPerTick = getPixelsPerTick();
                 if (pixelsPerTick > 0.0) {
                     int pixelOffset = Fl::event_x() - (x() + HEADER_WIDTH);
                     if (pixelOffset < 0) pixelOffset = 0;
                     uint32_t tick = static_cast<uint32_t>(pixelOffset / pixelsPerTick);
                     
                     // Snap to measure boundary unless Shift is held
                     bool isShift = (Fl::event_state() & FL_SHIFT) != 0;
                     if (!isShift) {
                         const uint32_t ppqn = (ppqn_ > 0) ? ppqn_ : DEFAULT_PPQN;
                         const uint32_t beatsPerMeasure = 4;
                         const uint32_t ticksPerMeasure = ppqn * beatsPerMeasure;
                         // Round to nearest measure
                         tick = ((tick + (ticksPerMeasure / 2)) / ticksPerMeasure) * ticksPerMeasure;
                     }
                     
                     if (onSetTime_) onSetTime_(tick);
                 }
            }
            
            if (onItemSelectionChanged_) {
                onItemSelectionChanged_(trackIndex_, selectedItems_); 
            }
            redraw();
            return 1;
        } else {
            // Header clicked
            if (onItemSelectionChanged_) {
                // Select track, deselect item
                onItemSelectionChanged_(trackIndex_, {});
            }
             // Ensure track selection event is fired by invoking the callback even if items are empty.
            // Wait, onItemSelectionChanged_ updates items. The `setItemSelectionCallback` in TrackView
            // handles track selection logic too.
            return 1;
        }
    } else if (event == FL_DRAG && isDragging_) {
        if (track_ && dragFocusIndex_ >= 0) {
            int mx = Fl::event_x();
            double pixelsPerTick = getPixelsPerTick();
            if (pixelsPerTick > 0.0) {
                int pixelOffset = mx - (x() + HEADER_WIDTH);
                double tickAtMouse = pixelOffset / pixelsPerTick;
                
                // Calculate raw new start tick for the FOCUS item
                long long rawFocusStart = static_cast<long long>(tickAtMouse) - dragOriginOffset_;
                if (rawFocusStart < 0) rawFocusStart = 0;
                
                // Snap focus item to measure
                const uint32_t ppqn = (ppqn_ > 0) ? ppqn_ : DEFAULT_PPQN;
                const uint32_t measureTicks = ppqn * 4;
                long long snappedFocusStart = ((rawFocusStart + (measureTicks / 2)) / measureTicks) * measureTicks;
                
                // Determine Delta from initial position of focus item
                long long initialFocusStart = 0;
                if (initialDragTicks_.count(dragFocusIndex_)) initialFocusStart = initialDragTicks_[dragFocusIndex_];
                
                long long delta = snappedFocusStart - initialFocusStart;
                
                // Apply delta to all items, collecting updates
                std::vector<std::pair<int, uint32_t>> updates;
                bool anyChanged = false;
                
                for (int idx : selectedItems_) {
                    if (initialDragTicks_.count(idx)) {
                        long long initialPos = initialDragTicks_[idx];
                        long long newPos = initialPos + delta;
                        if (newPos < 0) newPos = 0;
                        
                        // Check against current track data to minimize noise?
                        // Or just report all
                        if (static_cast<uint32_t>(newPos) != track_->items[idx].startTick) {
                            anyChanged = true;
                        }
                        updates.push_back({idx, static_cast<uint32_t>(newPos)});
                    }
                }
                
                if (anyChanged && onItemsMoved_) {
                     onItemsMoved_(trackIndex_, updates);
                }
            }
        }
        return 1;
    } else if (event == FL_RELEASE) {
        if (isDragging_) {
            isDragging_ = false;
            dragFocusIndex_ = -1;
            initialDragTicks_.clear();
            return 1;
        }
    } else if (event == FL_SHORTCUT || event == FL_KEYDOWN || event == FL_KEYBOARD || event == FL_KEYUP) {
        // Don't consume keyboard events - let them bubble up

        return 0;
    }
    
    return Fl_Widget::handle(event);
}

void TrackRowView::onChannelChanged() {
	const char* value = channelInput_->value();
	if (!value || !*value) {
		return;
	}
	int channel = std::atoi(value);
	if (channel < 1) {
		channel = 1;
	}
	if (channel > 16) {
		channel = 16;
	}
	channelInput_->value(std::to_string(channel).c_str());
	if (onChannelChanged_) {
		onChannelChanged_(trackIndex_, channel);
	}
}

void TrackRowView::onMuteToggled() {
	if (onMuteChanged_ && muteButton_) {
		bool mute = muteButton_->value() != 0;
		onMuteChanged_(trackIndex_, mute);
	}
}

void TrackRowView::onSoloToggled() {
	if (onSoloChanged_ && soloButton_) {
		bool solo = soloButton_->value() != 0;
		onSoloChanged_(trackIndex_, solo);
	}
}

double TrackRowView::getPixelsPerTick() const {
    const uint32_t ppqn = (ppqn_ > 0) ? ppqn_ : DEFAULT_PPQN;
    const uint32_t beatsPerMeasure = 4;
    const uint64_t ticksPerMeasure = static_cast<uint64_t>(ppqn) * beatsPerMeasure;
    const int measureWidth = 100;
    return static_cast<double>(measureWidth) / static_cast<double>(ticksPerMeasure);
}

void TrackRowView::getItemRect(const MidiItem& item, int& rx, int& ry, int& rw, int& rh) const {
    const double pixelsPerTick = getPixelsPerTick();
    rx = x() + HEADER_WIDTH + static_cast<int>(item.startTick * pixelsPerTick);
    ry = y() + 6;
    rw = std::max(6, static_cast<int>(item.lengthTicks * pixelsPerTick));
    rh = h() - 12;
}

} // namespace linearseq

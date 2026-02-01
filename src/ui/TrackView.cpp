#include "ui/TrackView.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cstdio>

#include "ui/TrackRowView.h"

namespace linearseq {

TrackView::TrackView(int x, int y, int w, int h) : Fl_Group(x, y, w, h) {
	box(FL_NO_BOX);
	end();
}

void TrackView::setSong(const Song& song) {
	song_ = song;
	const size_t trackCount = song_.tracks.size();
	const int rowHeight = 32;

	if (rowViews_.size() > trackCount) {
		for (size_t i = trackCount; i < rowViews_.size(); ++i) {
			remove(rowViews_[i]);
			delete rowViews_[i];
		}
		rowViews_.resize(trackCount);
	}

	if (rowViews_.size() < trackCount) {
		begin();
		for (size_t i = rowViews_.size(); i < trackCount; ++i) {
			auto* row = new TrackRowView(0, 0, w(), rowHeight);
			row->setChannelChangedCallback([this](int index, int channel) {
				if (onChannelChanged_) {
					onChannelChanged_(index, channel);
				}
			});
            row->setItemSelectionCallback([this](int trackIdx, std::set<int> items) {
                // When a row detects a click:
                // 1. Select the track
                if (trackIdx != selectedTrack_) {
                    selectedTrack_ = trackIdx;
                    if (onSelectionChanged_) onSelectionChanged_(selectedTrack_);
                    
                    // Update selection visuals
                    for (auto* r : rowViews_) {
                        r->setSelected(r->trackIndex() == selectedTrack_);
                    }
                }
                
                // 2. Select the items
                selectedItems_ = items;
                if (onItemSelectionChanged_) onItemSelectionChanged_(selectedTrack_, selectedItems_);
                
                // Update visuals
                for (auto* r : rowViews_) {
                    r->setSelectedItems(r->trackIndex() == selectedTrack_ ? selectedItems_ : std::set<int>{});
                }
                redraw();
            });
            row->setItemsMovedCallback([this, row](int trackIdx, const std::vector<std::pair<int, uint32_t>>& updates) {
                if (onItemsMoved_) {
                    onItemsMoved_(trackIdx, updates);
                }
            });
            row->setSetTimeCallback([this](uint32_t tick) {
                if (onSetTime_) onSetTime_(tick);
            });
            row->setMuteChangedCallback([this](int trackIdx, bool mute) {
                if (onMuteChanged_) onMuteChanged_(trackIdx, mute);
            });
            row->setSoloChangedCallback([this](int trackIdx, bool solo) {
                if (onSoloChanged_) onSoloChanged_(trackIdx, solo);
            });
			rowViews_.push_back(row);
		}
		end();
	}

	for (size_t i = 0; i < trackCount; ++i) {
		rowViews_[i]->setTrack(song_.tracks[i], static_cast<int>(i), song_.ppqn);
		rowViews_[i]->setSelected(static_cast<int>(i) == selectedTrack_);
        rowViews_[i]->setSelectedItems(static_cast<int>(i) == selectedTrack_ ? selectedItems_ : std::set<int>{});
	}

	layoutRows();

	if (selectedTrack_ >= static_cast<int>(song_.tracks.size())) {
		selectedTrack_ = song_.tracks.empty() ? -1 : 0;
	}
	if (selectedTrack_ < 0 || selectedTrack_ >= static_cast<int>(song_.tracks.size())) {
		selectedItems_.clear();
	} else {
		auto& items = song_.tracks[selectedTrack_].items;
        // Validate selected items
        std::set<int> validItems;
        for (int idx : selectedItems_) {
            if (idx >= 0 && idx < static_cast<int>(items.size())) {
                validItems.insert(idx);
            }
        }
        selectedItems_ = validItems;
	}
	redraw();
}

void TrackView::layoutRows() {
	const int rowHeight = 32;
	for (size_t i = 0; i < rowViews_.size(); ++i) {
		rowViews_[i]->resize(x(), y() + static_cast<int>(i * rowHeight), w(), rowHeight);
	}
}

void TrackView::resize(int x, int y, int w, int h) {
	Fl_Group::resize(x, y, w, h);
	layoutRows();
}

void TrackView::setSelectionChanged(std::function<void(int)> cb) {
	onSelectionChanged_ = std::move(cb);
}

void TrackView::setItemSelectionChanged(std::function<void(int, std::set<int>)> cb) {
	onItemSelectionChanged_ = std::move(cb);
}

void TrackView::setItemsMoved(std::function<void(int, const std::vector<std::pair<int, uint32_t>>&)> cb) {
	onItemsMoved_ = std::move(cb);
}

void TrackView::setChannelChanged(std::function<void(int, int)> cb) {
	onChannelChanged_ = std::move(cb);
}

void TrackView::setMuteChanged(std::function<void(int, bool)> cb) {
	onMuteChanged_ = std::move(cb);
}

void TrackView::setSoloChanged(std::function<void(int, bool)> cb) {
	onSoloChanged_ = std::move(cb);
}

void TrackView::setSetTime(std::function<void(uint32_t)> cb) {
    onSetTime_ = std::move(cb);
}

void TrackView::setPlayheadTick(uint32_t tick) {
    if (playheadTick_ != tick) {
        playheadTick_ = tick;
        redraw();
    }
}

int TrackView::selectedTrack() const {
	return selectedTrack_;
}

void TrackView::setSelectedTrack(int index) {
    if (song_.tracks.empty()) {
		selectedTrack_ = -1;
		redraw();
		return;
	}
	selectedTrack_ = std::max(0, std::min(index, static_cast<int>(song_.tracks.size() - 1)));
	selectedItems_.clear();
	for (size_t i = 0; i < rowViews_.size(); ++i) {
		rowViews_[i]->setSelected(static_cast<int>(i) == selectedTrack_);
        rowViews_[i]->setSelectedItems({});
	}
	redraw();
}

const std::set<int>& TrackView::selectedItems() const {
	return selectedItems_;
}

void TrackView::setSelectedItems(const std::set<int>& indices) {
	if (selectedTrack_ < 0 || selectedTrack_ >= static_cast<int>(song_.tracks.size())) {
		selectedItems_.clear();
		redraw();
		return;
	}
    
    selectedItems_ = indices;
    
    // Update active row
    if (selectedTrack_ >= 0 && selectedTrack_ < (int)rowViews_.size()) {
        rowViews_[selectedTrack_]->setSelectedItems(selectedItems_);
    }
	redraw();
}

int TrackView::contentHeight() const {
	const int rowHeight = 32;
	const int trackCount = static_cast<int>(song_.tracks.size());
	return std::max(rowHeight, trackCount * rowHeight);
}

int TrackView::contentWidth() const {
	const int labelWidth = 110;
	const uint32_t ppqn = song_.ppqn > 0 ? song_.ppqn : DEFAULT_PPQN;
	const uint32_t beatsPerMeasure = 4;
	const uint64_t ticksPerMeasure = static_cast<uint64_t>(ppqn) * beatsPerMeasure;
	const int measureWidth = 100; // Synced with TrackRowView
	const double pixelsPerTick = static_cast<double>(measureWidth) / static_cast<double>(ticksPerMeasure);

	uint64_t maxEndTick = 0;
	for (const auto& track : song_.tracks) {
		for (const auto& item : track.items) {
			maxEndTick = std::max(maxEndTick, static_cast<uint64_t>(item.startTick + item.lengthTicks));
		}
	}

	const int contentPixels = labelWidth + static_cast<int>(maxEndTick * pixelsPerTick) + 40;
	return std::max(labelWidth + 200, contentPixels);
}

void TrackView::draw() {
	fl_push_clip(x(), y(), w(), h());
	
    // Background
	fl_color(FL_DARK1);
	fl_rectf(x(), y(), w(), h());

    // Draw children (TrackRowViews)
	Fl_Group::draw();
    
    // Draw Playhead
    {
         const uint32_t ppqn = song_.ppqn > 0 ? song_.ppqn : DEFAULT_PPQN;
         const uint32_t beatsPerMeasure = 4;
         const uint64_t ticksPerMeasure = static_cast<uint64_t>(ppqn) * beatsPerMeasure;
         const int measureWidth = 100; 
         const double pixelsPerTick = static_cast<double>(measureWidth) / static_cast<double>(ticksPerMeasure);
         
         int cx = x() + TrackRowView::HEADER_WIDTH + static_cast<int>(playheadTick_ * pixelsPerTick);
         
         if (cx >= x() + TrackRowView::HEADER_WIDTH && cx < x() + w()) {
             fl_color(FL_RED);
             fl_line(cx, y(), cx, y() + h());
         }
    }

	fl_pop_clip();
}

int TrackView::handle(int event) {
    // Pass through keyboard shortcuts for any event type except keyup
    if ((Fl::event_state() & FL_CTRL) && event != FL_KEYUP) {
        int key = Fl::event_key();
        if (key == 'c' || key == 'v' || key == 'x' || key == 'z') {
            return 0;
        }
    }
    
	if (Fl_Group::handle(event)) {
		return 1;
	}
    
    // Note: Copy/Paste shortcuts are now handled at MainWindow level for consistency
    // No need to handle FL_SHORTCUT here
    
    // Fallback for clicks outside rows (if any)
    if (event == FL_PUSH) {
        // Optional: Deselect if clicking empty space? 
        // For now, do nothing or just return 0
    }

	return Fl_Widget::handle(event); // Call base widget handle (not group, since we already tried group)
}

} // namespace linearseq

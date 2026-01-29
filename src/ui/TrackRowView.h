#pragma once

#include <FL/Fl_Group.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Box.H>

#include <functional>
#include <set>
#include <map>

#include "core/Types.h"

namespace linearseq {

class TrackRowView : public Fl_Group {
public:
	TrackRowView(int x, int y, int w, int h);

	void setTrack(const Track& track, int index, uint32_t ppqn);
	void setSelected(bool selected);
    void setSelectedItems(const std::set<int>& itemIndices);
	void setChannelChangedCallback(std::function<void(int, int)> cb);
    void setItemSelectionCallback(std::function<void(int, std::set<int>)> cb);
    
    // Callback: vector of {itemIndex, newStartTick}
    void setItemsMovedCallback(std::function<void(int, const std::vector<std::pair<int, uint32_t>>&)> cb);
    void setSetTimeCallback(std::function<void(uint32_t)> cb);

	int trackIndex() const;
    
    void resize(int x, int y, int w, int h) override;
    void draw() override;
    int handle(int event) override;

private:
	void onChannelChanged();

	Fl_Box* nameLabel_;
	Fl_Int_Input* channelInput_;
	int trackIndex_;
    const Track* track_ = nullptr; // Pointer to track data owned by TrackView/Song
    uint32_t ppqn_ = DEFAULT_PPQN;
    bool selected_ = false;
    std::set<int> selectedItems_;
    
    // Drag state
    bool isDragging_ = false;
    int dragFocusIndex_ = -1; // The item under the mouse that started the drag
    uint32_t dragOriginOffset_ = 0; // Tick offset from dragFocus item start
    std::map<int, uint32_t> initialDragTicks_; // Initial start ticks of all selected items
    
	std::function<void(int, int)> onChannelChanged_;
    std::function<void(int, std::set<int>)> onItemSelectionChanged_;
    std::function<void(int, const std::vector<std::pair<int, uint32_t>>&)> onItemsMoved_;
    std::function<void(uint32_t)> onSetTime_;

    double getPixelsPerTick() const;
    void getItemRect(const MidiItem& item, int& rx, int& ry, int& rw, int& rh) const;
    static constexpr int HEADER_WIDTH = 110;
};

} // namespace linearseq

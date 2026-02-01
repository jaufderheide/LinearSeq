#pragma once

#include <FL/Fl_Group.H>

#include <functional>
#include <set>
#include <vector>

#include "core/Types.h"

namespace linearseq {

class TrackView : public Fl_Group {
public:
	TrackView(int x, int y, int w, int h);

	void setSong(const Song& song);
	void setSelectionChanged(std::function<void(int)> cb);
	void setItemSelectionChanged(std::function<void(int, std::set<int>)> cb);
	void setItemsMoved(std::function<void(int, const std::vector<std::pair<int, uint32_t>>&)> cb);
	void setChannelChanged(std::function<void(int, int)> cb);
    void setSetTime(std::function<void(uint32_t)> cb);
    void setMuteChanged(std::function<void(int, bool)> cb);
    void setSoloChanged(std::function<void(int, bool)> cb);
    
	int selectedTrack() const;
	void setSelectedTrack(int index);
	const std::set<int>& selectedItems() const;
	void setSelectedItems(const std::set<int>& indices);
    
    void setPlayheadTick(uint32_t tick);
    
	int contentHeight() const;
	int contentWidth() const;

	void draw() override;
	int handle(int event) override;
	void resize(int x, int y, int w, int h) override;

private:
	void layoutRows();

	Song song_{};
    uint32_t playheadTick_ = 0;
	int selectedTrack_ = -1;
	std::set<int> selectedItems_;
	std::function<void(int)> onSelectionChanged_;
	std::function<void(int, std::set<int>)> onItemSelectionChanged_;
    std::function<void(int, const std::vector<std::pair<int, uint32_t>>&)> onItemsMoved_;
	std::function<void(int, int)> onChannelChanged_;
    std::function<void(uint32_t)> onSetTime_;    std::function<void(int, bool)> onMuteChanged_;    std::function<void(int, bool)> onSoloChanged_;
	std::vector<class TrackRowView*> rowViews_;
};

} // namespace linearseq

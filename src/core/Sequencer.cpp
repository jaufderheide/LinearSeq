#include "core/Sequencer.h"
#include "audio/AlsaDriver.h"

#include <algorithm>
#include <chrono>

namespace linearseq {

Sequencer::Sequencer()
	: playing_(false), 
	  playbackIndex_(0),
	  recording_(false), 
	  activeTrack_(0), 
	  driver_(nullptr), 
	  recordingTrack_(-1), 
	  recordingItem_(-1) {
	clock_.setTickCallback([this](uint64_t tick) { onTick(tick); });
}

Sequencer::~Sequencer() {
	stopRecording();
	stop();
}

void Sequencer::setSong(const Song& song) {
	std::lock_guard<std::mutex> lock(mutex_);
	song_ = song;
	clock_.setBpm(song_.bpm);
	clock_.setPpqn(song_.ppqn);
}

Song Sequencer::song() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return song_;
}

void Sequencer::play(uint64_t startTick) {
	if (playing_.exchange(true)) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(pendingMutex_);
		pendingOffs_.clear();
	}
	buildPlaybackQueue();
	
	// Skip ahead to startTick in the playback queue
	playbackIndex_ = 0;
	for (size_t i = 0; i < playbackQueue_.size(); ++i) {
		if (playbackQueue_[i].absTick >= startTick) {
			playbackIndex_ = i;
			break;
		}
	}
	
	clock_.start(startTick);
}

void Sequencer::stop() {
	if (!playing_.exchange(false)) {
		return;
	}
	// Send All Notes Off to prevent stuck notes
	allNotesOff();
	clock_.stop();
}

void Sequencer::allNotesOff() {
	if (!driver_ || !driver_->isOpen()) {
		return;
	}
	driver_->sendAllNotesOff();
	// Also clear any pending note offs
	std::lock_guard<std::mutex> lock(pendingMutex_);
	pendingOffs_.clear();
}

bool Sequencer::isPlaying() const {
	return playing_.load();
}

void Sequencer::buildPlaybackQueue() {
	std::lock_guard<std::mutex> lock(mutex_);
	playbackQueue_.clear();
	playbackIndex_ = 0;

	for (const auto& track : song_.tracks) {
		for (const auto& item : track.items) {
			for (const auto& event : item.events) {
				PlaybackEvent pe;
				pe.absTick = static_cast<uint64_t>(item.startTick) + event.tick;
				pe.duration = event.duration;
				pe.status = event.status;
				// Override event channel with track channel for playback
				pe.channel = track.channel; 
				pe.data1 = event.data1;
				pe.data2 = event.data2;
				playbackQueue_.push_back(pe);
			}
		}
	}

	std::sort(playbackQueue_.begin(), playbackQueue_.end());
}

void Sequencer::startRecording() {
	if (recording_.exchange(true)) {
		return;
	}
	if (!driver_ || !driver_->isOpen()) {
		recording_.store(false);
		return;
	}
	if (!isPlaying()) {
		play();
	}

	const uint64_t startTick = clock_.currentTick();
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (song_.tracks.empty()) {
			Track track;
			track.name = "Track 1";
			track.channel = 0;
			song_.tracks.push_back(track);
		}

		int trackIndex = activeTrack_.load();
		if (trackIndex < 0 || trackIndex >= static_cast<int>(song_.tracks.size())) {
			trackIndex = 0;
		}

		MidiItem item;
		item.startTick = static_cast<uint32_t>(startTick);
		item.lengthTicks = 0;
		song_.tracks[trackIndex].items.push_back(item);

		recordingTrack_ = trackIndex;
		recordingItem_ = static_cast<int>(song_.tracks[trackIndex].items.size() - 1);
		activeNotes_.clear();
	}

	recordThread_ = std::thread(&Sequencer::recordLoop, this);
}

void Sequencer::stopRecording() {
	if (!recording_.exchange(false)) {
		return;
	}
	if (recordThread_.joinable()) {
		recordThread_.join();
	}

	std::lock_guard<std::mutex> lock(mutex_);
	if (recordingTrack_ >= 0 && recordingTrack_ < static_cast<int>(song_.tracks.size())) {
		auto& track = song_.tracks[recordingTrack_];
		if (recordingItem_ >= 0 && recordingItem_ < static_cast<int>(track.items.size())) {
			auto& item = track.items[recordingItem_];
			uint64_t maxLen = item.lengthTicks;
			for (const auto& event : item.events) {
				const uint64_t endTick = static_cast<uint64_t>(event.tick) + event.duration;
				maxLen = std::max(maxLen, endTick);
			}
			item.lengthTicks = static_cast<uint32_t>(maxLen);
		}
	}
}

bool Sequencer::isRecording() const {
	return recording_.load();
}

void Sequencer::setActiveTrack(int index) {
	activeTrack_.store(index);
}

int Sequencer::activeTrack() const {
	return activeTrack_.load();
}

uint64_t Sequencer::currentTick() const {
    return clock_.currentTick();
}

void Sequencer::setDriver(AlsaDriver* driver) {
	driver_ = driver;
}

void Sequencer::recordLoop() {
	while (recording_.load()) {
		MidiEvent inputEvent;
		if (driver_ && driver_->readInputEvent(inputEvent)) {
			const uint64_t nowTick = clock_.currentTick();
			std::lock_guard<std::mutex> lock(mutex_);
			if (recordingTrack_ < 0 || recordingTrack_ >= static_cast<int>(song_.tracks.size())) {
				continue;
			}
			auto& track = song_.tracks[recordingTrack_];
			if (recordingItem_ < 0 || recordingItem_ >= static_cast<int>(track.items.size())) {
				continue;
			}
			auto& item = track.items[recordingItem_];
			const uint64_t itemStart = item.startTick;
			const uint64_t relTick = nowTick > itemStart ? (nowTick - itemStart) : 0;

			if (inputEvent.status == MidiStatus::NoteOn && inputEvent.data2 > 0) {
				MidiEvent ev = inputEvent;
				ev.tick = static_cast<uint32_t>(relTick);
				ev.duration = 0;
				item.events.push_back(ev);
				activeNotes_[{ev.channel, ev.data1}] = {item.events.size() - 1, nowTick};
			} else if (inputEvent.status == MidiStatus::NoteOff ||
				(inputEvent.status == MidiStatus::NoteOn && inputEvent.data2 == 0)) {
				const auto key = std::make_pair(inputEvent.channel, inputEvent.data1);
				auto it = activeNotes_.find(key);
				if (it != activeNotes_.end()) {
					const size_t index = it->second.first;
					const uint64_t startTick = it->second.second;
					if (index < item.events.size()) {
						const uint64_t duration = nowTick > startTick ? (nowTick - startTick) : 0;
						item.events[index].duration = static_cast<uint32_t>(duration);
					}
					activeNotes_.erase(it);
				}
			} else {
				MidiEvent ev = inputEvent;
				ev.tick = static_cast<uint32_t>(relTick);
				ev.duration = 0;
				item.events.push_back(ev);
			}

			item.lengthTicks = std::max(item.lengthTicks, static_cast<uint32_t>(relTick));
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void Sequencer::onTick(uint64_t tick) {
	if (!playing_.load()) {
		return;
	}

	if (!driver_ || !driver_->isOpen()) {
		return;
	}

	// 1. Process Pending Note Offs
	{
		std::lock_guard<std::mutex> lock(pendingMutex_);
		auto it = pendingOffs_.begin();
		while (it != pendingOffs_.end()) {
			if (it->tick <= tick) {
				driver_->sendNoteOff(it->channel, it->note, it->velocity);
				it = pendingOffs_.erase(it);
			} else {
				++it;
			}
		}
	}

	// 2. Process Playback Queue
	// We read without the song mutex because playbackQueue_ is invariant during playback.
	while (playbackIndex_ < playbackQueue_.size()) {
		const auto& event = playbackQueue_[playbackIndex_];
		
		if (event.absTick > tick) {
			break; 
		}
		
		// If we missed a tick (lag), catch up, but only fire if it's not too old? 
		// For now, fire everything to maintain state.
		if (event.absTick <= tick) {
			switch (event.status) {
				case MidiStatus::NoteOn:
					driver_->sendNoteOn(event.channel, event.data1, event.data2);
					if (event.duration > 0) {
						PendingNoteOff pending;
						pending.tick = event.absTick + event.duration;
						pending.channel = event.channel;
						pending.note = event.data1;
						pending.velocity = 0;
						std::lock_guard<std::mutex> lock(pendingMutex_);
						pendingOffs_.push_back(pending);
					}
					break;
				case MidiStatus::NoteOff:
					driver_->sendNoteOff(event.channel, event.data1, event.data2);
					break;
				case MidiStatus::ControlChange:
					driver_->sendControlChange(event.channel, event.data1, event.data2);
					break;
				case MidiStatus::ProgramChange:
					driver_->sendProgramChange(event.channel, event.data1);
					break;
				case MidiStatus::PitchBend:
					// TODO: Implement pitch bend in AlsaDriver
					break;
				default:
					break;
			}
		}
		playbackIndex_++;
	}
}

} // namespace linearseq

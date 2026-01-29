#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "core/Clock.h"
#include "core/Types.h"

namespace linearseq {

class AlsaDriver;

class Sequencer {
public:
	Sequencer();
	~Sequencer();

	void setSong(const Song& song);
	Song song() const;

	void play();
	void stop();
	bool isPlaying() const;

	void startRecording();
	void stopRecording();
	bool isRecording() const;

	void setActiveTrack(int index);
	int activeTrack() const;

    uint64_t currentTick() const;

	void setDriver(AlsaDriver* driver);

private:
	void onTick(uint64_t tick);
	void recordLoop();
	void buildPlaybackQueue();

	struct PendingNoteOff {
		uint64_t tick = 0;
		uint8_t channel = 0;
		uint8_t note = 0;
		uint8_t velocity = 0;
	};

	struct PlaybackEvent {
		uint64_t absTick = 0;
		uint32_t duration = 0;
		MidiStatus status = MidiStatus::NoteOn;
		uint8_t channel = 0;
		uint8_t data1 = 0;
		uint8_t data2 = 0;

		bool operator<(const PlaybackEvent& other) const {
			return absTick < other.absTick;
		}
	};

	mutable std::mutex mutex_;
	std::mutex pendingMutex_;
	
	Song song_;
	Clock clock_;
	AlsaDriver* driver_;

	// Playback State
	std::atomic<bool> playing_;
	std::vector<PlaybackEvent> playbackQueue_;
	size_t playbackIndex_;
	std::vector<PendingNoteOff> pendingOffs_;

	// Recording State
	std::atomic<bool> recording_;
	std::thread recordThread_;
	std::atomic<int> activeTrack_;
	int recordingTrack_;
	int recordingItem_;
	std::map<std::pair<uint8_t, uint8_t>, std::pair<size_t, uint64_t>> activeNotes_;
};

} // namespace linearseq

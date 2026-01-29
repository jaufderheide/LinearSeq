#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace linearseq {

enum class MidiStatus : uint8_t {
	NoteOff = 0x80,
	NoteOn = 0x90,
	PolyAftertouch = 0xA0,
	ControlChange = 0xB0,
	ProgramChange = 0xC0,
	ChannelAftertouch = 0xD0,
	PitchBend = 0xE0
};

constexpr uint32_t DEFAULT_PPQN = 120;
constexpr double DEFAULT_BPM = 120.0;

struct MidiEvent {
	uint32_t tick = 0;
	MidiStatus status = MidiStatus::NoteOn;
	uint8_t channel = 0;
	uint8_t data1 = 0;
	uint8_t data2 = 0;
	uint32_t duration = 0;
};

struct MidiItem {
	uint32_t startTick = 0;
	uint32_t lengthTicks = 0;
	std::vector<MidiEvent> events;
};

struct Track {
	std::string name = "Track 1";
	int alsaClient = -1;
	int alsaPort = -1;
	uint8_t channel = 0;
	std::vector<MidiItem> items;
};

struct Song {
	uint32_t ppqn = DEFAULT_PPQN;
	double bpm = DEFAULT_BPM;
	std::string midiDevice;
	std::vector<Track> tracks;
};

} // namespace linearseq

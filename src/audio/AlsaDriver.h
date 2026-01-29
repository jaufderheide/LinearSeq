#pragma once

#include <cstdint>

#include <alsa/asoundlib.h>

#include "core/Types.h"

namespace linearseq {

class AlsaDriver {
public:
	AlsaDriver();
	~AlsaDriver();

	bool open();
	void close();
	bool isOpen() const;
	int inputPort() const;

	struct PortInfo {
		int client;
		int port;
		std::string name;
	};
	std::vector<PortInfo> listOutputPorts();
	bool connectOutput(int destClient, int destPort);

	bool sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
	bool sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
	bool readInputEvent(MidiEvent& event);

private:
	snd_seq_t* seq_;
	int outPort_;
	int inPort_;
};

} // namespace linearseq

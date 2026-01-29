#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace linearseq {

class Clock {
public:
	using TickCallback = std::function<void(uint64_t)>;

	Clock();
	~Clock();

	void setBpm(double bpm);
	double bpm() const;

	void setPpqn(uint32_t ppqn);
	uint32_t ppqn() const;

	void start();
	void stop();
	bool isRunning() const;

	void setTickCallback(TickCallback cb);
	uint64_t currentTick() const;

private:
	void runLoop();

	std::atomic<bool> running_;
	std::thread thread_;
	std::atomic<double> bpm_;
	std::atomic<uint32_t> ppqn_;
	std::atomic<uint64_t> tickCounter_;
	TickCallback onTick_;
};

} // namespace linearseq

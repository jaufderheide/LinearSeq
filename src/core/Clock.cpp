#include "core/Clock.h"
#include "core/Types.h"

namespace linearseq {

Clock::Clock() : running_(false), bpm_(DEFAULT_BPM), ppqn_(DEFAULT_PPQN), tickCounter_(0) {}

Clock::~Clock() {
	stop();
}

void Clock::setBpm(double bpm) {
	if (bpm <= 0.0) {
		return;
	}
	bpm_.store(bpm, std::memory_order_relaxed);
}

double Clock::bpm() const {
	return bpm_.load(std::memory_order_relaxed);
}

void Clock::setPpqn(uint32_t ppqn) {
	if (ppqn == 0) {
		return;
	}
	ppqn_.store(ppqn, std::memory_order_relaxed);
}

uint32_t Clock::ppqn() const {
	return ppqn_.load(std::memory_order_relaxed);
}

void Clock::start() {
	if (running_.exchange(true)) {
		return;
	}
	tickCounter_.store(0, std::memory_order_relaxed);
	thread_ = std::thread(&Clock::runLoop, this);
}

void Clock::stop() {
	if (!running_.exchange(false)) {
		return;
	}
	if (thread_.joinable()) {
		thread_.join();
	}
}

bool Clock::isRunning() const {
	return running_.load();
}

void Clock::setTickCallback(TickCallback cb) {
	onTick_ = std::move(cb);
}

uint64_t Clock::currentTick() const {
	return tickCounter_.load(std::memory_order_relaxed);
}

void Clock::runLoop() {
	using clock = std::chrono::steady_clock;
	uint64_t tick = 0;
	auto next = clock::now();

	while (running_.load()) {
		const double bpm = bpm_.load(std::memory_order_relaxed);
		const uint32_t ppqn = ppqn_.load(std::memory_order_relaxed);
		const double ticksPerSecond = (bpm / 60.0) * static_cast<double>(ppqn);
		const auto tickDuration = std::chrono::duration<double>(1.0 / ticksPerSecond);

		next += std::chrono::duration_cast<clock::duration>(tickDuration);

		tickCounter_.store(tick, std::memory_order_relaxed);
		if (onTick_) {
			onTick_(tick);
		}
		++tick;

		std::this_thread::sleep_until(next);
	}
}

} // namespace linearseq

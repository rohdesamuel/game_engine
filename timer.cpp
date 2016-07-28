#include "timer.h"

double NS_TO_SEC = 1e9;

Timer::Timer() {
	reset();
	QueryPerformanceFrequency(&freq_);
}

void Timer::start() {
	QueryPerformanceCounter(&start_);
}

void Timer::stop() {
	QueryPerformanceCounter(&end_);
}

double Timer::get_elapsed_ns() {
	return (get_elapsed_cycles() * NS_TO_SEC) / freq_.QuadPart;
}

LONGLONG Timer::get_elapsed_cycles() {
	return end_.QuadPart - start_.QuadPart;
}

LONGLONG Timer::get_clock_frequency() {
	if (!freq_.QuadPart) {
		QueryPerformanceFrequency(&freq_);
	}
	return freq_.QuadPart;
}

void Timer::reset() {
	start_ = end_ = LARGE_INTEGER();
}

WindowTimer::WindowTimer(UINT8 window_size): iterator_(0) {
	window_size_ = max(window_size, 1);
	window_.reserve(window_size_);
	for (UINT8 i = 0; i < window_size_; ++i) {
		window_.push_back(0);
	}
	reset();
	QueryPerformanceFrequency(&freq_);
}

// Starts the timer.
void WindowTimer::start() {
	QueryPerformanceCounter(&start_);
}

// Starts the timer.
void WindowTimer::step() {
	window_[iterator_++] = get_elapsed_cycles();
	iterator_ = iterator_ % window_size_;
}

// Stops the timer.
void WindowTimer::stop() {
	QueryPerformanceCounter(&end_);
}

// Stops and resets the timer.
void WindowTimer::reset() {
	start_ = end_ = LARGE_INTEGER();
	for (UINT8 i = 0; i < window_size_; ++i) {
		window_[i] = 0;
	}
}

// Gets the average amount of elapsed clock cycles since start.
LONGLONG WindowTimer::get_elapsed_cycles() {
	return end_.QuadPart - start_.QuadPart;
}

// Gets the average elapsed time in [ns] since start.
double WindowTimer::get_elapsed_ns() {
	return (get_elapsed_cycles() * NS_TO_SEC) / freq_.QuadPart;
}

// double the average amount of elapsed clock cycles since start.
double WindowTimer::get_avg_elapsed_cycles() {
	double accum = 0;
	for (auto& n : window_) {
		accum += n;
	}
	return accum / window_size_;
}

// Gets the average elapsed time in [ns] since start.
double WindowTimer::get_avg_elapsed_ns() {
	double accum = 0;
	for (auto& n : window_) {
		accum += (n * NS_TO_SEC) / freq_.QuadPart;;
	}
	return accum / window_size_;
}

// Gets the clock frequency of the CPU.
LONGLONG WindowTimer::get_clock_frequency() {
	if (!freq_.QuadPart) {
		QueryPerformanceFrequency(&freq_);
	}
	return freq_.QuadPart;
}
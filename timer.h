#ifndef TIMER__H
#define TIMER__H

#include "std.h"

class Timer {
public:
	Timer();
	// Starts the timer.
	void start();

	// Stops the timer.
	void stop();

	// Stops and resets the timer.
	void reset();

	// Gets the amount of elapsed clock cycles since start.
	LONGLONG get_elapsed_cycles();

	// Gets the elapsed time in [ns] since start.
	double get_elapsed_ns();

	// Gets the clock frequency of the CPU.
	LONGLONG get_clock_frequency();

private:
	LARGE_INTEGER start_;
	LARGE_INTEGER end_;
	LARGE_INTEGER freq_;
};

class WindowTimer {
public:
	WindowTimer(UINT8 window_size = 0);

	// Starts the timer.
	void start();

	// Adds to the window average.
	void step();

	// Stops the timer.
	void stop();

	// Stops and resets the timer.
	void reset();

	// Gets the average amount of elapsed clock cycles since start.
	LONGLONG get_elapsed_cycles();

	// Gets the average elapsed time in [ns] since start.
	double get_elapsed_ns();

	// Gets the average amount of elapsed clock cycles since start.
	double get_avg_elapsed_cycles();

	// Gets the average elapsed time in [ns] since start.
	double get_avg_elapsed_ns();

	// Gets the clock frequency of the CPU.
	LONGLONG get_clock_frequency();

private:
	LARGE_INTEGER start_;
	LARGE_INTEGER end_;
	LARGE_INTEGER freq_;

	UINT8 iterator_;
	UINT8 window_size_;
	std::vector<LONGLONG> window_;
};

#endif
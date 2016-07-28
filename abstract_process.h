#ifndef ABSTRACT_PROCESS__H
#define ABSTRACT_PROCESS__H

#include "std.h"

class AbstractProcess
{
public:
	typedef INT(*ProcessFunction)(AbstractProcess*);

	// Such an egotistical class...
	AbstractProcess(AbstractProcess* me) :me(me) {
		on_create = nullptr;
		on_start = nullptr;
		on_run = nullptr;
		on_restart = nullptr;
		on_resume = nullptr;
		on_pause = nullptr;
		on_stop = nullptr;
		on_destroy = nullptr;
	}

	virtual INT set_args(UINT8 count...) { return 0; }
	virtual INT set_args(va_list args) { return 0; }

	// Called when process is created.
	ProcessFunction on_create;

	// Called when process is started. Can be called after OnStart
	// or OnRestart.
	ProcessFunction on_start;

	// Called when process is restarted. Process can be restarted from
	// exception, or signal.
	ProcessFunction on_restart;

	// REQUIRED: Main function to run for process.
	ProcessFunction on_run;

	// Called when process is resumed. Called after OnStart, or after scheduler
	// resumes execution.
	ProcessFunction on_resume;

	// Called when process is paused to let other tasks run.
	ProcessFunction on_pause;

	// Called when process is stopped.
	ProcessFunction on_stop;

	// Called when process is destroyed.
	ProcessFunction on_destroy;

	AbstractProcess* me;
};

#endif /*ABSTRACT_PROCESS__H*/
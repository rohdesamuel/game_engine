#ifndef ABSTRACT_SCHEDULER__H
#define ABSTRACT_SCHEDULER__H

#include "abstract_process.h"
#include "std.h"

/*

Schedules all needed units and executes functions.

*/
class AbstractScheduler
{
private:
	std::string name_;
	AbstractScheduler* me_;

public:
	AbstractScheduler(const std::string& name);

	// Queue an already made process to be scheduled.
	virtual INT queue(AbstractProcess* process) = 0;

	// Queue a batch of processes at once.
	virtual INT queue_batch(AbstractProcess** process, UINT count) = 0;

	// Start the scheduler.
	virtual INT start() = 0;

	// Stop the scheduler.
	virtual INT stop() = 0;

	const std::string& name() const;
};

#endif
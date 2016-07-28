#include "abstract_scheduler.h"

AbstractScheduler::AbstractScheduler(const std::string& name) : name_(name) { }

const std::string& AbstractScheduler::name() const {
	return name_;
}
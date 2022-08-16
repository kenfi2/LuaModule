#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "thread_base.h"
#include "task.h"

class SchedulerTask : public Task
{
public:
	void setEventId(uint32_t id) {
		eventId = id;
	}
	uint32_t getEventId() const {
		return eventId;
	}

	std::chrono::system_clock::time_point getCycle() const {
		return expiration;
	}

private:
	SchedulerTask(uint32_t delay, std::function<void(void)>&& f) : Task(delay, std::move(f)) {}

	uint32_t eventId = 0;

	friend SchedulerTaskPtr createSchedulerTask(uint32_t, std::function<void(void)>);
};

SchedulerTaskPtr createSchedulerTask(uint32_t delay, std::function<void(void)> f);

struct TaskComparator {
	bool operator()(const SchedulerTaskPtr lhs, const SchedulerTaskPtr rhs) const {
		return lhs->getCycle() > rhs->getCycle();
	}
};

class Scheduler : public Thread<Scheduler>
{
public:
	void main();

	uint32_t addEvent(SchedulerTaskPtr task);
	bool stopEvent(uint32_t eventId);

	void shutdown();

private:
	uint32_t m_lastEventId{ 0 };
	std::priority_queue<SchedulerTaskPtr, std::deque<SchedulerTaskPtr>, TaskComparator> m_eventList;
	std::unordered_set<uint32_t> m_eventIds;
};

TaskPtr createTask(std::function<void(void)> f);
TaskPtr createExpirationTask(uint32_t expiration, std::function<void(void)> f);

extern Scheduler g_scheduler;

#endif

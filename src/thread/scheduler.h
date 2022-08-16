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

	friend SchedulerTask* createSchedulerTask(uint32_t, std::function<void(void)>);
};

SchedulerTask* createSchedulerTask(uint32_t delay, std::function<void(void)> f);

struct TaskComparator {
	bool operator()(const SchedulerTask* lhs, const SchedulerTask* rhs) const {
		return lhs->getCycle() > rhs->getCycle();
	}
};

class Scheduler : public Thread<Scheduler>
{
public:
	void main();

	uint32_t addEvent(uint32_t delay, std::function<void(void)> f);
	uint32_t addEventPointer(SchedulerTask* task);
	bool stopEvent(uint32_t eventId);

	void shutdown();

private:
	uint32_t m_lastEventId{ 0 };
	std::priority_queue<SchedulerTask*, std::deque<SchedulerTask*>, TaskComparator> m_eventList;
	std::unordered_set<uint32_t> m_eventIds;
};

extern Scheduler g_scheduler;

#endif

#include <includes/includes.h>

#include "scheduler.h"
#include "dispatcher.h"

void Scheduler::main()
{
	std::unique_lock<std::mutex> taskLockUnique(m_taskLock, std::defer_lock);

	while (getState() != THREAD_STATE_TERMINATED) {
		std::cv_status ret = std::cv_status::no_timeout;

		taskLockUnique.lock();
		if (m_eventList.empty()) {
			m_taskSignal.wait(taskLockUnique);
		}
		else {
			ret = m_taskSignal.wait_until(taskLockUnique, m_eventList.top()->getCycle());
		}

		if (ret == std::cv_status::timeout && !m_eventList.empty()) {
			SchedulerTaskPtr task = m_eventList.top();
			m_eventList.pop();

			auto it = m_eventIds.find(task->getEventId());
			if (it == m_eventIds.end()) {
				taskLockUnique.unlock();
				continue;
			}

			m_eventIds.erase(it);
			taskLockUnique.unlock();

			task->setDontExpire();
			g_dispatcher.addTask(task, true);
		}
		else {
			taskLockUnique.unlock();
		}
	}
}

uint32_t Scheduler::addEvent(SchedulerTaskPtr task)
{
	bool do_signal = false;

	m_taskLock.lock();

	if (getState() != THREAD_STATE_RUNNING) {
		m_taskLock.unlock();
		return 0;
	}

	if (task->getEventId() == 0) {
		if (++m_lastEventId == 0) {
			m_lastEventId = 1;
		}

		task->setEventId(m_lastEventId);
	}

	uint32_t eventId = task->getEventId();
	m_eventIds.insert(eventId);

	m_eventList.push(task);

	do_signal = (task == m_eventList.top());

	m_taskLock.unlock();

	if (do_signal) {
		m_taskSignal.notify_one();
	}

	return eventId;
}

bool Scheduler::stopEvent(uint32_t eventId)
{
	if (eventId == 0) {
		return false;
	}

	std::lock_guard<std::mutex> lockClass(m_taskLock);

	// search the event id..
	auto it = m_eventIds.find(eventId);
	if (it == m_eventIds.end()) {
		return false;
	}

	m_eventIds.erase(it);
	return true;
}

void Scheduler::shutdown()
{
	setState(THREAD_STATE_TERMINATED);
	m_taskLock.lock();

	//this list should already be empty
	while (!m_eventList.empty()) {
		m_eventList.pop();
	}

	m_eventIds.clear();
	m_taskLock.unlock();
	m_taskSignal.notify_one();
}

SchedulerTaskPtr createSchedulerTask(uint32_t delay, std::function<void(void)> f)
{
	return SchedulerTaskPtr(new SchedulerTask(delay, std::move(f)));
}

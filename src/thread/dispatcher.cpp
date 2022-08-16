#include <includes/includes.h>

#include "dispatcher.h"

TaskPtr createTask(std::function<void(void)> f)
{
	return TaskPtr(new Task(std::move(f)));
}

TaskPtr createTask(uint32_t expiration, std::function<void(void)> f)
{
	return TaskPtr(new Task(expiration, std::move(f)));
}

void Dispatcher::main()
{
	std::unique_lock<std::mutex> taskLockUnique(m_taskLock, std::defer_lock);

	while (getState() != THREAD_STATE_TERMINATED) {
		taskLockUnique.lock();

		if (m_taskList.empty()) {
			m_taskSignal.wait(taskLockUnique);
		}

		if (!m_taskList.empty()) {
			TaskPtr task = m_taskList.front();
			m_taskList.pop_front();
			taskLockUnique.unlock();

			if (!task->hasExpired()) {
				(*task)();
			}
		}
		else {
			taskLockUnique.unlock();
		}
	}
}

void Dispatcher::addTask(TaskPtr task, bool push_front/*= false*/)
{
	bool do_signal = false;

	m_taskLock.lock();

	if (getState() == THREAD_STATE_RUNNING) {
		do_signal = m_taskList.empty();

		if (push_front) {
			m_taskList.push_front(task);
		}
		else {
			m_taskList.push_back(task);
		}
	}

	m_taskLock.unlock();

	if (do_signal) {
		m_taskSignal.notify_one();
	}
}

void Dispatcher::shutdown()
{
	const TaskPtr& task = createTask([this]() {
		setState(THREAD_STATE_TERMINATED);
		m_taskSignal.notify_one();
	});

	std::lock_guard<std::mutex> lockClass(m_taskLock);
	m_taskList.push_back(task);

	m_taskSignal.notify_one();
}

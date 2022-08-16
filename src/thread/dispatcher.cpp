#include <includes/includes.h>

#include "dispatcher.h"

Task* createTask(std::function<void(void)> f)
{
	return new Task(std::move(f));
}

Task* createTask(uint32_t expiration, std::function<void(void)> f)
{
	return new Task(expiration, std::move(f));
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
			Task* task = m_taskList.front();
			m_taskList.pop_front();
			taskLockUnique.unlock();

			if (!task->hasExpired()) {
				(*task)();
			}

			delete task;
		}
		else {
			taskLockUnique.unlock();
		}
	}
}

void Dispatcher::addExpirationTask(uint32_t expiration, std::function<void(void)> f, bool push_front)
{
	return addTaskPointer(createTask(expiration, std::move(f)), push_front);
}

void Dispatcher::addTask(std::function<void(void)> f, bool push_front/*= false*/)
{
	return addTaskPointer(createTask(std::move(f)), push_front);
}

void Dispatcher::addTaskPointer(Task* task, bool push_front)
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
	else {
		delete task;
	}

	m_taskLock.unlock();

	if (do_signal) {
		m_taskSignal.notify_one();
	}
}

void Dispatcher::shutdown()
{
	Task* task = createTask([this]() {
		setState(THREAD_STATE_TERMINATED);
		m_taskSignal.notify_one();
	});

	std::lock_guard<std::mutex> lockClass(m_taskLock);
	m_taskList.push_back(task);

	m_taskSignal.notify_one();
}

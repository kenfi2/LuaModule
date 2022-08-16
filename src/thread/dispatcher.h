#ifndef __DISPATCHER__
#define __DISPATCHER__

#include "thread_base.h"
#include "task.h"

class Dispatcher : public Thread<Dispatcher>
{
public:
	void main();

	void addTask(TaskPtr task, bool push_front = false);

	void shutdown();

private:
	std::list<TaskPtr> m_taskList;
};

TaskPtr createTask(std::function<void(void)> f);
TaskPtr createTask(uint32_t expiration, std::function<void(void)> f);

extern Dispatcher g_dispatcher;

#endif

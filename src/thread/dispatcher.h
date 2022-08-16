#ifndef __DISPATCHER__
#define __DISPATCHER__

#include "thread_base.h"
#include "task.h"

class Dispatcher : public Thread<Dispatcher>
{
public:
	void main();

	void addTask(std::function<void(void)> f, bool push_front = false);
	void addExpirationTask(uint32_t expiration, std::function<void(void)> f, bool push_front = false);
	void addTaskPointer(Task* task, bool push_front); // safe call

	void shutdown();

private:
	std::list<Task*> m_taskList;
};

Task* createTask(std::function<void(void)> f);
Task* createTask(uint32_t expiration, std::function<void(void)> f);

extern Dispatcher g_dispatcher;

#endif

#ifndef __TASK__
#define __TASK__

const int DISPATCHER_TASK_EXPIRATION = 2000;
const auto SYSTEM_TIME_ZERO = std::chrono::system_clock::time_point(std::chrono::milliseconds(0));

#include <lua/luaobject.h>

class Task : public LuaObject
{
public:
	// DO NOT allocate this class on the stack
	explicit Task(std::function<void(void)>&& f) : func(std::move(f)) {}
	Task(uint32_t ms, std::function<void(void)>&& f) :
		expiration(std::chrono::system_clock::now() + std::chrono::milliseconds(ms)), func(std::move(f)) {}

	virtual ~Task() {};

	void operator()() {
		func();
	}

	void setDontExpire() {
		expiration = SYSTEM_TIME_ZERO;
	}

	bool hasExpired() const {
		if (expiration == SYSTEM_TIME_ZERO) {
			return false;
		}
		return expiration < std::chrono::system_clock::now();
	}

protected:
	std::chrono::system_clock::time_point expiration = SYSTEM_TIME_ZERO;

private:
	// Expiration has another meaning for scheduler tasks,
	// then it is the time the task should be added to the
	// dispatcher
	std::function<void(void)> func;
};

#endif

#include <includes/includes.h>
#include <includes/tools.h>

#include <thread/thread.h>

#include <lua/luainterface.h>

Dispatcher g_dispatcher;
Scheduler g_scheduler;

std::mutex g_loaderLock;
std::condition_variable g_loaderSignal;
std::unique_lock<std::mutex> g_loaderUniqueLock(g_loaderLock);

void loaderError(const std::string& error)
{
	std::cout << "> ERROR: " << error << std::endl;
	g_loaderSignal.notify_all();
}

void mainLoader(int argc, char* argv[]);

int main(int argc, char* argv[])
{
	g_dispatcher.start();
	g_scheduler.start();

	g_dispatcher.addTask(std::bind(mainLoader, argc, argv));

	g_loaderSignal.wait(g_loaderUniqueLock); // lock current thread until signaled

	g_scheduler.join();
	g_dispatcher.join();
	return 0;
}

void mainLoader(int argc, char* argv[])
{
	if (!g_lua.init()) {
		loaderError("Fail to start lua interface");
		return;
	}

	if (!g_lua.loadFile("project/main.lua")) {
		loaderError("main.lua not found");
		return;
	}

	g_loaderSignal.notify_all();
}

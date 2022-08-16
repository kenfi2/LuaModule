#ifndef __THREAD__
#define __THREAD__

#include <mutex>
#include <atomic>
#include <thread>

enum ThreadState {
	THREAD_STATE_RUNNING,
	THREAD_STATE_CLOSING,
	THREAD_STATE_TERMINATED
};

template<typename Derived>
class Thread
{
public:
	Thread() {}
	
	void start() {
		setState(THREAD_STATE_RUNNING);
		m_thread = std::thread(&Derived::main, static_cast<Derived*>(this));
	}

	void stop() {
		setState(THREAD_STATE_CLOSING);
	}

	void join() {
		if (m_thread.joinable()) {
			m_thread.join();
		}
	}

protected:
	void setState(ThreadState state) {
		m_state.store(state, std::memory_order_relaxed);
	}

	ThreadState getState() const {
		return m_state.load(std::memory_order_relaxed);
	}

	std::mutex m_taskLock;
	std::condition_variable m_taskSignal;

private:
	std::atomic<ThreadState> m_state{ THREAD_STATE_TERMINATED };
	std::thread m_thread;
};

#endif

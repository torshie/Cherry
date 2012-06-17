#ifndef _CHERRY_THREAD_GUARD_HPP_INCLUDED_
#define _CHERRY_THREAD_GUARD_HPP_INCLUDED_

#include <cherry/thread/Mutex.hpp>

namespace cherry {

class Guard {
public:
	explicit Guard(Mutex& mutex) : mutex(mutex) {
		mutex.lock();
	}

	~Guard() {
		mutex.unlock();
	}

private:
	Mutex& mutex;
};

} // namespace cherry

#endif // _CHERRY_THREAD_GUARD_HPP_INCLUDED_

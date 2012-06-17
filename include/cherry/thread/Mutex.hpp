#ifndef _CHERRY_THREAD_MUTEX_HPP_INCLUDED_
#define _CHERRY_THREAD_MUTEX_HPP_INCLUDED_

#include <pthread.h>

namespace cherry {

class Mutex {
	friend class Guard;
public:
	Mutex() {
		pthread_mutex_init(&mutex, NULL);
	}

	~Mutex() {
		pthread_mutex_destroy(&mutex);
	}

private:
	pthread_mutex_t mutex;

	void lock() {
		pthread_mutex_lock(&mutex);
	}

	void unlock() {
		pthread_mutex_unlock(&mutex);
	}
};

} // namespace cherry

#endif // _CHERRY_THREAD_MUTEX_HPP_INCLUDED_

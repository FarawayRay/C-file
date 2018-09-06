#pragma once

#include <pthread.h>

class Mutex
{	
public:
	Mutex()
	{
		pthread_mutex_init(&mutex, NULL);
	}
	virtual ~Mutex()
	{
		pthread_mutex_destroy(&mutex);
	}
	void lock()
	{
		pthread_mutex_lock(&mutex);
	}
	void unLock()
	{
		pthread_mutex_unlock(&mutex);
	}
private:
    pthread_mutex_t mutex;	
};
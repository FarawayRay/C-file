#pragma once

#include <semaphore.h>

class Sem{
public:
	Sem(){
		sem_init(&sem, 0, 0);
	}
	~Sem(){
		sem_destroy(&sem);
	}
	void semWait(){
		sem_wait(&sem);
	}
	void semPost(){
		sem_post(&sem);
	}
private:
	sem_t sem;
};

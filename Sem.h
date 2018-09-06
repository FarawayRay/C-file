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

#include <condition_variable>
#include <mutex>
	class Semaphore
	{
	public:
		Semaphore(unsigned long count = 0) : m_count(count) {}
 
		Semaphore(const Semaphore&) = delete;
 
		Semaphore& operator=(const Semaphore&) = delete;
 
		void Signal()
		{
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				++m_count;
			}
			m_cv_uptr.notify_one();
		}
 
		void Wait()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			while (m_count == 0) { // we may have spurious wakeup!
				m_cv_uptr.wait(lock);
			}
			--m_count;
		}
 
	private:
		std::mutex m_mutex;
		std::condition_variable m_cv_uptr;
		unsigned long m_count;
	};


#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <thread>
#include <list>
#include <mutex>
#include <atomic>
#include <cmath>

template<typename T = std::mutex>
class RJMutexLIB
{
public:
	void lock()
	{
		m_mutex.lock();
	}
	void unlock()
	{
		m_mutex.unlock();
	}
	static RJMutexLIB* get()
	{
		static RJMutexLIB gp_rjSigMutexLib;
		return &gp_rjSigMutexLib;
	}
private:
	T m_mutex;
};

struct IDataBase
{
	virtual ~IDataBase() {}
	virtual void run() = 0;
};

template <typename T>
struct CDataBase : public IDataBase
{
	CDataBase(T&& f)
		: m_fun(std::forward<T>(f))
	{}

	void run()
	{
		m_fun();
	}

	T m_fun;
};
class RJEventData
{
public:
	template <typename T, typename ... Args>
	RJEventData(T&& f, Args&& ... args)
	{
		m_pFun = makeCDataBase(std::bind(std::forward<T>(f), std::forward<Args>(args)...));
	}

	~RJEventData()
	{
	}
	void run()
	{
		m_pFun->run();
	}
private:

	template <typename T>
	std::shared_ptr<CDataBase<T>> makeCDataBase(T&& f)
	{
		//完美转发
		return std::make_shared<CDataBase<T>>(std::forward<T>(f));
	}
	std::shared_ptr<IDataBase> m_pFun;
};

// this loop for other thread sendevent to main thread
class RJEventLoop
{
	typedef std::list<std::shared_ptr<RJEventData>> eventDataList;
	typedef std::shared_ptr<eventDataList> eventDataListPtr;
public:
	/*
	 * @brief 主线程while中调用,用来遍历
	 */
	bool runEventList()
	{
		if (std::this_thread::get_id() != m_threadId)
		{
			return false;
		}
		eventDataListPtr eventList;
		{
			std::lock_guard<std::mutex> _m(mtx);
			eventList.reset(new eventDataList(*m_eventList));
			m_eventList->clear();
		}
		b_isRuning = true;
		auto iter = eventList->begin();
		while (iter != eventList->end())
		{
			//在run中删除迭代器
			bool b_isUnconn = false;
			if (m_eventUnConnList->size() > 0)
			{
				auto v = m_eventUnConnList->begin();
				while (v != m_eventUnConnList->end())
				{
					if ((*v) == (*iter))
					{
						b_isUnconn = true;
					}
					v++;
				}
			}
			if (!b_isUnconn)
			{
				(*iter)->run();
			}
			else
			{

			}
			iter++;
		}
		b_isRuning = false;
		if (m_eventUnConnList->size() > 0)
		{
			m_eventUnConnList->clear();
		}
		return true;
	}

	static RJEventLoop* getEventLoopCurrThread()
	{
		RJEventLoop*rjt_eventInThisThread = new RJEventLoop();
		return rjt_eventInThisThread;
	}

private:
	RJEventLoop()
		: m_threadId(std::this_thread::get_id())
		, b_isRuning(false)
	{
		rjt_eventInThisThread = this;
		m_eventList.reset(new eventDataList);
		m_eventUnConnList.reset(new eventDataList);
	}

	RJEventLoop*rjt_eventInThisThread{nullptr};
	std::thread::id m_threadId;
	eventDataListPtr m_eventList;
	eventDataListPtr m_eventUnConnList;
	std::mutex mtx;
	bool b_isRuning;
};

template <typename ... Args>
struct ISlotBase
{
	virtual ~ISlotBase() {}
	virtual void sendEvent(Args&& ... args_) = 0;
	virtual std::shared_ptr<RJEventData> sendEventSync(Args&& ... args_) = 0;
};

template <typename PSignal, typename FunSignal, typename ... Args>
struct CSlotBase : public ISlotBase<Args...>
{
	CSlotBase(PSignal* object, const FunSignal& fun)
		: m_object(object)
		, m_fun(fun) {}
	~CSlotBase() {}

	void sendEvent(Args&& ... args_)
	{
		(m_object->*m_fun)(std::forward<Args>(args_)...);
	}
	std::shared_ptr<RJEventData> sendEventSync(Args&& ... args_)
	{
		return std::shared_ptr<RJEventData>(new RJEventData(std::bind(m_fun, m_object, std::forward<Args>(args_)...)));
	}
	PSignal* m_object;
	FunSignal m_fun;
};

template <typename ... Args>
struct RJSlot
{
	template <typename PSignal, typename FunSignal>
	RJSlot(PSignal* object, const FunSignal& fun)
	{
		m_vec_sync_event.reset(new std::vector< std::shared_ptr<RJEventData> >);
		m_pFun = std::make_shared<CSlotBase<PSignal, FunSignal, Args...>>(object, fun);
	}

	~RJSlot()
	{
	}

	void sendEvent(Args ... args_)
	{
		m_pFun->sendEvent(std::forward<Args>(args_)...);
	}

	std::shared_ptr<RJEventData> sendEventSync(Args ... args_)
	{
		auto event = m_pFun->sendEventSync(std::forward<Args>(args_)...);

		//此类中是否一个此实例被管理(可忽略)
		if (!m_vec_sync_event.unique()) //注意：C++17及以后被遗弃
		{
			m_vec_sync_event.reset(new std::vector< std::shared_ptr<RJEventData> >(*m_vec_sync_event));
		}

		m_vec_sync_event->push_back(event);
		return event;
	}

	std::shared_ptr<ISlotBase<Args...>> m_pFun;
	std::shared_ptr< std::vector< std::shared_ptr<RJEventData> > > m_vec_sync_event;//槽函数队列
};

template <typename ... Args>
class RJSignal
{
	typedef std::vector<RJSlot<Args...>*> eventSlotVec;
	typedef std::shared_ptr<eventSlotVec> eventSlotVecPtr;
public:
	RJSignal()
		: m_threadId(std::this_thread::get_id())
		, p_eventLoop(RJEventLoop::getEventLoopCurrThread())
	{
		m_slots.reset(new eventSlotVec);
	}
	~RJSignal()
	{
		auto iter = m_slots->begin();
		while (iter != m_slots->end())
		{
			delete (*iter);
			iter++;
		}
	}

	template <typename PSignal, typename FunSignal>
	bool connect(PSignal* object, const FunSignal& fun)
	{
		if (m_threadId != std::this_thread::get_id())
		{
			return false;
		}
		RJMutexLIB<>::get()->lock();
		{
			auto iter = m_slots->begin();
			while (iter != m_slots->end())
			{
				auto pSlotbase = static_cast<CSlotBase<PSignal, FunSignal, Args...>*>((*iter)->m_pFun.get());
				if ((object == pSlotbase->m_object) && (fun == pSlotbase->m_fun))
				{
					break;
				}
				iter++;
			}
			if (iter != m_slots->end())
			{
				RJMutexLIB<>::get()->unlock();
				return false;
			}
		}
		if (!m_slots.unique())
		{
			m_slots.reset(new eventSlotVec(*m_slots));
		}
		RJSlot<Args...>* t = new RJSlot<Args...>(object, fun);
		m_slots->push_back(t);
		RJMutexLIB<>::get()->unlock();
		return true;
	}

	template <typename PSignal, typename FunSignal>
	bool unconnect(PSignal* object, const FunSignal& fun)
	{
		if (m_threadId != std::this_thread::get_id())
		{
			return false;
		}
		RJMutexLIB<>::get()->lock();
		if (!m_slots.unique())
		{
			m_slots.reset(new eventSlotVec(*m_slots));
		}
		auto iter = m_slots->begin();
		while (iter != m_slots->end())
		{
			auto pSlotbase = static_cast<CSlotBase<PSignal, FunSignal, Args...>*>((*iter)->m_pFun.get());
			if ((object == pSlotbase->m_object) && (fun == pSlotbase->m_fun))
			{
				break;
			}
			iter++;
		}
		if (iter != m_slots->end())
		{
			auto ptr = (*iter);
			{
				auto vec_sync_event = ptr->m_vec_sync_event;
				m_slots->erase(iter);

				if (vec_sync_event->size() > 0)
				{
					auto v = vec_sync_event->begin();
					while (v != vec_sync_event->end())
					{
						if (p_eventLoop->isRunningEventList())
						{
							p_eventLoop->addUnconnectEventList(*v);
						}
						p_eventLoop->removeEventList(*v);
						v++;
					}
				}
			}

			delete (ptr);
			RJMutexLIB<>::get()->unlock();
			return true;
		}
		RJMutexLIB<>::get()->unlock();
		return false;
	}

	void sendEvent(Args... args_)
	{
		if (m_threadId != std::this_thread::get_id())
		{
			return;
		}
		eventSlotVecPtr slot;
		{
			RJMutexLIB<>::get()->lock();
			slot = m_slots;
			RJMutexLIB<>::get()->unlock();
		}

		auto iter = slot->begin();
		while (iter != slot->end())
		{
			if (isExistInSlots(*iter))
			{
				(*iter)->sendEvent((args_)...);
			}
			iter++;
		}
	}

	void sendEventSync(Args ... args_)
	{
		if (m_threadId == std::this_thread::get_id())
		{

		}
		RJMutexLIB<>::get()->lock();
		auto iter = m_slots->begin();
		while (iter != m_slots->end())
		{
			auto event = (*iter)->sendEventSync((args_)...);
			p_eventLoop->addEventList(event);
			iter++;
		}
		RJMutexLIB<>::get()->unlock();
	}

private:

	bool isExistInSlots(RJSlot<Args...>* ptr)
	{
		bool isexist = false;
		auto iter = m_slots->begin();
		while (iter != m_slots->end())
		{
			if (ptr == *iter)
			{
				isexist = true;
				break;
			}
			iter++;
		}
		return isexist;
	}

	eventSlotVecPtr m_slots;
	std::thread::id m_threadId;
	RJEventLoop* p_eventLoop;
};

//连接=>qt connect
#define EConnect(sender, Signal, ...) \
(sender)->Signal.connect(__VA_ARGS__)

//断开=>qt unconnect
#define EUnConnect(sender, Signal, ...) \
(sender)->Signal.unconnect(__VA_ARGS__)

//定义信号=>qt Single
#define ESignal(Signal, ...) RJSignal<__VA_ARGS__> Signal

//发送=>emit
#define Eemit(sender, Signal, ...) \
(sender)->Signal.sendEvent(__VA_ARGS__)

//异步发送=>emit
#define Eemit_Asyn(sender, Signal, ...) \
(sender)->Signal.sendEventSync(__VA_ARGS__)

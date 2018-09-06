#include "Qu1etMessageQueue.h"

Qu1etMessageQueue *Qu1etMessageQueue::pMessageQueue = new Qu1etMessageQueue();
Qu1etMessageQueue *Qu1etMessageQueue::getInstance()
{
	return pMessageQueue;
}

void Qu1etMessageQueue::addItem(CONTROL_MSG &item)
{
	Qu1etMutex oMutex;
	qMessageQueue.emplace(item);
}

bool Qu1etMessageQueue::getFrontItem(CONTROL_MSG &item)
{
	Qu1etMutex oMutex;
	if (qMessageQueue.empty())
	{
		return false;
	}
	item = qMessageQueue.front();
	qMessageQueue.pop();
	return true;
}

size_t Qu1etMessageQueue::getSize()
{
	size_t nCount;
	Qu1etMutex oMutex;
	nCount = qMessageQueue.size();
	return nCount;
}
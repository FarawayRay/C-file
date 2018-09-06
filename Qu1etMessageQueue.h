#pragma once

#include <mutex>
#include "Qu1etTypes.h"
#include "Qu1etMsgDef.h"

class Qu1etMessageQueue
{
	Qu1etMessageQueue(){}
	virtual ~Qu1etMessageQueue(){}
	static Qu1etMessageQueue *pMessageQueue;
	Qu1etQueue<CONTROL_MSG> qMessageQueue;

public:
	static Qu1etMessageQueue *getInstance();
	void addItem(CONTROL_MSG &item);
	bool getFrontItem(CONTROL_MSG &item);
	size_t getSize();
};

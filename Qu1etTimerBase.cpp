#include "Qu1etTimerBase.h"
#include "Qu1etTimerMonitor.h"

Qu1etTimerBase::Qu1etTimerBase()
{
    // TODO Auto-generated constructor stub

}

Qu1etTimerBase::~Qu1etTimerBase()
{
    // TODO Auto-generated destructor stub
}

void Qu1etTimerBase::setTimer(int nTimerID, long nInterval, Qu1etTimerBase *pThis )
{
	if (pThis){
		Qu1etTimerMonitor::setTimer(nTimerID, nInterval, pThis);
	}
	else{
		Qu1etTimerMonitor::setTimer(nTimerID, nInterval, this);
	}
}

bool Qu1etTimerBase::resetTimer(int nTimerId)
{
	return Qu1etTimerMonitor::resetTimer(nTimerId);
}

int Qu1etTimerBase::killTimer(int nTimerID)
{
	return Qu1etTimerMonitor::killTimer(nTimerID);
}

void Qu1etTimerBase::killAllTimer()
{
	Qu1etTimerMonitor::killAllTimer();
}

bool Qu1etTimerBase::enableTimer(int nTimerID, bool bEnable)
{
	return Qu1etTimerMonitor::enableTimer(nTimerID, bEnable);
}

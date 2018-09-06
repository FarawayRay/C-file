#include "Qu1etTimerMonitor.h"
#include "Qu1etTimerBase.h"
#include "Qu1etTypes.h"

#ifdef QNX
#include <condition_variable>
#include <list>
#include <mutex>
#include <pthread.h>

typedef struct
{
    int nTimerID;
    int nInterval;
    bool bState; //定时器状态
    int nElapse;
    Qu1etTimerBase *pThis;  //定时器发出者
}QU1ET_TIMER_STRUCT;

typedef struct TIMER_MSG
{
    TIMER_MSG()
    :nTimerId(-1), pThis(NULL)
    {

    }
    ~TIMER_MSG()
    {
        nTimerId = -1;
        pThis = NULL;
    }
    int nTimerId;
    Qu1etTimerBase *pThis;
}QU1ET_TIMER_MSG;

static std::list<QU1ET_TIMER_STRUCT> g_timer_list;  //定时器列表
static std::list<QU1ET_TIMER_MSG>g_msg_list;  //定时器消息列表
static pthread_t g_timer_tid = -1;
static pthread_t g_msg_tid = -1;
static const int  QU1ET_TIME_PRECISION = 200*1000;  //精度100000us  0.5s
static std::condition_variable g_cv_timer;
static std::condition_variable g_cv_timer_msg;
static std::mutex m_tTimerLock;

Qu1etTimerMonitor::Qu1etTimerMonitor()
{
    g_timer_list.clear();
    g_msg_list.clear();
}

Qu1etTimerMonitor::~Qu1etTimerMonitor()
{
    g_timer_list.clear();
    g_msg_list.clear();
}

void *Qu1etTimerMonitor::timerProc(void*)
{
    LOGE("start TimerProc...");
    struct timeval tempval;
    tempval.tv_sec = 0;
    tempval.tv_usec = QU1ET_TIME_PRECISION;   //0.1s
    while( true )
    {
        if ( g_timer_list.empty() )
        {
            /*若定时器列表为空，则等待信号量*/
            std::unique_lock<std::mutex> mtx(m_tTimerLock);
            g_cv_timer.wait(mtx);
        }
        if (g_timer_list.empty())
            continue;
        pthread_testcancel();
        select(0, NULL, NULL, NULL, &tempval);  //select 阻塞
        std::unique_lock<std::mutex> mtx(m_tTimerLock);
        auto it = g_timer_list.begin();
        for( ; it != g_timer_list.end(); ++it )
        {
            if ( it->nElapse == it->nInterval && it->bState )  //定时时间周期到并且状态为true
            {
                it->nElapse = 0;  //重新计时
                QU1ET_TIMER_MSG msg;
                msg.nTimerId = it->nTimerID;
                msg.pThis = it->pThis;
                g_msg_list.push_back( msg );
                g_cv_timer_msg.notify_one();
            }
            else
            {
                it->nElapse++;  //逝去计数+1
            }
        } // end of for
    }
    return NULL;
}

void* Qu1etTimerMonitor::timerMsgProc(void*)
{
    while( true )
    {
        if ( g_msg_list.empty() )
        {
            std::unique_lock<std::mutex> mtx(m_tTimerLock);
            g_cv_timer_msg.wait(mtx);
        }
        if (g_msg_list.empty() )
            continue;

        std::list<QU1ET_TIMER_MSG>::iterator it = g_msg_list.begin();
        if ( it->pThis )
            it->pThis->onTimer( it->nTimerId );

        std::unique_lock<std::mutex> mtx(m_tTimerLock);
        g_msg_list.pop_front();
    }
    return NULL;
}

void Qu1etTimerMonitor::setTimer(int nTimerID, long nInterval, Qu1etTimerBase *pThis )
{
    static bool g_bFirstRun = true;
    if (g_bFirstRun)
    {
        g_bFirstRun = false;
        pthread_create( &g_timer_tid, NULL, timerProc, NULL );
        pthread_create( &g_msg_tid, NULL, timerMsgProc, NULL );

        pthread_detach(g_timer_tid);
        pthread_detach(g_msg_tid);
    }

    if ( !resetTimer( nTimerID ) )  //如果设置的定时器ID已经存在，则重置它否则添加新的定时器
    {
        std::unique_lock<std::mutex> mtx(m_tTimerLock);
        QU1ET_TIMER_STRUCT item = { 0 };
        item.bState = true; //默认为enable 状态
        item.nTimerID = nTimerID;
        item.nInterval = nInterval / QU1ET_TIME_PRECISION;  //精确到0.1s

        item.pThis = pThis;

        LOGE( "set timer id[%d],nInterval = %d", nTimerID, item.nInterval / 10 );
        g_timer_list.push_back( item );
        g_cv_timer.notify_one();
    }
}

bool Qu1etTimerMonitor::resetTimer(int nTimerId)
{
    std::unique_lock<std::mutex> mtx(m_tTimerLock);
    bool bRet = false;
    std::list<QU1ET_TIMER_STRUCT>::iterator it = g_timer_list.begin();
    for( ; it != g_timer_list.end(); ++it )
    {
        if ( it->nTimerID == nTimerId )
        {
            LOGI("Reset Timer id[%d]", nTimerId);
            it->nElapse = 0;
            bRet = true;
            break;  //定时器id应唯一，所以找到一个即可退出
        }
    }
    return bRet;
}

int Qu1etTimerMonitor::killTimer(int nTimerID)
{
    //std::unique_lock<std::mutex> mtx(m_tTimerLock);
    if ( g_timer_list.empty() )
    {
        return -1;
    }
    int nRet = -1;
    auto it = g_timer_list.begin();
    for( ; it != g_timer_list.end(); ++it )
    {
        if ( it->nTimerID == nTimerID )
        {
            LOGI("Kill timer id[%d]", nTimerID);
            g_timer_list.erase( it );
            nRet = 0;
            break;
        }
    }
    return nRet;
}


void Qu1etTimerMonitor::killAllTimer()
{
    std::unique_lock<std::mutex> lock(m_tTimerLock);
    std::list<QU1ET_TIMER_STRUCT>::iterator it = g_timer_list.begin();
    g_timer_list.clear();
    LOGE("Killed all timer");
}

bool Qu1etTimerMonitor::enableTimer(int nTimerID, bool bEnable)
{
    std::unique_lock<std::mutex> lock(m_tTimerLock);
    bool bRet = false;
    std::list<QU1ET_TIMER_STRUCT>::iterator it = g_timer_list.begin();
    for( ; it != g_timer_list.end(); ++it )
    {
        if ( it->nTimerID == nTimerID )
        {
            LOGI("Set Timer State id[%d], state[%d]", nTimerID, bEnable );
            it->bState = bEnable;
            bRet = true;
            break;
        }
    }
    return bRet;
}

#endif //only support qnx
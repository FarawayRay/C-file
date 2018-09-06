#pragma once

class Qu1etTimerBase {
public:
    Qu1etTimerBase();
    virtual ~Qu1etTimerBase();

    /*
     * @brief 重置定时器，并启动它
     *
     * @return 若成功重置指定定时器返回true，否则返回false
     * */
    bool resetTimer( int nTimerId );

    /*
     * @brief 设置定时器，若不调用此函数，定时器将不会工作
     *
     * @param nTimerID[IN]  定时器ID,唯一，若id已存在，函数执行动作为重置该定时器
     *
     * @param nInteraval[IN] 定时器时间间隔，单位微秒(us)
     * */
    void setTimer( int nTimerID, long nInterval, Qu1etTimerBase *pThis = nullptr );

    /*
     * @brief 销毁指定id的定时器，若指定的定时器不存在，此函数不做处理
     * */
    int killTimer( int nTimerID );

    /*
     * @brief 销毁已知所有定时器
     * */
    void killAllTimer();

    /*
     * @brief 使能定时器,若定时器失效，将不会参数该定时器的消息
     *
     * @param nTimerID 定时器id
     *
     * @param bEnable[IN]  为false时表示设置定时器失效,为true表示让定时器生效,并重新开始计数
     *
     * @return 无
     * */
    bool enableTimer( int nTimerID, bool bEnable = true );

public:
    /*
     * @brief 定时器消息回调函数
     *
     * @param nTimerID[IN] 定时器ID
     * */
    virtual void onTimer( int nTimerID ) = 0;
};
#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <QObject>
#include <thread>
#include <atomic>

class BaseThread : public QObject
{
public:
    typedef enum {
        UNINIT = 0,     // 线程尚未运行状态
        RUNNING,        // 线程运行中状态
        PAUSED,         // 线程暂停运行状态
        FINISHED,       // 线程运行完成状态 —— 正常结束
        STOPPED,        // 线程停止运行状态 —— 异常结束
        EXITED          // 线程结束状态
    } THREAD_STATE;

    BaseThread();
    virtual ~BaseThread();

    virtual void task();
    void start();   // UNINT -> RUNNING
    void pause();   // RUNNING ->PAUSED
    void resume();  // PAUSED -> RUNNING
    void finish();  // RUNNING | PAUSED -> FINISHED
    void stop();    // RUNNING | PAUSED | FINISHED -> STOPPED
    void exit();    // STOPPED -> EXITED

    bool isRunning() const;
    bool isPaused() const;
    bool isFinished() const;
    bool isStopped() const;
    bool isExited() const;

protected:
    std::thread m_thread;
    std::atomic<THREAD_STATE> m_state;
};

#endif // BASETHREAD_H

#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <thread>
#include <atomic>

class BaseThread
{
public:
    BaseThread();
    virtual ~BaseThread();

    virtual void task();
    void start();
    void stop();
    void pause();
    void resume();

protected:
    enum THREAD_STATE {
        UNINIT,
        RUNNING,
        PAUSED,
        STOPPED
    };
    std::thread m_thread;
    std::atomic<THREAD_STATE> m_state;
};

#endif // BASETHREAD_H

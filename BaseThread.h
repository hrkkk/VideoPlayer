#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <QObject>
#include <thread>
#include <atomic>

class BaseThread : public QObject
{
public:
    enum THREAD_STATE {
        UNINIT,
        RUNNING,
        PAUSED,
        STOPPED,
        FINISHED
    };

    BaseThread();
    virtual ~BaseThread();

    virtual void task();
    void start();
    void stop();
    void pause();
    void resume();
    bool isFinished() const;
    bool isPaused() const;
    bool isRunning() const;

protected:
    std::thread m_thread;
    std::atomic<THREAD_STATE> m_state;
};

#endif // BASETHREAD_H

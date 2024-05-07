#include "BaseThread.h"

BaseThread::BaseThread() : m_thread(), m_state(THREAD_STATE::UNINIT)
{}

BaseThread::~BaseThread()
{}

void BaseThread::start()
{
    if (m_state == THREAD_STATE::UNINIT) {
        m_thread = std::thread([this]() {
            task();
        });
        m_thread.detach();
        m_state = THREAD_STATE::RUNNING;
    }
}

void BaseThread::pause()
{
    if (m_state == THREAD_STATE::RUNNING) {
        m_state = THREAD_STATE::PAUSED;
    }
}

void BaseThread::resume()
{
    if (m_state == THREAD_STATE::PAUSED) {
        m_state = THREAD_STATE::RUNNING;
    }
}

void BaseThread::finish()
{
    if (m_state == THREAD_STATE::RUNNING || m_state == THREAD_STATE::PAUSED) {
        m_state = THREAD_STATE::FINISHED;
    }
}

void BaseThread::stop()
{
    if (m_state == THREAD_STATE::RUNNING || m_state == THREAD_STATE::PAUSED || m_state == THREAD_STATE::FINISHED) {
        m_state = THREAD_STATE::STOPPED;
    }
}

void BaseThread::exit()
{
    if (m_state == THREAD_STATE::STOPPED) {
        m_state = THREAD_STATE::EXITED;
    }
}

bool BaseThread::isFinished() const
{
    return m_state == THREAD_STATE::FINISHED;
}

bool BaseThread::isPaused() const
{
    return m_state == THREAD_STATE::PAUSED;
}

bool BaseThread::isRunning() const
{
    return m_state == THREAD_STATE::RUNNING;
}

bool BaseThread::isStopped() const
{
    return m_state == THREAD_STATE::STOPPED;
}

bool BaseThread::isExited() const
{
    return m_state == THREAD_STATE::EXITED;
}

void BaseThread::task()
{}

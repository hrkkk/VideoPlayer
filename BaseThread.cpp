#include "BaseThread.h"

BaseThread::BaseThread() : m_thread(), m_state(THREAD_STATE::UNINIT)
{}

BaseThread::~BaseThread()
{}

void BaseThread::start()
{
    m_thread = std::thread([this]() {
        task();
    });
    m_state = THREAD_STATE::RUNNING;
    m_thread.detach();
}

void BaseThread::stop()
{
    m_state = THREAD_STATE::STOPPED;
}

void BaseThread::pause()
{
    m_state = THREAD_STATE::PAUSED;
}

void BaseThread::resume()
{
    m_state = THREAD_STATE::RUNNING;
}

void BaseThread::task()
{

}

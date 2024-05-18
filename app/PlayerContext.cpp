#include "PlayerContext.h"

PlayerContext* PlayerContext::m_playerContext = nullptr;

PlayerContext* PlayerContext::getInstance()
{
    if (m_playerContext == nullptr) {
        m_playerContext = new PlayerContext();
    }
    return m_playerContext;
}

void PlayerContext::destroyInstance()
{
    if (m_playerContext != nullptr) {
        delete m_playerContext;
        m_playerContext = nullptr;
    }
}



void PlayerContext::setFilename(const std::string& filename)
{
    this->m_filename = filename;
}

void PlayerContext::seekToPos(double pos)
{
    this->m_doSeek = true;
    this->m_seekPosition = pos;
}

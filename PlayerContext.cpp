#include "PlayerContext.h"

PlayerContext::PlayerContext() {}

PlayerContext::~PlayerContext()
{
    Log("Player context destroyed.");
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

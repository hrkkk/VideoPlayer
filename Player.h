#ifndef PLAYER_H
#define PLAYER_H

#include <string>

#include "PlayerContext.h"

class Player
{
public:
    explicit Player(const std::string& filename);

    void setFilePath(const std::string& filename);
    void play();
    void pause();


private:
    PlayerContext m_playerCtx;
};

#endif // PLAYER_H

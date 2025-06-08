#pragma once

#include "game/GameManager.h"

class Feature
{
public:
    virtual void onEnable() {}
    virtual void onDisable() {}

    void setManager(GameManager* gameManager)
    {
        game = gameManager;
    }

protected:
    GameManager* game;
};
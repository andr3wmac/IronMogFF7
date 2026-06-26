#pragma once

#include "livemod/game/GameManager.h"
#include "core/utilities/BattleScriptBuilder.h"

#include <cstdint>

class EnemyControl
{
public:
    EnemyControl();

    void setup(GameManager* game);
    void reset();
    void draw();

private:
    GameManager* game = nullptr;
    BattleScriptOverwrite enemy1ScriptOverwrite;

    bool controlEnemy1 = false;
    bool previousControlEnemy1 = false;
    bool hasSavedEnemy1ATBRate = false;
    bool enemy1ActionSubmitted = false;
    uint16_t savedEnemy1ATBRate = 0x2022;
    uint16_t lastEnemy1ATB = 0;

    bool includeMessage = true;
    char message[64]{};
    int selectedTarget = 0;
    int selectedAttack = 0;
};

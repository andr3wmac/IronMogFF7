#pragma once

#include "game/GameManager.h"
#include <string>
#include <vector>

class MemorySearch
{
public:
    MemorySearch(GameManager* gameManager);
    ~MemorySearch();

    std::vector<uintptr_t> search(std::vector<uint8_t> searchBytes);
    std::vector<uintptr_t> searchForCloseValues(uint8_t first, uint8_t second, uintptr_t maxDistance);

private:
    GameManager* game;
    uint8_t* RAMData;
};

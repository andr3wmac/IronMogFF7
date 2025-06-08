#include "game/GameManager.h"
#include "features/DisableLimitBreaks.h"
#include "features/Permadeath.h"
#include "features/RandomizeEnemyDrops.h"
#include "utilities/MemorySearch.h"

#include <iostream>

int main() 
{ 
    std::string targetProcess = "duckstation-qt-x64-ReleaseLTCG.exe";

    GameManager manager;
    if (!manager.attachToEmulator(targetProcess))
    {
        std::cout << "Failed to attach to emulator. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Attached to emulator." << std::endl;

    //manager.addFeature(new DisableLimitBreaks());
    //manager.addFeature(new Permadeath());
    manager.addFeature(new RandomizeEnemyDrops());
    manager.run();

    return 0;
}
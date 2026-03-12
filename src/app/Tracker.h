#include "core/game/GameManager.h"

struct TrackedCharacter
{
    bool isActive = false;
    bool isPermadead = false;
};

class Tracker
{
public:
    Tracker();
    void setup(GameManager* game);
    void reset();
    void update();

    // Settings
    int attemptCounter = 0;
    bool showLogo = true;

    // Display elements
    TrackedCharacter characters[9];
    std::string inGameTime = "";
    std::string currentSong = "";
    std::string rulesSummary = "";

private:
    GameManager* game = nullptr;
};
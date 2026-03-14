#include "core/game/GameManager.h"

struct TrackedCharacter
{
    bool isActive = false;
    bool isPermadead = false;
};

enum class AttemptsDisplayMode : uint8_t
{
    Automatic = 0,
    Attempts  = 1,
    GameOvers = 2,
    Disabled  = 3
};

class Tracker
{
public:
    Tracker();
    void setup(GameManager* game);
    void reset();
    void update();

    bool showAttempts();
    bool showGameOvers();

    // Settings
    AttemptsDisplayMode attemptsDisplayMode = AttemptsDisplayMode::Automatic;
    bool showLogo = true;
    int attemptCounter = 0;
    int gameOverCounter = 0;

    // Display elements
    TrackedCharacter characters[9];
    std::string inGameTime = "";
    std::string currentSong = "";
    std::string rulesSummary = "";

private:
    void onNewGame();
    void onGameOver();

    GameManager* game = nullptr;
};
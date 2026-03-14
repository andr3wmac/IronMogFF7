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
    bool showLogo = true;
    bool showCharacters = true;
    bool showSeed = true;
    bool showTime = true;
    bool showSong = true;
    bool showRuleSummary = true;
    AttemptsDisplayMode attemptsDisplayMode = AttemptsDisplayMode::Automatic;

    // Display elements
    TrackedCharacter characters[9];
    std::string inGameTime = "";
    std::string currentSong = "";
    int attemptCounter = 0;
    int gameOverCounter = 0;
    std::string rulesSummary = "";

private:
    void onNewGame();
    void onGameOver();

    GameManager* game = nullptr;
};
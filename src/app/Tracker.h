#include "LiveModFF7Core/game/GameManager.h"
#include <atomic>

struct TrackedCharacter
{
    bool isActive = false;
    bool isPermadead = false;
    bool isRevealed = true; // False while Randomize Characters hides who they look like.
    uint8_t portrait = 0;   // Character ID of the portrait to show.
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
    bool showCharacters = true;
    bool showSeed = true;
    bool showTime = true;
    bool showSong = true;
    bool showModSummary = true;
    AttemptsDisplayMode attemptsDisplayMode = AttemptsDisplayMode::Automatic;

    // Display elements
    TrackedCharacter characters[9];
    std::string inGameTime = "";
    std::string currentSong = "";
    // Incremented by game events on the manager thread and edited from the GUI, hence atomic.
    std::atomic<int> attemptCounter = 0;
    std::atomic<int> gameOverCounter = 0;
    std::string modsSummary = "";

private:
    void onNewGame();
    void onGameOver();
    void onFieldChanged(uint16_t fieldID);
    void updateSightings(uint16_t gameMoment);

    GameManager* game = nullptr;

    // Characters seen outside the story's game moments (Yuffie's and Vincent's optional meetings).
    uint16_t sightedMask = 0;
    uint16_t lastGameMoment = 0;
    std::atomic<uint16_t> loadedFieldID = 0; // Set by the manager thread once a field has loaded.
};
#include "GameManager.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "extras/Extra.h"
#include "rules/RandomizeShops.h"
#include "rules/Rule.h"

#include <thread>
#include <chrono>

GameManager::GameManager()
    : emulator(nullptr)
{
    memset(fieldScriptExecutionTable, 0, 128);
}

GameManager::~GameManager()
{
    if (emulator != nullptr)
    {
        delete emulator;
    }
}

bool GameManager::connectToEmulator(std::string processName)
{
    emulator = Emulator::getEmulatorFromProcessName(processName);
    if (emulator == nullptr)
    {
        return false;
    }

    return emulator->connect(processName);
}

bool GameManager::connectToEmulator(std::string processName, uintptr_t memoryAddress)
{
    emulator = Emulator::getEmulatorCustom(processName, memoryAddress);
    if (emulator == nullptr)
    {
        return false;
    }

    return emulator->connect(processName);
}

std::string GameManager::readString(uintptr_t offset, uint32_t length)
{
    std::vector<uint8_t> strData;
    strData.resize(length);
    emulator->read(offset, strData.data(), length);
    return GameData::decodeString(strData);
}

size_t GameManager::writeString(uintptr_t offset, uint32_t length, const std::string& string, bool centerAlign)
{
    std::vector<uint8_t> strData = GameData::encodeString(string);

    std::vector<uint8_t> finalStrData;
    finalStrData.resize(length, 0x00); // Fill with 0x00 by default

    size_t strLen = std::min<size_t>(strData.size(), length);
    size_t padding = centerAlign ? (length - strLen) / 2 : 0;

    for (size_t i = 0; i < strLen; ++i)
    {
        finalStrData[padding + i] = strData[i];
    }

    emulator->write(offset, finalStrData.data(), length);
    return strLen;
}

bool GameManager::isRuleEnabled(std::string ruleName)
{
    for (Rule* rule : Rule::getList())
    {
        if (rule->enabled && rule->name == ruleName)
        {
            return true;
        }
    }

    return false;
}

Rule* GameManager::getRule(std::string ruleName)
{
    for (Rule* rule : Rule::getList())
    {
        if (rule->enabled && rule->name == ruleName)
        {
            return rule;
        }
    }

    return nullptr;
}

bool GameManager::isExtraEnabled(std::string extraName)
{
    for (Extra* extra : Extra::getList())
    {
        if (extra->enabled && extra->name == extraName)
        {
            return true;
        }
    }

    return false;
}

Extra* GameManager::getExtra(std::string extraName)
{
    for (Extra* extra : Extra::getList())
    {
        if (extra->enabled && extra->name == extraName)
        {
            return extra;
        }
    }

    return nullptr;
}

std::string GameManager::getSettingsSummary()
{
    std::map<std::string, std::vector<std::string>> groups;

    groups["No"] = {};
    groups["Randomized"] = {};
    groups["Multipliers"] = {};
    groups["Unique"] = {};
    groups["BanItems"] = {};
    groups["BanMateria"] = {};

    for (Rule* rule : Rule::getList())
    {
        if (!rule->enabled)
        {
            continue;
        }

        std::vector<std::string> negations = rule->describe(RuleDescripionType::Negation);
        groups["No"].insert(groups["No"].end(), negations.begin(), negations.end());

        std::vector<std::string> randomized = rule->describe(RuleDescripionType::Randomized);
        groups["Randomized"].insert(groups["Randomized"].end(), randomized.begin(), randomized.end());

        std::vector<std::string> multipliers = rule->describe(RuleDescripionType::Multiplier);
        groups["Multipliers"].insert(groups["Multipliers"].end(), multipliers.begin(), multipliers.end());

        std::vector<std::string> uniques = rule->describe(RuleDescripionType::Unique);
        groups["Unique"].insert(groups["Unique"].end(), uniques.begin(), uniques.end());

        std::vector<std::string> bannedItems = rule->describe(RuleDescripionType::BanItems);
        groups["BanItems"].insert(groups["BanItems"].end(), bannedItems.begin(), bannedItems.end());

        std::vector<std::string> bannedMateria = rule->describe(RuleDescripionType::BanMateria);
        groups["BanMateria"].insert(groups["BanMateria"].end(), bannedMateria.begin(), bannedMateria.end());
    }

    for (Extra* extra : Extra::getList())
    {
        if (!extra->enabled)
        {
            continue;
        }

        std::vector<std::string> randomized = extra->describe(ExtraDescripionType::Randomized);
        groups["Randomized"].insert(groups["Randomized"].end(), randomized.begin(), randomized.end());
    }

    std::stringstream ss;
    if (groups["BanItems"].size() > 0 || groups["BanMateria"].size() > 0)
    {
        bool banItems = groups["BanItems"].size() > 0;
        bool banMateria = groups["BanMateria"].size() > 0;

        if (banItems)
        {
            auto& subjects = groups["BanItems"];
            std::sort(subjects.begin(), subjects.end());

            ss << "- Ban ";
            for (size_t i = 0; i < subjects.size(); ++i)
            {
                std::string subject = subjects[i];
                std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
                ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
            }
            if (banMateria)
            {
                ss << ".";
            }
            else 
            {
                ss << ".\n";
            }
        }

        if (banMateria)
        {
            auto& subjects = groups["BanMateria"];
            std::sort(subjects.begin(), subjects.end());

            if (banItems)
            {
                ss << " Ban ";
            }
            else 
            {
                ss << "- Ban ";
            }
            
            for (size_t i = 0; i < subjects.size(); ++i)
            {
                std::string subject = subjects[i];
                std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
                ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
            }
            ss << " materia.\n";
        }
    }

    if (groups["No"].size() > 0)
    { 
        auto& subjects = groups["No"];
        std::sort(subjects.begin(), subjects.end());

        ss << "- No ";
        for (size_t i = 0; i < subjects.size(); ++i)
        {
            std::string subject = subjects[i];
            std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
            ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " or " : ", "));
        }
        ss << ".\n";
    }

    if (groups["Randomized"].size() > 0)
    {
        auto& subjects = groups["Randomized"];
        std::sort(subjects.begin(), subjects.end());

        ss << "- Randomized ";
        for (size_t i = 0; i < subjects.size(); ++i)
        {
            std::string subject = subjects[i];
            std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
            ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
        }
        ss << ".\n";
    }

    if (groups["Multipliers"].size() > 0)
    {
        auto& subjects = groups["Multipliers"];

        ss << "- ";
        for (size_t i = 0; i < subjects.size(); ++i)
        {
            std::string subject = subjects[i];
            std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
            ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
        }
        ss << ".\n";
    }

    if (groups["Unique"].size() > 0)
    {
        auto& subjects = groups["Unique"];

        for (size_t i = 0; i < subjects.size(); ++i)
        {
            ss << "- " << subjects[i] << ".\n";
        }
    }

    return ss.str();
}

void GameManager::setup(GameVersion version, uint32_t inputSeed)
{
    // Prepare game data based on version
    gameVersion = version;
    if (gameVersion == GameVersion::PlayStationUS)
    {
        LOG("Loading game data for PlayStation US (Original)");
    }
    if (gameVersion == GameVersion::PlayStationUS_CSR)
    {
        LOG("Loading game data for PlayStation US (CSR)");
    }
    GameData::clearGameData();
    GameData::loadGameData(gameVersion, gameDisc);

    // Note: seed may change after loading a save file, so its important to not utilize it in rule setup.
    seed = inputSeed;

    // Setup modules
    battle.setup(this);
    field.setup(this);
    menu.setup(this);
    world.setup(this);

    for (Rule* rule : Rule::getList())
    {
        if (!rule->enabled)
        {
            continue;
        }
        rule->setManager(this);
        rule->setup();
    }

    for (Extra* extra : Extra::getList())
    {
        if (!extra->enabled)
        {
            continue;
        }
        extra->setManager(this);
        extra->setup();
    }
}

void GameManager::loadSaveData()
{
    const uint8_t saveDataVersion = 0;

    uint16_t ironMogID = read<uint16_t>(SavemapOffsets::IronMogSave);

    // 49 and 4D are the letters IM for Iron Mog
    if (ironMogID == 0x4D49)
    {
        // Load existing save data.
        seed = read<uint32_t>(SavemapOffsets::IronMogSeed);

        std::string seedString = Utilities::seedToHexString(seed);
        LOG("Loaded seed from save file: %s", seedString.c_str());
    }
    else 
    {
        clearSaveData();

        // Write header and seed into save data area.
        write<uint16_t>(SavemapOffsets::IronMogSave, 0x4D49);
        write<uint8_t>(SavemapOffsets::IronMogVersion, saveDataVersion);

        // Write seed
        write<uint32_t>(SavemapOffsets::IronMogSeed, seed);
    }
}

void GameManager::clearSaveData()
{
    // Zero out the area.
    for (int i = 0; i < 8; ++i)
    {
        write<uint32_t>(SavemapOffsets::IronMogSave + (i * 4), 0);
    }
}

GameManager::GameState GameManager::getState()
{
    uint8_t gameModule = read<uint8_t>(GameOffsets::CurrentModule);

    // Not really sure what this is actually meant for but its consistently
    // 27 when on the main menu, and 26 when on game over screen.
    uint8_t screenState = read<uint8_t>(0xEFBB1);

    if (gameModule == 0 && screenState == 0)
    {
        return GameState::BootScreen;
    }

    if (gameModule == 0 && screenState == 27)
    {
        // Fresh boot, main menu.
        return GameState::MainMenuCold;
    }

    // Mini games do not respect the screenState variable so they
    // are excluded from this check.
    if (gameModule > 0 && gameModule < 6 && screenState == 27)
    {
        // Main menu after a soft reset or game over.
        return GameState::MainMenuWarm;
    }

    return GameState::InGame;
}

bool GameManager::update()
{
    double currentTime = Utilities::getTimeMS();
    
    // If read/write errors have occurred then connection has been broken.
    if (emulator->pollErrors())
    {
        return false;
    }

    GameState state = getState();
    {
        if (lastGameState == GameState::InGame && state != GameState::InGame)
        {
            // Clearing save data prevents stale state getting stuck from a game over.
            clearSaveData();
            AudioManager::pauseMusic();
        }

        if (lastGameState != GameState::InGame && state == GameState::InGame)
        {
            loadSaveData();
            onStart.invoke();
            updatesSinceFrame = 0;
            lastGameMoment = 0;
            justEnteredGame = true;
        }

        lastGameState = state;
    }

    // Only perform updates when we're actually in the game.
    if (state != GameState::InGame)
    {
        return true;
    }

    // If the disc changes we need to reload the data since CSR has 
    // different data for different discs.
    uint8_t currentDisc = read<uint8_t>(GameOffsets::DiscNumber);
    if (currentDisc != gameDisc)
    {
        GameData::clearGameData();
        GameData::loadGameData(gameVersion, currentDisc);
        LOG("Disc changed from %d to %d.", gameDisc, currentDisc);
        gameDisc = currentDisc;
    }

    // If over 30 updates have passed without frame number advancing it should be at least 300ms 
    // passing without a frame update which is enough time to conclude the emulator was paused.
    if (updatesSinceFrame > 30 && !emulatorPaused)
    {
        emulatorPaused = true;
        onEmulatorPaused.invoke();
        LOG("Emulator paused.");
    }
    updatesSinceFrame++;

    // Update the field script execution table.
    read(FieldScriptOffsets::ExecutionTable, 128, (uint8_t*)(&fieldScriptExecutionTable[0]));
    
    // Check if the active module has changed.
    uint8_t newGameModule = read<uint8_t>(GameOffsets::CurrentModule);
    if (newGameModule != gameModule)
    {
        if (gameModule == GameModule::Battle && newGameModule == GameModule::Field)
        {
            // Check if we're playing game over music
            uint16_t musicID = read<uint16_t>(GameOffsets::MusicID);
            if (musicID == 59)
            {
                waitingForGameOver = true;
            }
        }

        // Game module changed.
        gameModule = newGameModule;
        onModuleChanged.invoke(gameModule);
    }

    if (justEnteredGame)
    {
        uint32_t igt = read<uint32_t>(GameOffsets::InGameTime);
        if (igt > 2)
        {
            uint16_t fieldID = read<uint16_t>(GameOffsets::FieldID);
            if (fieldID == 116)
            {
                LOG("New game started.");
                onNewGame.invoke();
            }
            justEnteredGame = false;
        }
    }

    uint16_t currentGameMoment = getGameMoment();
    if (currentGameMoment != lastGameMoment)
    {
        onGameMomentChanged.invoke(currentGameMoment);
        lastGameMoment = currentGameMoment;
    }

    // Check if we are on game over screen
    if (waitingForGameOver)
    {
        uint8_t screenState = read<uint8_t>(0xEFBB1);
        uint16_t musicID = read<uint16_t>(GameOffsets::MusicID);

        // Screen state of 26 confirms we actually went to the game over screen.
        if (screenState == 26 && musicID == 59)
        {
            LOG("Game over.");
            onGameOver.invoke();
            waitingForGameOver = false;
        }
        else if (musicID != 59)
        {
            waitingForGameOver = false;
        }
    }
    
    // Broadcast update event for anything that needs it
    bool justConnected = field.getFieldID() == 0 || framesSinceReload == 0;
    onUpdate.invoke(justConnected);

    uint32_t newFrameNumber = read<uint32_t>(GameOffsets::FrameNumber);

    // A jump in frame number likely indicates a load game or load save state.
    framesSinceReload++;
    int frameDifference = std::abs((int)newFrameNumber - (int)frameNumber);
    if (frameDifference > 30 && framesSinceReload > 30)
    {
        LOG("Load detected, reloading rules %d - %d = %d", newFrameNumber, frameNumber, frameDifference);
        loadSaveData();
        onStart.invoke();
        framesSinceReload = 0;
    }

    // Detect change in frame number and trigger event
    if (newFrameNumber != frameNumber)
    {
        frameNumber = newFrameNumber;
        updatesSinceFrame = 0;

        if (emulatorPaused)
        {
            LOG("Emulator resumed.");
            onEmulatorResumed.invoke();
            emulatorPaused = false;
        }
        
        onFrame.invoke(newFrameNumber);
    }

    lastUpdateDuration = Utilities::getTimeMS() - currentTime;
    return true;
}

void GameManager::setDifficultyScale(float newScale)
{
    if (difficultyScale == newScale)
    {
        return;
    }

    difficultyScale = Utilities::clamp(newScale, 0.0f, 1.0f);
    onDifficultyScaleChanged.invoke(difficultyScale);
}

std::array<uint8_t, 3> GameManager::getPartyIDs()
{
    std::array<uint8_t, 3> results = { 0xFF, 0xFF, 0xFF };

    results[0] = read<uint8_t>(GameOffsets::PartyIDList + 0);
    results[1] = read<uint8_t>(GameOffsets::PartyIDList + 1);
    results[2] = read<uint8_t>(GameOffsets::PartyIDList + 2);

    return results;
}

std::array<uint16_t, 320> GameManager::getPartyInventory()
{
    std::array<uint16_t, 320> results;
    emulator->read(GameOffsets::Inventory, results.data(), sizeof(uint16_t) * 320);
    return results;
}

void GameManager::setInventorySlot(uint32_t slotIndex, uint16_t itemID, uint8_t quantity)
{
    if (slotIndex >= 320)
    {
        return;
    }

    uint16_t data = (quantity << 9) | (itemID & 0x1FF);
    emulator->write(GameOffsets::Inventory + (sizeof(uint16_t) * slotIndex), &data, sizeof(uint16_t));
}

std::array<uint32_t, 200> GameManager::getPartyMateria()
{
    std::array<uint32_t, 200> results;
    emulator->read(GameOffsets::MateriaInventory, results.data(), sizeof(uint32_t) * 200);
    return results;
}

uint16_t GameManager::getGameMoment()
{
    return read<uint16_t>(GameOffsets::GameMoment);
}

bool GameManager::inMenu()
{
    return gameModule == GameModule::Menu;
}

std::string GameManager::getWindowText(uint8_t index)
{
    if (getGameModule() != GameModule::Field)
    {
        return "";
    }

    return readString(getWindowTextOffset(index), 256);
}

std::pair<BattleScene*, BattleFormation*> GameManager::getBattleFormation(uint16_t formationID)
{
    for (auto& [id, scene] : GameData::battleScenes)
    {
        for (BattleFormation& formation : scene.formations)
        {
            if (formation.id == formationID)
            {
                return { &scene, &formation };
            }
        }
    }

    return { nullptr, nullptr };
}

std::pair<BattleScene*, BattleFormation*> GameManager::getBattleFormation()
{
    if (getGameModule() != GameModule::Battle)
    {
        return { nullptr, nullptr };
    }

    uint16_t formationID = read<uint16_t>(BattleOffsets::ActiveFormationID);
    return getBattleFormation(formationID);
}

template <typename T>
void multiplyStat(GameManager* game, uintptr_t offset, float multiplier)
{
    if (multiplier == 1.0f)
    {
        return;
    }

    T stat = game->read<T>(offset);
    stat = Utilities::clampTo<T>(stat * multiplier);
    game->write<T>(offset, stat);
}

void multiplyDefense(GameManager* game, uintptr_t offset, float multiplier, bool softCap)
{
    if (multiplier == 1.0f)
    {
        return;
    }

    uint16_t stat = game->read<uint16_t>(offset);
    if (softCap)
    {
        uint16_t orig = stat;
        uint16_t naiveScaling = Utilities::clampTo<uint16_t>(stat * multiplier);
        uint16_t halfGap = Utilities::clampTo<uint16_t>(512 - (512 - stat) * std::powf(0.5f, multiplier - 1.0f));
        stat = std::min(naiveScaling, halfGap);
        LOG("Applied defense soft cap: %d = %d or %d. Chose: %d", orig, naiveScaling, halfGap, stat);
    }
    else 
    {
        stat = Utilities::clampTo<uint16_t>(stat * multiplier);
    }

    game->write<uint16_t>(offset, stat);
}

void GameManager::applyBattleStatMultiplier(uintptr_t battleCharOffset, StatMultiplierSet& multiplierSet, bool defenseSoftCap)
{
    if (getGameModule() != GameModule::Battle)
    {
        LOG("Could not apply battle stat multiplier outside of battle.");
        return;
    }

    multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::CurrentHP, multiplierSet.currentHP);
    multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::MaxHP, multiplierSet.maxHP);
    multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::CurrentMP, multiplierSet.currentMP);
    multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::MaxMP, multiplierSet.maxMP);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Strength, multiplierSet.strength);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Magic, multiplierSet.magic);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Evade, multiplierSet.evade);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Speed, multiplierSet.speed);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Luck, multiplierSet.luck);
    multiplyDefense(this, battleCharOffset + BattleOffsets::Defense, multiplierSet.defense, defenseSoftCap);
    multiplyDefense(this, battleCharOffset + BattleOffsets::MDefense, multiplierSet.mDefense, defenseSoftCap);
}

void GameManager::applyBattleStatMultiplier(uintptr_t battleCharOffset, float multiplier, bool applyToHP, bool applyToMP, bool applyToStats, bool defenseSoftCap)
{
    if (getGameModule() != GameModule::Battle)
    {
        LOG("Could not apply battle stat multiplier outside of battle.");
        return;
    }

    if (applyToHP)
    {
        multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::CurrentHP, multiplier);
        multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::MaxHP, multiplier);
    }

    if (applyToMP)
    {
        multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::CurrentMP, multiplier);
        multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::MaxMP, multiplier);
    }

    if (applyToStats)
    {
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Strength, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Magic, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Evade, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Speed, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Luck, multiplier);
        multiplyDefense(this, battleCharOffset + BattleOffsets::Defense, multiplier, defenseSoftCap);
        multiplyDefense(this, battleCharOffset + BattleOffsets::MDefense, multiplier, defenseSoftCap);
    }
}
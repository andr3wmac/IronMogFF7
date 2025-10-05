#include "RandomizeESkills.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"

#include <imgui.h>
#include <random>
#include <set>
#include <unordered_set>

REGISTER_RULE("Randomize E.Skills", RandomizeESkills)

// The concept here is when we enter battle we find any enemy skill materia on any characters
// and track its value. When we exit the fight we look for any newly flipped bits, meaning a 
// new enemy skill was learned during the fight, and then we take its index and selecte from
// a randomized mapping and flip that bit instead, resulting in a different e.skill learned.

// We also monitor each player's e.skill menu list during battle and disable a newly learned
// eskill and enable the randomly selected one.

// TODO: it would be cool if we could change the text thats displayed when the skill is learned.

void RandomizeESkills::setup()
{
    BIND_EVENT(game->onStart, RandomizeESkills::onStart);
    BIND_EVENT(game->onBattleEnter, RandomizeESkills::onBattleEnter);
    BIND_EVENT(game->onBattleExit, RandomizeESkills::onBattleExit);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeESkills::onFrame);
}

void RandomizeESkills::onDebugGUI()
{
    if (game->getGameModule() != GameModule::Battle)
    {
        ImGui::Text("Not currently in battle.");
        return;
    }

    for (int p = 0; p < 3; ++p)
    {
        std::string playerText = "Player " + std::to_string(p);
        ImGui::Text(playerText.c_str());

        for (int i = 0; i < 24; ++i)
        {
            uintptr_t offset = PlayerOffsets::Players[p] + PlayerOffsets::EnemySkillMenu + (i * 8);
            uint64_t curValue = game->read<uint64_t>(offset);
            std::string eSkillText = " " + std::to_string(curValue);
            ImGui::Text(eSkillText.c_str());
        }
    }
}

void RandomizeESkills::onStart()
{
    battleEntered = false;
    trackedPlayers.clear();

    // Generate a shuffled remapping of e.skills based on game seed.
    eSkillMapping.resize(24);
    for (int i = 0; i < 24; ++i) 
    {
        eSkillMapping[i] = i;
    }

    std::mt19937 rng(game->getSeed());
    std::shuffle(eSkillMapping.begin(), eSkillMapping.end(), rng);
}

void RandomizeESkills::onBattleEnter()
{
    trackedPlayers.clear();

    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    for (int p = 0; p < 3; ++p)
    {
        if (partyIDs[p] == 0xFF)
        {
            continue;
        }

        uintptr_t characterOffset = getCharacterDataOffset(partyIDs[p]);
        bool hasESkillMateria = false;

        TrackedPlayer player;
        player.index = p;

        // Weapon Materia Slots
        for (int i = 0; i < 8; ++i)
        {
            uintptr_t materiaOffset = characterOffset + CharacterDataOffsets::WeaponMateria[i];
            uint32_t materiaID = game->read<uint32_t>(materiaOffset);

            // Check if its Enemy Skill Materia
            if ((materiaID & 0xFF) == 0x2C)
            {
                player.trackedMateria.push_back({ materiaOffset, materiaID });
                hasESkillMateria = true;
            }
        }

        // Armor Materia Slots
        for (int i = 0; i < 8; ++i)
        {
            uintptr_t materiaOffset = characterOffset + CharacterDataOffsets::ArmorMateria[i];
            uint32_t materiaID = game->read<uint32_t>(materiaOffset);

            // Check if its Enemy Skill Materia
            if ((materiaID & 0xFF) == 0x2C)
            {
                player.trackedMateria.push_back({ materiaOffset, materiaID });
                hasESkillMateria = true;
            }
        }

        if (hasESkillMateria)
        {
            trackedPlayers.push_back(player);
        }
    }

    battleEntered = true;
}

std::vector<int> getFlippedBits(uint32_t before, uint32_t after) 
{
    // Mask out lowest 8 bits (always 0x2C)
    uint32_t beforeMasked = before & 0xFFFFFF00;
    uint32_t afterMasked = after & 0xFFFFFF00;

    // XOR to find which bits changed
    uint32_t flipped = beforeMasked ^ afterMasked;

    std::vector<int> flippedIndices;
    for (int i = 8; i < 32; ++i) 
    { 
        if (flipped & (1 << i)) 
        {
            flippedIndices.push_back(i - 8); // index relative to the 24-bit payload
        }
    }

    return flippedIndices;
}

void RandomizeESkills::onBattleExit()
{
    for (TrackedPlayer& player : trackedPlayers)
    {
        for (TrackedMateria& mat : player.trackedMateria)
        {
            uint32_t currentMateriaID = game->read<uint32_t>(mat.offset);
            if (currentMateriaID != mat.materiaID)
            {
                std::vector<int> learnedESkills = getFlippedBits(mat.materiaID, currentMateriaID);
                uint32_t newMateriaID = mat.materiaID;

                for (int i = 0; i < learnedESkills.size(); ++i)
                {
                    int randomizedESkill = eSkillMapping[learnedESkills[i]];
                    int bitPosition = randomizedESkill + 8;

                    newMateriaID ^= (1u << bitPosition);

                    LOG("Randomized Enemy Skill from %d to %d", learnedESkills[i], randomizedESkill);
                }

                game->write<uint32_t>(mat.offset, newMateriaID);
            }
        }
    }

    trackedPlayers.clear();
    battleEntered = false;
}

void RandomizeESkills::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Battle || !battleEntered)
    {
        return;
    }

    // Loop through each player and each e.skill to see if its changed since last frame.
    for (TrackedPlayer& player : trackedPlayers)
    {
        if (!player.menuLoaded)
        {
            player.menuLoaded = isESkillMenuLoaded(player);
            if (!player.menuLoaded)
            {
                continue;
            }

            LOG("Player %d ESkill menu fully loaded.", player.index);
        }

        for (int i = 0; i < 24; ++i)
        {
            uintptr_t offset = PlayerOffsets::Players[player.index] + PlayerOffsets::EnemySkillMenu + (i * 8);
            uint64_t curValue = game->read<uint64_t>(offset);
            uint64_t prevValue = player.previousESkillValues[i];

            if (curValue != prevValue && curValue == GameData::eSkills[i].battleData)
            {
                // Disable newly learned eskill
                setESkillBattleMenu(player, i, false);

                // Enable randomized one
                int randomizedESkill = eSkillMapping[i];
                setESkillBattleMenu(player, randomizedESkill, true);

                LOG("Randomized Enemy Skill from %d to %d", i, randomizedESkill);
            }
        }
    }
}

bool RandomizeESkills::isESkillMenuLoaded(TrackedPlayer& player)
{
    std::unordered_set<uint64_t> validESkillValues;
    for (int i = 0; i < 24; ++i) 
    {
        validESkillValues.insert(GameData::eSkills[i].battleData);
    }
    validESkillValues.insert(ESKILL_EMPTY);

    int verifiedValues = 0;
    for (int i = 0; i < 24; ++i)
    {
        uintptr_t offset = PlayerOffsets::Players[player.index] + PlayerOffsets::EnemySkillMenu + (i * 8);
        uint64_t curValue = game->read<uint64_t>(offset);

        if (validESkillValues.count(curValue) > 0)
        {
            player.previousESkillValues[i] = curValue;
            verifiedValues++;
        }
    }

    return verifiedValues == 24;
}

void RandomizeESkills::setESkillBattleMenu(TrackedPlayer& player, int eSkillIndex, bool enabled)
{
    uintptr_t offset = PlayerOffsets::Players[player.index] + PlayerOffsets::EnemySkillMenu + (eSkillIndex * 8);

    if (enabled)
    {
        game->write<uint64_t>(offset, GameData::eSkills[eSkillIndex].battleData);
        player.previousESkillValues[eSkillIndex] = GameData::eSkills[eSkillIndex].battleData;
    }
    else 
    {
        game->write<uint64_t>(offset, ESKILL_EMPTY);
        player.previousESkillValues[eSkillIndex] = ESKILL_EMPTY;
    }
}
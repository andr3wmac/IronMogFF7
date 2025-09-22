#include "RandomizeESkills.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include <random>

REGISTER_RULE("Randomize E.Skills", RandomizeESkills)

// The concept here is when we enter battle we find any enemy skill materia on any characters
// and track its value. When we exit the fight we look for any newly flipped bits, meaning a 
// new enemy skill was learned during the fight, and then we take its index and selecte from
// a randomized mapping and flip that bit instead, resulting in a different e.skill learned.

// We also monitor each player's e.skill menu list during battle and disable a newly learned
// eskill and enable the randomly selected one.

// TODO: it would be cool if we could change the text thats displayed when the skill is learned.

// Value found in each empty eskill menu slot
#define ESKILL_EMPTY 562949953421567

void RandomizeESkills::setup()
{
    BIND_EVENT(game->onStart, RandomizeESkills::onStart);
    BIND_EVENT(game->onBattleEnter, RandomizeESkills::onBattleEnter);
    BIND_EVENT(game->onBattleExit, RandomizeESkills::onBattleExit);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeESkills::onFrame);
}

void RandomizeESkills::onStart()
{
    battleEntered = false;
    trackedMateria.clear();

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
    trackedMateria.clear();

    for (int c = 0; c < 9; ++c)
    {
        uintptr_t characterOffset = CharacterDataOffsets::Characters[c];

        // Weapon Materia Slots
        for (int i = 0; i < 8; ++i)
        {
            uintptr_t materiaOffset = characterOffset + CharacterDataOffsets::WeaponMateria[i];
            uint32_t materiaID = game->read<uint32_t>(materiaOffset);

            // Check if its Enemy Skill Materia
            if ((materiaID & 0xFF) == 0x2C)
            {
                trackedMateria.push_back({ materiaOffset, materiaID });
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
                trackedMateria.push_back({ materiaOffset, materiaID });
            }
        }
    }

    // Read current state of e.skill values when entering the battle.
    for (int p = 0; p < 3; ++p)
    {
        for (int i = 0; i < 24; ++i)
        {
            uintptr_t offset = PlayerOffsets::Players[p] + PlayerOffsets::EnemySkillMenu + (8 * i);
            previousESkillValues[(p * 24) + i] = game->read<uint64_t>(offset);
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
    for (int i = 0; i < trackedMateria.size(); ++i)
    {
        TrackedMateria& mat = trackedMateria[i];
        uint32_t currentMateriaID = game->read<uint32_t>(mat.offset);

        if (currentMateriaID != mat.materiaID)
        {
            std::vector<int> learnedESkills = getFlippedBits(mat.materiaID, currentMateriaID);
            uint32_t newMateriaID = mat.materiaID;

            for (int j = 0; j < learnedESkills.size(); ++j)
            {
                int randomizedESkill = eSkillMapping[learnedESkills[j]];
                int bitPosition = randomizedESkill + 8;

                newMateriaID ^= (1u << bitPosition);

                LOG("Randomized Enemy Skill from %d to %d", learnedESkills[j], randomizedESkill);
            }

            game->write<uint32_t>(mat.offset, newMateriaID);
        }
    }

    trackedMateria.clear();
    battleEntered = false;
}

void RandomizeESkills::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Battle || !battleEntered)
    {
        return;
    }

    // Loop through each player and each e.skill to see if its changed since last frame.
    for (int p = 0; p < 3; ++p)
    {
        for (int i = 0; i < 24; ++i)
        {
            uintptr_t offset = PlayerOffsets::Players[p] + PlayerOffsets::EnemySkillMenu + (i * 8);
            uint64_t curValue = game->read<uint64_t>(offset);
            uint64_t prevValue = previousESkillValues[(p * 24) + i];

            if (curValue != prevValue && curValue == GameData::eSkills[i].battleData)
            {
                // Disable newly learned eskill
                setESkillBattleMenu(p, i, false);

                // Enable randomized one
                int randomizedESkill = eSkillMapping[i];
                setESkillBattleMenu(p, randomizedESkill, true);

                LOG("Randomized Enemy Skill from %d to %d", i, randomizedESkill);
            }
        }
    }
}

void RandomizeESkills::setESkillBattleMenu(int playerIndex, int eSkillIndex, bool enabled)
{
    uintptr_t offset = PlayerOffsets::Players[playerIndex] + PlayerOffsets::EnemySkillMenu + (eSkillIndex * 8);

    if (enabled)
    {
        game->write<uint64_t>(offset, GameData::eSkills[eSkillIndex].battleData);
        previousESkillValues[(playerIndex * 24) + eSkillIndex] = GameData::eSkills[eSkillIndex].battleData;
    }
    else 
    {
        game->write<uint64_t>(offset, ESKILL_EMPTY);
        previousESkillValues[(playerIndex * 24) + eSkillIndex] = ESKILL_EMPTY;
    }
}
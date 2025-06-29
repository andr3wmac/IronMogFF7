#include "RandomizeESkills.h"
#include "game/MemoryOffsets.h"
#include "utilities/Logging.h"
#include <random>

REGISTER_RULE("Randomize E.Skills", RandomizeESkills)

// The concept here is when we enter battle we find any enemy skill materia on any characters
// and track its value. When we exit the fight we look for any newly flipped bits, meaning a 
// new enemy skill was learned during the fight, and then we take its index and selecte from
// a randomized mapping and flip that bit instead, resulting in a different e.skill learned.

// TODO: I'd really like to see this work in battle so you don't end up being able to use the
// unrandomized enemy skill in that fight. Can't seem to locate how the menu is controlled
// while in battle though.

void RandomizeESkills::setup()
{
    BIND_EVENT(game->onStart, RandomizeESkills::onStart);
    BIND_EVENT(game->onBattleEnter, RandomizeESkills::onBattleEnter);
    BIND_EVENT(game->onBattleExit, RandomizeESkills::onBattleExit);
}

void RandomizeESkills::onStart()
{
    // Generate a shuffled remapping of eskills based on game seed.

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

        // Armor Material Slots
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
}
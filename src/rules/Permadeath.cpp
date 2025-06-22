#include "Permadeath.h"
#include "game/MemoryOffsets.h"
#include "utilities/Flags.h"
#include "utilities/Logging.h"

REGISTER_RULE("Permadeath", Permadeath)

void Permadeath::onStart()
{
    game->onFrame.AddListener(std::bind(&Permadeath::onFrame, this, std::placeholders::_1));
}

void Permadeath::onFrame(uint32_t frameNumber)
{
    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    for (int i = 0; i < 3; ++i)
    {
        uint8_t id = partyIDs[i];
        if (id == 0xFF)
        {
            continue;
        }

        uintptr_t characterOffset = getCharacterDataOffset(id);

        // Check if character died
        if (deadCharacterIDs.count(id) == 0)
        {
            bool isDead = false;

            if (game->inBattle())
            {
                Flags<uint16_t> statusFlags = game->read<uint16_t>(BattleCharacterOffsets::Allies[i] + BattleCharacterOffsets::Status);
                isDead = statusFlags.isSet(StatusFlags::Dead);
            }
            else 
            {
                uint16_t currentHP = game->read<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP);
                isDead = (currentHP == 0);
            }

            if (isDead)
            {
                deadCharacterIDs.insert(id);
                LOG("Character has died: %d", id);
            }
        }

        // Force death if the character is in our dead characters list
        if (deadCharacterIDs.count(id) > 0)
        {
            // Force HP to 0
            game->write<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP, 0);

            if (game->inBattle())
            {
                game->write<uint16_t>(PlayerOffsets::Players[i] + PlayerOffsets::CurrentHP, 0);
                game->write<uint16_t>(BattleCharacterOffsets::Allies[i] + BattleCharacterOffsets::Status, StatusFlags::Dead);
            }
        }
    }
}
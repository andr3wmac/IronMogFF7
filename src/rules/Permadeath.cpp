#include "Permadeath.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"

REGISTER_RULE("Permadeath", Permadeath)

void Permadeath::setup()
{
    BIND_EVENT(game->onStart, Permadeath::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, Permadeath::onFrame);

    // Kalm Flashback
    {
        PermadeathExemption& kalmExemption = exemptions.emplace_back();
        kalmExemption.maxGameMoment = 384;
        kalmExemption.fieldIDs.insert(277);
        kalmExemption.fieldIDs.insert(278);
        for (int i = 311; i <= 321; ++i)
        {
            kalmExemption.fieldIDs.insert(i);
        }
    }

    // Golden Saucer Arena
    {
        PermadeathExemption& saucerArena = exemptions.emplace_back();
        saucerArena.fieldIDs.insert(502);
    }
}

void Permadeath::onStart()
{
    deadCharacterIDs.clear();
}

void Permadeath::onFrame(uint32_t frameNumber)
{
    uint16_t fieldID = game->getFieldID();
    if (isExempt(fieldID))
    {
        // We don't enforce permadeath in scripted scenes where deaths can occur.
        return;
    }

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

bool Permadeath::isExempt(uint16_t fieldID)
{
    uint16_t gameMoment = game->getGameMoment();
    for (int i = 0; i < exemptions.size(); ++i)
    {
        PermadeathExemption& exemption = exemptions[i];
        if (gameMoment < exemption.minGameMoment || gameMoment > exemption.maxGameMoment)
        {
            continue;
        }

        if (exemption.fieldIDs.count(fieldID) > 0)
        {
            return true;
        }
    }

    return false;
}
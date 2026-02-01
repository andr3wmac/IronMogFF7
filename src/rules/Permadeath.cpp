#include "Permadeath.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>

REGISTER_RULE(Permadeath, "Permadeath", "If a character dies, they cannot be revived and will remain dead for the rest of the playthrough.")

#define RUFUS_FIELD_ID 268
#define DYNE_FIELD_ID 480
#define CLOUD_ID 0
#define BARRET_ID 1

void Permadeath::setup()
{
    BIND_EVENT(game->onStart, Permadeath::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, Permadeath::onFrame);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, Permadeath::onFieldChanged);
    BIND_EVENT(game->onBattleExit, Permadeath::onBattleExit);

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

    // Fort Condor Battle
    {
        PermadeathExemption& fortCondorBattle = exemptions.emplace_back();
        fortCondorBattle.fieldIDs.insert(354);
        fortCondorBattle.fieldIDs.insert(356);
    }

    // Final Sephiroth 1v1 Battle
    {
        PermadeathExemption& finalBattle = exemptions.emplace_back();
        finalBattle.fieldIDs.insert(763);
    }
}

void Permadeath::onDebugGUI()
{
    std::string deadCharText = "Dead Characters: ";
    for (int i = 0; i < 9; ++i)
    {
        if (deadCharacters.isBitSet(i))
        {
            deadCharText += std::to_string(i) + " ";
        }
    }
    ImGui::Text(deadCharText.c_str());

    if (ImGui::Button("Clear Dead Characters"))
    {
        deadCharacters = 0;
        game->write<uint16_t>(SavemapOffsets::IronMogPermadeath, deadCharacters.value());

        std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
        for (int i = 0; i < 3; ++i)
        {
            uint8_t id = partyIDs[i];
            if (id == 0xFF)
            {
                continue;
            }

            uintptr_t characterOffset = getCharacterDataOffset(id);
            game->write<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP, 1);
        }
    }

    static char debugKillCharacterIndex[5];
    ImGui::InputText("##killCharacterField", debugKillCharacterIndex, 5);
    ImGui::SameLine();
    if (ImGui::Button("Kill"))
    {
        uint16_t charID = atoi(debugKillCharacterIndex);
        if (charID >= 0 && charID <= 9)
        {
            deadCharacters.setBit(charID, true);
        }
    }
}

void Permadeath::onStart()
{
    deadCharacters = game->read<uint16_t>(SavemapOffsets::IronMogPermadeath);
    appliedRufusRandom = false;
    waitingOnBattleExit = false;
}

void Permadeath::onFrame(uint32_t frameNumber)
{
    uint16_t fieldID = game->getFieldID();
    if (isExempt(fieldID))
    {
        // We don't enforce permadeath in scripted scenes where deaths can occur.
        return;
    }

    updateOverrideFights();

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
        if (!deadCharacters.isBitSet(id))
        {
            bool isDead = false;

            if (game->inBattle())
            {
                Flags<uint16_t> statusFlags = game->read<uint16_t>(BattleOffsets::Allies[i] + BattleOffsets::Status);
                isDead = statusFlags.isSet(StatusFlags::Dead);
            }
            else 
            {
                uint16_t currentHP = game->read<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP);
                isDead = (currentHP == 0);
            }

            if (isDead)
            {
                deadCharacters.setBit(id, true);
                game->write<uint16_t>(SavemapOffsets::IronMogPermadeath, deadCharacters.value());
                justDiedCharacters.insert(id);
                LOG("Character has died: %d", id);
            }
        }

        // Force death if the character is in our dead characters list
        if (deadCharacters.isBitSet(id))
        {
            // Force HP to 0
            game->write<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP, 0);

            if (game->inBattle())
            {
                // If the player just died we let the game drop the HP gauges 
                // down naturally instead of us forcing them down instantly.
                if (justDiedCharacters.count(id) > 0)
                {
                    uint16_t currentHP = game->read<uint16_t>(PlayerOffsets::Players[i] + PlayerOffsets::CurrentHP);
                    if (currentHP == 0)
                    {
                        justDiedCharacters.erase(id);
                    }
                }
                else
                {
                    game->write<uint16_t>(PlayerOffsets::Players[i] + PlayerOffsets::CurrentHP, 0);
                    game->write<uint16_t>(BattleStateOffsets::Allies[i] + BattleStateOffsets::HPDisplay, 0);
                }

                game->write<uint16_t>(BattleOffsets::Allies[i] + BattleOffsets::Status, StatusFlags::Dead);
            }
        }
    }
}

void Permadeath::onFieldChanged(uint16_t fieldID)
{
    if (fieldID == RUFUS_FIELD_ID && isCharacterDead(CLOUD_ID))
    {
        if (game->getGameMoment() < 320)
        {
            int randomCharacter = selectRandomLivingCharacter(fieldID, CLOUD_ID);
            if (randomCharacter > -1)
            {
                // Overwrite two commands related to hiding Rufus. This seems to be harmless.
                constexpr uintptr_t RufusHideScript = FieldScriptOffsets::ScriptStart + 0x952;

                // Overwrite the command that triggers the Rufus fight trigger with this 
                // command to switch to party to a different character.
                game->write<uint8_t>(RufusHideScript + 0, 0xCA);
                game->write<uint8_t>(RufusHideScript + 1, (uint8_t)randomCharacter);
                game->write<uint8_t>(RufusHideScript + 2, 0xFE);
                game->write<uint8_t>(RufusHideScript + 3, 0xFE);

                appliedRufusRandom = true;
                LOG("Replaced Cloud with %s in Rufus fight due to Cloud being dead.", getCharacterName(randomCharacter).c_str());
            }
            else 
            {
                // Everyone is dead, do nothing.
                LOG("Did not replace Cloud in Rufus fight because all characters are dead.");
            }
        }
    }

    if (fieldID == DYNE_FIELD_ID && isCharacterDead(BARRET_ID))
    {
        // Select a random living character other than Barret.
        int randomCharacter = selectRandomLivingCharacter(fieldID, BARRET_ID);
        if (randomCharacter == -1)
        {
            // Everyone is dead, do nothing.
            LOG("Did not Barret Cloud in Dyne fight because all characters are dead.");
            return;
        }

        // Overwrite the command that swaps party before the dyne 
        // fight to use a character other than Barret since hes dead.
        constexpr uintptr_t DynePartyCommand = FieldScriptOffsets::ScriptStart + 0x4FE;
        game->write<uint8_t>(DynePartyCommand + 1, (uint8_t)randomCharacter);

        LOG("Replaced Barret with %s in Dyne fight due to Barret being dead.", getCharacterName(randomCharacter).c_str());
    }
}

void Permadeath::onBattleExit()
{
    uint16_t fieldID = game->getFieldID();
    if (fieldID == RUFUS_FIELD_ID && appliedRufusRandom)
    {
        waitingOnBattleExit = true;
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

std::vector<uint8_t> Permadeath::getLivingCharacters()
{
    std::vector<uint8_t> results;
    uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);

    for (int i = 0; i < 9; ++i)
    {
        // If character is not in PHS vis mask then we haven't recruited them yet.
        if (!Utilities::isBitSet(phsVisMask, i))
        {
            continue;
        }

        if (!isCharacterDead(i))
        {
            results.push_back(i);
        }
    }

    return results;
}

int Permadeath::selectRandomLivingCharacter(uint16_t fieldID, uint8_t ignoreCharacter)
{
    std::vector<uint8_t> livingCharacters = getLivingCharacters();
    if (livingCharacters.size() == 0)
    {
        return -1;
    }

    // Create seed for rng
    uint64_t rngSeed = Utilities::makeSeed64(game->getSeed(), fieldID);

    // Shuffle the possible options
    std::mt19937_64 rng(rngSeed);
    std::shuffle(livingCharacters.begin(), livingCharacters.end(), rng);

    // Select character that isn't dead and isn't the one we want to ignore.
    for (int i = 0; i < livingCharacters.size(); ++i)
    {
        if (livingCharacters[i] != ignoreCharacter)
        {
            return livingCharacters[i];
        }
    }

    return -1;
}

// There are two fights in the game where its scripted that you will only have a single character.
// If that character is permadead then you just instantly game over. Rather than letting that happen
// we swap in a living character for just that fight then swap it back after.
// Shoutout to Zheal for this idea.
void Permadeath::updateOverrideFights()
{
    uint16_t fieldID = game->getFieldID();
    
    if (fieldID != RUFUS_FIELD_ID)
    {
        return;
    }
    
    // We only need to do something if Cloud is dead.
    if (!isCharacterDead(CLOUD_ID))
    {
        return;
    }

    if (waitingOnBattleExit)
    {
        // Monitor screen fade for when it starts to drop to know we're revealing the field.
        uint16_t fieldTrigger = game->read<uint16_t>(GameOffsets::FieldScreenFade);
        if (fieldTrigger == 256)
        {
            constexpr uintptr_t ScriptAfterRufus = FieldScriptOffsets::ScriptStart + 0x45E;

            // Overwrite the command that comes after the Rufus fight trigger with this 
            // command to switch to party back to Cloud.
            game->write<uint8_t>(ScriptAfterRufus + 0, 0xCA);
            game->write<uint8_t>(ScriptAfterRufus + 1, CLOUD_ID);
            game->write<uint8_t>(ScriptAfterRufus + 2, 0xFE);
            game->write<uint8_t>(ScriptAfterRufus + 3, 0xFE);

            // MAPJUMP to the same map in the same place. This makes the game reload cloud properly.
            game->write<uint8_t>(ScriptAfterRufus + 4, 0x60);
            game->write<uint16_t>(ScriptAfterRufus + 5, 268);
            game->write<uint16_t>(ScriptAfterRufus + 7, 134);
            game->write<uint16_t>(ScriptAfterRufus + 9, 1617);
            game->write<uint16_t>(ScriptAfterRufus + 11, 16);
            game->write<uint8_t>(ScriptAfterRufus + 14, 0);

            // Overwrite the game moment to where it should be after the Rufus fight.
            game->write<uint16_t>(GameOffsets::GameMoment, 320);

            waitingOnBattleExit = false;
        }

        lastFieldTrigger = fieldTrigger;
    }
}

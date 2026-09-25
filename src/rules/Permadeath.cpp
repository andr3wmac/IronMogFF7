#include "Permadeath.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "core/gui/GUI.h"

#include <imgui.h>
#include <random>

REGISTER_RULE(Permadeath, "Permadeath", "If a character dies, they cannot be revived and will remain dead for the rest of the playthrough.")

#define RUFUS_FIELD_ID 268
#define DYNE_FIELD_ID 480
#define CLOUD_ID 0
#define BARRET_ID 1
#define CLOUD_LIFESTREAM_FIELD_ID 73
#define CLOUD_LIFESTREAM_GAME_MOMENT 1197
#define MAX_CLOUD_DEATHS 3

static const char* cloudDeathModes[] { "Permanent", "Revive After Lifestream", "Sacrifice Your Friends", "Item" };

void Permadeath::setup()
{
    BIND_EVENT(game->onStart, Permadeath::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, Permadeath::onFrame);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, Permadeath::onFieldChanged);
    BIND_EVENT(game->onBattleExit, Permadeath::onBattleExit);
    BIND_EVENT_ONE_ARG(game->onCustomItemUsed, Permadeath::onCustomItemUsed);

    cloudReviveItemId = 0xFFFF;
    if (cloudDeathMode == CloudDeathMode::Item)
    {
        CustomItem item;
        item.name = "Resurrect Cloud";
        item.targetsCharacter = false;
        item.spawnCount = cloudReviveItemCount;
        cloudReviveItemId = game->registerCustomItem(item);
        LOG("Registered %s as item ID: %d", item.name.c_str(), cloudReviveItemId);
    }

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

bool Permadeath::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Delete Equipped", &deleteEquipped);
    ImGui::SetItemTooltip("Anything equipped at the time of death is deleted.");

    ImGui::Spacing();
    ImGui::Text("Cloud Permadeath:");
    ImGui::SetItemTooltip("Permanent: Cloud remains permanently dead.\nRevive After Lifestream: Cloud returns after the Lifestream sequence.\nSacrifice Your Friends: Another character dies in Cloud's place.\nItem: Cloud can be revived with a findable item.");
    ImGui::SameLine(DPI(200.0f));
    ImGui::SetNextItemWidth(DPI(200.0f));

    int cloudDeathModeIndex = (int)cloudDeathMode;
    if (ImGui::Combo("##Permadeath_cloudDeathMode", &cloudDeathModeIndex, cloudDeathModes, IM_ARRAYSIZE(cloudDeathModes)))
    {
        cloudDeathMode = (CloudDeathMode)cloudDeathModeIndex;
        changed = true;
    }

    if (cloudDeathMode == CloudDeathMode::Item)
    {
        ImGui::Text("Number in World:");
        ImGui::SetItemTooltip("How many copies of the revive item are hidden among the field pickups.");
        ImGui::SameLine(DPI(200.0f));
        ImGui::SetNextItemWidth(DPI(200.0f));
        changed |= ImGui::SliderInt("##Permadeath_cloudReviveItemCount", &cloudReviveItemCount, 1, 10);
    }

    return changed;
}

void Permadeath::loadSettings(const ConfigFile& cfg)
{
    deleteEquipped = cfg.get<bool>("deleteEquipped", deleteEquipped);
    cloudDeathMode = (CloudDeathMode)cfg.get<int>("cloudDeathMode", (int)cloudDeathMode);
    cloudReviveItemCount = cfg.get<int>("cloudReviveItemCount", cloudReviveItemCount);
}

void Permadeath::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("deleteEquipped", deleteEquipped);
    cfg.set<int>("cloudDeathMode", (int)cloudDeathMode);
    cfg.set<int>("cloudReviveItemCount", cloudReviveItemCount);
}

void Permadeath::onDebugGUI()
{
    std::string deadCharText = "Dead Characters: ";
    uint16_t deadMask = getDeadCharacterMask();
    for (int i = 0; i < 9; ++i)
    {
        if (Utilities::isBitSet(deadMask, i))
        {
            deadCharText += std::to_string(i) + " ";
        }
    }
    ImGui::Text(deadCharText.c_str());

    // These mutate rule state, so they run on the game manager thread.
    if (ImGui::Button("Clear Dead Characters"))
    {
        game->queueAction([this]() { clearDeadCharacters(); });
    }

    static char debugKillCharacterIndex[5];
    ImGui::InputText("##killCharacterField", debugKillCharacterIndex, 5);
    ImGui::SameLine();
    if (ImGui::Button("Kill"))
    {
        uint8_t charID = atoi(debugKillCharacterIndex);
        if (charID <= 9)
        {
            game->queueAction([this, charID]() { killCharacter(charID); });
        }
    }
}

void Permadeath::clearDeadCharacters()
{
    deadCharacters = 0;
    cloudDeathCount = 0;
    savePermadeathState();

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

std::vector<std::string> Permadeath::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Unique)
    {
        return { "Permadeath" };
    }

    return {};
}

void Permadeath::onStart()
{
    loadPermadeathState();
    newCloudDeath = false;
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
                // The game sets the Dead status bit the moment an enemy attack begins, before the animation plays out. 
                // We also require the displayed HP gauge to have drained to 0 so permadeath triggers when the death is 
                // actually visible on screen, rather than spoiling it at the start of the attack.
                Flags<uint16_t> statusFlags = game->read<uint16_t>(BattleOffsets::Allies[i] + BattleOffsets::Status);
                uint16_t hpDisplay = game->read<uint16_t>(BattleStateOffsets::Allies[i] + BattleStateOffsets::HPDisplay);
                isDead = statusFlags.isSet(StatusFlags::Dead) && hpDisplay == 0;
            }
            else 
            {
                uint16_t currentHP = game->read<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP);
                isDead = (currentHP == 0);
            }

            if (isDead)
            {
                killCharacter(id);
            }
        }

        // Force death if the character is in our dead characters list
        if (deadCharacters.isBitSet(id))
        {
            // Force HP to 0
            game->write<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP, 0);

            if (game->inBattle())
            {
                // If the player just died we let the game drop the HP gauges down naturally instead of us forcing them down instantly.
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
    reviveCloudAfterLifestream(fieldID);

    if (fieldID == RUFUS_FIELD_ID && isCharacterDead(CLOUD_ID))
    {
        if (game->getGameMoment() < 320)
        {
            int randomCharacter = selectRandomLivingCharacter(fieldID, CLOUD_ID);
            if (randomCharacter > -1)
            {
                uintptr_t rufusHideScript = FieldScriptOffsets::ScriptStart + 0x952;
                if (game->getGameVersion() == GameVersion::PlayStationUS_CSR)
                {
                    rufusHideScript = FieldScriptOffsets::ScriptStart + 0x94D;
                }

                // Overwrite two commands related to hiding Rufus. This seems to be harmless.
                game->write<uint8_t>(rufusHideScript + 0, 0xCA);
                game->write<uint8_t>(rufusHideScript + 1, (uint8_t)randomCharacter);
                game->write<uint8_t>(rufusHideScript + 2, 0xFE);
                game->write<uint8_t>(rufusHideScript + 3, 0xFE);

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
            LOG("Did not replace Barret in Dyne fight because all characters are dead.");
            return;
        }

        // Overwrite the command that swaps party before the dyne fight to use a character other than Barret since hes dead.
        uintptr_t dynePartyCommand = FieldScriptOffsets::ScriptStart + 0x4FE;
        if (game->getGameVersion() == GameVersion::PlayStationUS_CSR)
        {
            dynePartyCommand = FieldScriptOffsets::ScriptStart + 0x508;
        }
        game->write<uint8_t>(dynePartyCommand + 1, (uint8_t)randomCharacter);

        LOG("Replaced Barret with %s in Dyne fight due to Barret being dead.", getCharacterName(randomCharacter).c_str());
    }
}

void Permadeath::onBattleExit()
{
    sacrificeFriendForCloud();

    uint16_t fieldID = game->getFieldID();
    if (fieldID == RUFUS_FIELD_ID && appliedRufusRandom)
    {
        waitingOnBattleExit = true;
    }
}

void Permadeath::onCustomItemUsed(CustomItemUse use)
{
    if (cloudDeathMode != CloudDeathMode::Item || use.itemId != cloudReviveItemId)
    {
        return;
    }

    if (!isCharacterDead(CLOUD_ID))
    {
        // Nothing to do if Cloud is already alive.
        return;
    }

    reviveCharacter(CLOUD_ID);
    game->menu.showPopup("Cloud has been revived!");
    LOG("Cloud Permadeath: revive item used.");
}

void Permadeath::reviveCloudAfterLifestream(uint16_t fieldID)
{
    if (cloudDeathMode != CloudDeathMode::ReviveAfterLifestream || fieldID != CLOUD_LIFESTREAM_FIELD_ID || game->getGameMoment() != CLOUD_LIFESTREAM_GAME_MOMENT)
    {
        return;
    }

    if (!isCharacterDead(CLOUD_ID))
    {
        return;
    }

    reviveCharacter(CLOUD_ID);
    LOG("Cloud Permadeath: revived Cloud after the Lifestream sequence.");
}

// Cloud's deaths are counted in killCharacter. Any death that hasn't been paid for yet (Cloud dead with fewer
// than 3 deaths) is resolved here, so the sacrifice survives a reload between the death and the battle exit.
void Permadeath::sacrificeFriendForCloud()
{
    if (cloudDeathMode != CloudDeathMode::SacrificeYourFriends || !isCharacterDead(CLOUD_ID))
    {
        newCloudDeath = false;
        return;
    }

    // Each of Cloud's deaths costs more:
    // 1 friend the first time, 2 the second, and on the 3rd death he's permanently gone.
    if (cloudDeathCount >= MAX_CLOUD_DEATHS)
    {
        if (newCloudDeath)
        {
            LOG("Cloud Permadeath: Cloud died a third time and is now permanently dead.");
        }
        newCloudDeath = false;
        return;
    }

    // Cloud died without the death being counted (e.g. while another death mode was active), count it now.
    if (cloudDeathCount == 0)
    {
        cloudDeathCount = 1;
        savePermadeathState();
    }

    // Cloud is already dead so getLivingCharacters excludes him, these are the sacrifice candidates.
    std::vector<uint8_t> livingCharacters = getLivingCharacters();
    if ((int)livingCharacters.size() < cloudDeathCount)
    {
        // Not enough friends to pay the toll, so Cloud stays dead. The toll is retried on later battle exits
        // in case more characters are recruited, without counting it as another death.
        if (newCloudDeath)
        {
            LOG("Cloud Permadeath: not enough living characters to sacrifice; Cloud remains dead.");
        }
        newCloudDeath = false;
        return;
    }
    newCloudDeath = false;

    // Shuffle deterministically from the seed so the chosen victims are stable across reloads.
    uint64_t rngSeed = Utilities::makeSeed64(game->getSeed(), game->getFieldID());
    std::mt19937_64 rng(rngSeed);
    std::shuffle(livingCharacters.begin(), livingCharacters.end(), rng);

    reviveCharacter(CLOUD_ID);

    std::string sacrificedNames;
    for (int i = 0; i < cloudDeathCount; ++i)
    {
        uint8_t victim = livingCharacters[i];
        killCharacter(victim);

        if (!sacrificedNames.empty())
        {
            sacrificedNames += ", ";
        }
        sacrificedNames += getCharacterName(victim);
    }

    LOG("Cloud Permadeath: sacrificed %s to revive Cloud (death #%d).", sacrificedNames.c_str(), cloudDeathCount);
}

void Permadeath::reviveCharacter(uint8_t id)
{
    deadCharacters.setBit(id, false);
    savePermadeathState();
    justDiedCharacters.erase(id);

    // Restore to full HP so the onFrame loop stops forcing the character down and they're properly alive again.
    uintptr_t characterOffset = getCharacterDataOffset(id);
    uint16_t maxHP = game->read<uint16_t>(characterOffset + CharacterDataOffsets::MaxHP);
    game->write<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP, maxHP);

    LOG("Character has been revived: %d", id);
}

// The savemap word packs the dead-character mask in the low bits and Cloud's death count in the top two bits (14-15).
// Splitting/combining is kept to these two functions so the bit layout lives in one place.
void Permadeath::loadPermadeathState()
{
    uint16_t raw = game->read<uint16_t>(SavemapOffsets::IronMogPermadeath);
    deadCharacters = raw & 0x3FFF;
    cloudDeathCount = std::min<uint8_t>((uint8_t)((raw >> 14) & 0x3), MAX_CLOUD_DEATHS);
    publishedDeadCharacters = deadCharacters.value();
}

void Permadeath::savePermadeathState()
{
    uint16_t raw = (uint16_t)(deadCharacters.value() & 0x3FFF) | (uint16_t)((cloudDeathCount & 0x3) << 14);
    game->write<uint16_t>(SavemapOffsets::IronMogPermadeath, raw);
    publishedDeadCharacters = deadCharacters.value();
}

void Permadeath::killCharacter(uint8_t id)
{
    // Count each of Cloud's deaths once, capped so it can't wrap the 2 bits it's saved in.
    if (id == CLOUD_ID && !isCharacterDead(CLOUD_ID) && cloudDeathMode == CloudDeathMode::SacrificeYourFriends)
    {
        cloudDeathCount = std::min<uint8_t>(cloudDeathCount + 1, MAX_CLOUD_DEATHS);
        newCloudDeath = true;
    }

    deadCharacters.setBit(id, true);
    savePermadeathState();
    justDiedCharacters.insert(id);
    LOG("Character has died: %d", id);

    if (deleteEquipped)
    {
        // Weapons IDs for each characters default weapon.
        static uint8_t defaultWeapons[] = { 0, 32, 16, 62, 48, 87, 101, 114, 73 };

        uintptr_t characterOffset = getCharacterDataOffset(id);

        // Delete equipment
        game->write<uint8_t>(characterOffset + CharacterDataOffsets::EquippedWeapon, defaultWeapons[id]);
        game->write<uint8_t>(characterOffset + CharacterDataOffsets::EquippedArmor, 0x00);
        game->write<uint8_t>(characterOffset + CharacterDataOffsets::EquippedAccessory, 0xFF);

        // Clear weapon and armor materia slots
        for (int i = 0; i < 8; ++i)
        {
            game->write<uint32_t>(characterOffset + CharacterDataOffsets::WeaponMateria[i], 0xFFFFFFFF);
            game->write<uint32_t>(characterOffset + CharacterDataOffsets::ArmorMateria[i], 0xFFFFFFFF);
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
            uintptr_t scriptAfterRufus = FieldScriptOffsets::ScriptStart + 0x45E;
            if (game->getGameVersion() == GameVersion::PlayStationUS_CSR)
            {
                scriptAfterRufus = FieldScriptOffsets::ScriptStart + 0x46A;
            }

            // Overwrite the command that comes after the Rufus fight trigger with this command to switch to party back to Cloud.
            game->write<uint8_t>(scriptAfterRufus + 0, 0xCA);
            game->write<uint8_t>(scriptAfterRufus + 1, CLOUD_ID);
            game->write<uint8_t>(scriptAfterRufus + 2, 0xFE);
            game->write<uint8_t>(scriptAfterRufus + 3, 0xFE);

            // MAPJUMP to the same map in the same place. This makes the game reload cloud properly.
            game->write<uint8_t>(scriptAfterRufus + 4, 0x60);
            game->write<uint16_t>(scriptAfterRufus + 5, 268);
            game->write<uint16_t>(scriptAfterRufus + 7, 134);
            game->write<uint16_t>(scriptAfterRufus + 9, 1617);
            game->write<uint16_t>(scriptAfterRufus + 11, 16);
            game->write<uint8_t>(scriptAfterRufus + 14, 0);

            // Overwrite the game moment to where it should be after the Rufus fight.
            game->write<uint16_t>(GameOffsets::GameMoment, 320);

            waitingOnBattleExit = false;
        }

        lastFieldTrigger = fieldTrigger;
    }
}

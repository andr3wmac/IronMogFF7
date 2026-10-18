#include "Permadeath.h"
#include "app/gui/GUI.h"
#include "AppFrame/AppFrame.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "LiveModFF7Core/utilities/Utilities.h"
#include "utilities/Flags.h"

#include <algorithm>
#include <random>

REGISTER_MOD(Permadeath, "Permadeath", "Restrictions")

std::string Permadeath::getDescription() const
{
    return "A character who dies cannot be revived for the rest of the run.\n\nSettings can also remove that character's equipped items when they die.";
}

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
    GUI::wrappedTooltip("Anything equipped at the time of death is deleted.");

    ImGui::Spacing();
    ImGui::Text("Cloud Permadeath:");
    GUI::wrappedTooltip("Permanent: Cloud remains permanently dead.\nRevive After Lifestream: Cloud returns after the Lifestream sequence.\nSacrifice Your Friends: Another character dies in Cloud's place.\nItem: Cloud can be revived with a findable item.");
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
        GUI::wrappedTooltip("How many copies of the revive item are hidden among the field pickups.");
        ImGui::SameLine(DPI(200.0f));
        ImGui::SetNextItemWidth(DPI(200.0f));
        changed |= ImGui::SliderInt("##Permadeath_cloudReviveItemCount", &cloudReviveItemCount, 1, 10);
    }

    if (cloudDeathMode == CloudDeathMode::SacrificeYourFriends)
    {
        changed |= ImGui::Checkbox("Prefer Party Members", &sacrificePreferParty);
        GUI::wrappedTooltip("Sacrifices are taken from current party first, then from other recruited characters. When disabled, sacrifices are chosen at random.");

        changed |= ImGui::Checkbox("Include Unrecruited", &sacrificeIncludeUnrecruited);
        GUI::wrappedTooltip("Characters you haven't recruited yet (e.g. Yuffie, Vincent) can be sacrificed. With Prefer Party Members enabled, they're only used as a last resort.");

        if (deleteEquipped)
        {
            changed |= ImGui::Checkbox("Delete Cloud Equipped", &deleteCloudEquipped);
            GUI::wrappedTooltip("Delete Cloud's equipment and materia when he dies in Sacrifice Your Friends mode.");
        }
    }

    ImGui::Spacing();
    changed |= ImGui::Checkbox("Always Randomize 1v1 Fights", &alwaysRandomizeOneVOne);
    GUI::wrappedTooltip("Always randomize which character fights in a 1v1 (e.g. Rufus, Dyne) even if the scripted character is still alive.");

    return changed;
}

void Permadeath::loadSettings(const ConfigFile& cfg)
{
    deleteEquipped = cfg.get<bool>("deleteEquipped", deleteEquipped);
    deleteCloudEquipped = cfg.get<bool>("deleteCloudEquipped", deleteCloudEquipped);
    cloudDeathMode = (CloudDeathMode)cfg.get<int>("cloudDeathMode", (int)cloudDeathMode);
    cloudReviveItemCount = cfg.get<int>("cloudReviveItemCount", cloudReviveItemCount);
    sacrificePreferParty = cfg.get<bool>("sacrificePreferParty", sacrificePreferParty);
    sacrificeIncludeUnrecruited = cfg.get<bool>("sacrificeIncludeUnrecruited", sacrificeIncludeUnrecruited);
    alwaysRandomizeOneVOne = cfg.get<bool>("alwaysRandomizeOneVOne", alwaysRandomizeOneVOne);
}

void Permadeath::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("deleteEquipped", deleteEquipped);
    cfg.set<bool>("deleteCloudEquipped", deleteCloudEquipped);
    cfg.set<int>("cloudDeathMode", (int)cloudDeathMode);
    cfg.set<int>("cloudReviveItemCount", cloudReviveItemCount);
    cfg.set<bool>("sacrificePreferParty", sacrificePreferParty);
    cfg.set<bool>("sacrificeIncludeUnrecruited", sacrificeIncludeUnrecruited);
    cfg.set<bool>("alwaysRandomizeOneVOne", alwaysRandomizeOneVOne);
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

    uint8_t rufusCharacter = rufusOverrideCharacter;
    uint8_t dyneCharacter = dyneOverrideCharacter;
    ImGui::Text("Always randomize 1v1 fights: %s", alwaysRandomizeOneVOne ? "On" : "Off");
    ImGui::Text("Rufus fight: %s", rufusCharacter == 0xFF ? "Cloud (unchanged)" : getCharacterName(rufusCharacter).c_str());
    ImGui::Text("Dyne fight: %s", dyneCharacter == 0xFF ? "Barret (unchanged)" : getCharacterName(dyneCharacter).c_str());

    // These mutate mod state, so they run on the game manager thread.
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

std::vector<std::string> Permadeath::describe(ModDescriptionType descType)
{
    if (descType == ModDescriptionType::Unique)
    {
        if (cloudDeathMode == CloudDeathMode::SacrificeYourFriends)
        {
            return { "Permadeath (with Sacrifice Your Friends)" };
        }
        else 
        {
            return { "Permadeath" };
        }
    }

    if (descType == ModDescriptionType::Randomized && alwaysRandomizeOneVOne)
    {
        return { "1v1 fights" };
    }

    return {};
}

void Permadeath::onStart()
{
    loadPermadeathState();
    newCloudDeath = false;
    appliedRufusRandom = false;
    appliedDyneRandom = false;
    waitingOnBattleExit = false;
    rufusOverrideCharacter = 0xFF;
    dyneOverrideCharacter = 0xFF;
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

    // The Rufus and Dyne fights are scripted to always use Cloud and Barret. Normally we only substitute a living character 
    // when the scripted one is permadead but when "Always Randomize 1v1 Fights" is enabled we roll a random character everytime.
    if (fieldID == RUFUS_FIELD_ID && (isCharacterDead(CLOUD_ID) || alwaysRandomizeOneVOne))
    {
        if (game->getGameMoment() < 320)
        {
            // 0xFF means "exclude nobody" so Cloud can be rolled when he is alive.
            int randomCharacter = selectRandomLivingCharacter(fieldID, alwaysRandomizeOneVOne ? 0xFF : CLOUD_ID);
            if (randomCharacter == CLOUD_ID)
            {
                LOG("Left the Rufus fight alone because Cloud was selected.");
            }
            else if (randomCharacter > -1)
            {
                // TLKON (0x7E) 1, SOLID (0xC7) 1.
                std::optional<uint16_t> rufusHideOffset = game->field.findScriptOffset("lu", 11, { 0x7E, 0x01, 0xC7, 0x01 });
                if (rufusHideOffset)
                {
                    uintptr_t rufusHideScript = FieldScriptOffsets::ScriptStart + *rufusHideOffset;

                    // Overwrite two commands related to hiding Rufus. This seems to be harmless.
                    game->write<uint8_t>(rufusHideScript + 0, 0xCA);
                    game->write<uint8_t>(rufusHideScript + 1, (uint8_t)randomCharacter);
                    game->write<uint8_t>(rufusHideScript + 2, 0xFE);
                    game->write<uint8_t>(rufusHideScript + 3, 0xFE);

                    appliedRufusRandom    = true;
                    rufusOverrideCharacter = (uint8_t)randomCharacter;
                    LOG("Replaced Cloud with %s in Rufus fight%s.", getCharacterName(randomCharacter).c_str(),
                        isCharacterDead(CLOUD_ID) ? " due to Cloud being dead" : " because 1v1 fights are always randomized");
                }
                else
                {
                    LOG("Did not replace Cloud in Rufus fight because the Rufus hide commands were not found.");
                }
            }
            else
            {
                // Everyone is dead, do nothing.
                LOG("Did not replace Cloud in Rufus fight because all characters are dead.");
            }
        }
    }

    if (fieldID == DYNE_FIELD_ID && (isCharacterDead(BARRET_ID) || alwaysRandomizeOneVOne))
    {
        // 0xFF means "exclude nobody" so Barret can be rolled when he is alive.
        int randomCharacter = selectRandomLivingCharacter(fieldID, alwaysRandomizeOneVOne ? 0xFF : BARRET_ID);
        if (randomCharacter == BARRET_ID)
        {
            LOG("Left the Dyne fight alone because Barret was selected.");
        }
        else if (randomCharacter == -1)
        {
            // Everyone is dead, do nothing.
            LOG("Did not replace Barret in Dyne fight because all characters are dead.");
            return;
        }
        else
        {
            // Overwrite the command that swaps party before the dyne fight to use a character other than Barret.
            // PRTYE (0xCA) Barret, empty, empty.
            std::optional<uint16_t> dynePartyCommand = game->field.findScriptOffset("dic", 0, { 0xCA, BARRET_ID, 0xFE, 0xFE });
            if (!dynePartyCommand)
            {
                LOG("Did not replace Barret in Dyne fight because the party command was not found.");
                return;
            }
            game->write<uint8_t>(FieldScriptOffsets::ScriptStart + *dynePartyCommand + 1, (uint8_t)randomCharacter);

            appliedDyneRandom    = true;
            dyneOverrideCharacter = (uint8_t)randomCharacter;
            LOG("Replaced Barret with %s in Dyne fight%s.", getCharacterName(randomCharacter).c_str(),
                isCharacterDead(BARRET_ID) ? " due to Barret being dead" : " because 1v1 fights are always randomized");
        }
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

    // Sort living friends into priority tiers: current party, other recruited (visible in PHS), then unrecruited.
    // Without Prefer Party Members everyone shares a single tier and who is sacrificed is purely random.
    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);
    std::array<std::vector<uint8_t>, 3> tiers;
    for (uint8_t id = 0; id < 9; ++id)
    {
        if (id == CLOUD_ID || isCharacterDead(id))
        {
            continue;
        }

        bool inParty = std::find(partyIDs.begin(), partyIDs.end(), id) != partyIDs.end();
        bool recruited = inParty || Utilities::isBitSet(phsVisMask, id);
        if (!recruited && !sacrificeIncludeUnrecruited)
        {
            continue;
        }

        int tier = 0;
        if (sacrificePreferParty)
        {
            tier = inParty ? 0 : (recruited ? 1 : 2);
        }
        tiers[tier].push_back(id);
    }

    // Shuffle deterministically from the seed so the chosen victims are stable across reloads.
    uint64_t rngSeed = Utilities::makeSeed64(game->getSeed(), game->getFieldID());
    std::mt19937_64 rng(rngSeed);

    std::vector<uint8_t> livingCharacters;
    for (std::vector<uint8_t>& tier : tiers)
    {
        std::shuffle(tier.begin(), tier.end(), rng);
        livingCharacters.insert(livingCharacters.end(), tier.begin(), tier.end());
    }

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

    bool keepCloudEquipped = id == CLOUD_ID && cloudDeathMode == CloudDeathMode::SacrificeYourFriends && !deleteCloudEquipped;
    if (deleteEquipped && !keepCloudEquipped)
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
    
    // We only need to switch the party back if someone else was substituted into the fight.
    if (!appliedRufusRandom)
    {
        return;
    }

    if (waitingOnBattleExit)
    {
        // Monitor screen fade for when it starts to drop to know we're revealing the field.
        uint16_t fieldTrigger = game->read<uint16_t>(GameOffsets::FieldScreenFade);
        if (fieldTrigger == 256)
        {
            // SCRLA (0x63) camera scroll that follows the Rufus fight trigger.
            std::optional<uint16_t> afterRufusOffset = game->field.findScriptOffset("dir", 0, { 0x63, 0x00, 0x1E, 0x00 });
            if (!afterRufusOffset)
            {
                LOG("Did not switch party back to Cloud after Rufus fight because the script was not found.");
                waitingOnBattleExit = false;
                return;
            }
            uintptr_t scriptAfterRufus = FieldScriptOffsets::ScriptStart + *afterRufusOffset;

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

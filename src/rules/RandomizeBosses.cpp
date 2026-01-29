#include "RandomizeBosses.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>

REGISTER_RULE("Randomize Bosses", RandomizeBosses)

RandomizeBosses::RandomizeBosses()
{
    randomNames.resize(7);
    randomNames[0] = "Normal";
    randomNames[1] = "Death";
    randomNames[2] = "Double Damage";
    randomNames[3] = "Half Damage";
    randomNames[4] = "Immune";
    randomNames[5] = "Absorb";
    randomNames[6] = "Full Cure";

    randomWeights.resize(7);
    randomWeights[0] = 50;
    randomWeights[1] = 0;
    randomWeights[2] = 20;
    randomWeights[3] = 0;
    randomWeights[4] = 20;
    randomWeights[5] = 10;
    randomWeights[6] = 0;
}

void RandomizeBosses::setup()
{
    BIND_EVENT(game->onStart, RandomizeBosses::onStart);
    BIND_EVENT(game->onBattleEnter, RandomizeBosses::onBattleEnter);
}

bool RandomizeBosses::onSettingsGUI()
{
    bool changed = false;

    ImGui::Text("Stat Multiplier");
    ImGui::SetItemTooltip("Multiplies each boss' HP, MP, Strength, Magic,\nEvade, Speed, Luck, Defense, and MDefense.");
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(50.0f);
    changed |= ImGui::InputFloat("##bossStatMultiplier", &statMultiplier, 0.0f, 0.0f, "%.2f");

    if (ImGui::CollapsingHeader("Resistance and Weakness"))
    {
        int* randomModeInt = (int*)(&randomMode);

        ImGui::Indent(25.0f);
        changed |= ImGui::RadioButton("Shuffle", randomModeInt, 0);
        ImGui::SetItemTooltip("Resistances and weaknesses are shuffled amongst bosses.");
        changed |= ImGui::RadioButton("Weighted Random", randomModeInt, 1);
        ImGui::SetItemTooltip("Elements are randomly selected and their\neffect is set based on chosen odds.");

        if (randomMode == RandomMode::WeightedRandom)
        {
            ImGui::Indent(25.0f);

            ImGui::Text("Element Count");
            ImGui::SetItemTooltip("The number of elements that will be rolled using the odds set below.");
            ImGui::SameLine(180.0f);
            ImGui::SetNextItemWidth(200.0f);
            changed |= ImGui::SliderInt("##elementCount", &elementCount, 0, 7);

            for (int i = 0; i < randomNames.size(); i++)
            {
                ImGui::PushID(i); // Prevents ID conflicts

                ImGui::Text(randomNames[i].c_str());
                ImGui::SameLine(180.0f);

                // Set a small width so it's not a giant text box
                ImGui::SetNextItemWidth(35.0f);
                changed |= ImGui::InputInt("##val", &randomWeights[i], 0);

                ImGui::PopID();
            }

            ImGui::Unindent(25.0f);
        }

        ImGui::Unindent(25.0f);
    }

    return changed;
}

void RandomizeBosses::loadSettings(const ConfigFile& cfg)
{
    statMultiplier = cfg.get<float>("statMultiplier", 1.0f);
    randomMode = (RandomMode)cfg.get<int>("randomMode", 0);

    elementCount = cfg.get<int>("elementCount", elementCount);
    for (int i = 0; i < randomWeights.size(); i++)
    {
        randomWeights[i] = cfg.get<int>("randomWeight" + std::to_string(i), randomWeights[i]);
    }
}

void RandomizeBosses::saveSettings(ConfigFile& cfg)
{
    cfg.set<float>("statMultiplier", statMultiplier);
    cfg.set<int>("randomMode", (int)randomMode);

    cfg.set<int>("elementCount", elementCount);
    for (int i = 0; i < randomWeights.size(); i++)
    {
        cfg.set<int>("randomWeight" + std::to_string(i), randomWeights[i]);
    }
}

std::string buildElementsString(uint64_t elementTypes, uint64_t elementRates, std::string delimiter = ", ")
{
    std::string logString = "";

    for (int i = 0; i < 8; ++i)
    {
        uint8_t type = uint8_t((elementTypes >> (i * 8)) & 0xFF);
        uint8_t rate = uint8_t((elementRates >> (i * 8)) & 0xFF);

        if (type == ElementType::None || rate == ElementRate::None)
        {
            continue;
        }

        if (i > 0)
        {
            logString += delimiter;
        }
        logString += getElementTypeName(type) + ": " + getElementRateName(rate);
    }

    return logString;
}

void RandomizeBosses::onDebugGUI()
{
    std::pair<BattleScene*, BattleFormation*> battleData = game->getBattleFormation();
    BattleScene* scene = battleData.first;
    BattleFormation* formation = battleData.second;

    if (scene == nullptr || formation == nullptr)
    {
        ImGui::Text("Not currently in a battle.");
        return;
    }

    std::vector<const Boss*> bosses = GameData::getBossesInScene(scene);
    if (bosses.size() == 0)
    {
        ImGui::Text("Not currently in a battle with any bosses.");
        return;
    }

    for (const Boss* boss : bosses)
    {
        std::string bossDisplay = std::to_string(boss->id) + ") " + boss->name;
        ImGui::Text(bossDisplay.c_str());

        ImGui::Indent(32.0f);
        for (int i = 0; i < 3; ++i)
        {
            if (boss->id == scene->enemyIDs[i])
            {
                uint64_t elementTypes = game->read<uint64_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::ElementTypes);
                uint64_t elementRates = game->read<uint64_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::ElementRates);
                std::string elementsStr = buildElementsString(elementTypes, elementRates, "\n");
                ImGui::Text(elementsStr.c_str());
                break;
            }
        }
        ImGui::Unindent(32.0f);
    }
}

void RandomizeBosses::onStart()
{
    rng.seed(game->getSeed());
    generateShuffledBosses();
}

void RandomizeBosses::generateShuffledBosses()
{
    shuffledBosses.clear();

    std::vector<Boss> bosses;
    std::vector<uint16_t> bossIDs;

    for (const Boss& boss : GameData::bosses)
    {
        bosses.push_back(boss); 
        bossIDs.push_back(boss.id);
    }

    std::shuffle(bossIDs.begin(), bossIDs.end(), rng);
    for (int i = 0; i < bossIDs.size(); ++i)
    {
        shuffledBosses[bossIDs[i]] = bosses[i];
    }
}

std::pair<uint64_t, uint64_t> RandomizeBosses::getWeightedRandomElements(uint16_t bossID)
{
    std::vector<uint8_t> elementTypes;
    std::vector<uint8_t> elementRates;

    // All bosses are immune to gravity magic
    elementTypes.push_back(ElementType::Gravity);
    elementRates.push_back(ElementRate::NullifyDamage);

    std::mt19937_64 rng64(Utilities::makeSeed64(game->getSeed(), bossID));

    // Gravity is excluded from types since we hardcoded it above.
    std::vector<uint8_t> typeItems = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E };
    std::shuffle(typeItems.begin(), typeItems.end(), rng64);

    std::vector<uint8_t> rateItems = { 0xFF, 0x00, 0x02, 0x04, 0x05, 0x06, 0x07 };
    std::discrete_distribution<int> rateDist{ {(double)randomWeights[0], (double)randomWeights[1], (double)randomWeights[2], (double)randomWeights[3], (double)randomWeights[4], (double)randomWeights[5], (double)randomWeights[6]} };
    
    for (int i = 0; i < elementCount; ++i)
    {
        uint8_t rate = rateItems[rateDist(rng64)];
        if (rate == ElementRate::None)
        {
            continue;
        }

        // Type items was shuffled above so sequential access is random results
        uint8_t type = typeItems[i];
        elementTypes.push_back(type);
        elementRates.push_back(rate);
    }

    while (elementTypes.size() < 8)
    {
        elementTypes.push_back(ElementType::None);
        elementRates.push_back(ElementRate::None);
    }

    uint64_t* packedElementTypes = (uint64_t*)elementTypes.data();
    uint64_t* packedElementRates = (uint64_t*)elementRates.data();
    return { *packedElementTypes, *packedElementRates };
}

void RandomizeBosses::onBattleEnter()
{
    uint16_t fieldID = game->getFieldID();
    uint16_t formationID = game->read<uint16_t>(BattleOffsets::FormationID);

    std::pair<BattleScene*, BattleFormation*> battleData = game->getBattleFormation();
    BattleScene* scene = battleData.first;
    BattleFormation* formation = battleData.second;

    if (scene == nullptr || formation == nullptr)
    {
        return;
    }

    bool inBossFight = false;

    std::vector<const Boss*> bosses = GameData::getBossesInScene(scene);
    for (const Boss* boss : bosses)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (boss->id == scene->enemyIDs[i])
            {
                if (randomMode == RandomMode::Shuffle)
                {
                    Boss& shuffledBoss = shuffledBosses[boss->id];

                    game->write<uint64_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::ElementTypes, shuffledBoss.elementTypes);
                    game->write<uint64_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::ElementRates, shuffledBoss.elementRates);

                    std::string elementsStr = buildElementsString(shuffledBoss.elementTypes, shuffledBoss.elementRates);
                    LOG("Shuffled boss %s to %s weakness and resistance: %s", boss->name.c_str(), shuffledBoss.name.c_str(), elementsStr.c_str());
                }

                if (randomMode == RandomMode::WeightedRandom)
                {
                    std::pair<uint64_t, uint64_t> randomizedElements = getWeightedRandomElements(boss->id);

                    game->write<uint64_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::ElementTypes, randomizedElements.first);
                    game->write<uint64_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::ElementRates, randomizedElements.second);

                    std::string elementsStr = buildElementsString(randomizedElements.first, randomizedElements.second);
                    LOG("Randomized weakness and resistance on boss %s to: %s", boss->name.c_str(), elementsStr.c_str());
                }
    
                inBossFight = true;
                break;
            }
        }
    }

    if (inBossFight && statMultiplier != 1.0f)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (formation->enemyIDs[i] == UINT16_MAX)
            {
                continue;
            }

            game->applyBattleStatMultiplier(BattleOffsets::Enemies[i], statMultiplier);
        }
    }
}
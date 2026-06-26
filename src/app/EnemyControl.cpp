#include "EnemyControl.h"

#include "livemod/game/MemoryOffsets.h"
#include "core/gui/GUI.h"

#include <imgui.h>

#include <array>
#include <cstdio>
#include <string>

namespace
{
    struct AttackOption
    {
        const char* name;
        uint16_t id;
    };

    constexpr uintptr_t Enemy1ATBRateOffset = 0xF5CCA;
    constexpr uintptr_t Enemy1ATBOffset = 0xF5CCC;
    constexpr uintptr_t Enemy1MainScriptOffset = 0xF6F28;
    constexpr uint16_t DefaultEnemy1ATBRate = 0x2022;

    const AttackOption attackOptions[]
    {
        { "Scorpion Tail (011B)", 0x011B },
        { "Search Scope (011C)", 0x011C },
    };
    const char* attackOptionNames[]
    {
        "Scorpion Tail (011B)",
        "Search Scope (011C)",
    };

    void drawProgressBar(const char* label, float fraction, const std::string& overlay, const ImVec4& color)
    {
        const float labelWidth = DPI(35.0f);
        const float barWidth = DPI(300.0f);

        if (fraction < 0.0f)
        {
            fraction = 0.0f;
        }
        else if (fraction > 1.0f)
        {
            fraction = 1.0f;
        }

        ImGui::Text("%s", label);
        ImGui::SameLine(labelWidth);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(fraction, ImVec2(barWidth, 0.0f), overlay.c_str());
        ImGui::PopStyleColor();
    }
}

EnemyControl::EnemyControl()
{
    snprintf(message, sizeof(message), "%s", "Hello World!");
}

void EnemyControl::setup(GameManager* game)
{
    reset();
    this->game = game;
}

void EnemyControl::reset()
{
    if (game != nullptr)
    {
        if (enemy1ScriptOverwrite.isApplied())
        {
            enemy1ScriptOverwrite.restore();
        }

        if (controlEnemy1 && hasSavedEnemy1ATBRate)
        {
            game->write<uint16_t>(Enemy1ATBRateOffset, savedEnemy1ATBRate);
        }
    }

    game = nullptr;
    enemy1ScriptOverwrite = {};

    controlEnemy1 = false;
    previousControlEnemy1 = false;
    hasSavedEnemy1ATBRate = false;
    enemy1ActionSubmitted = false;
    savedEnemy1ATBRate = DefaultEnemy1ATBRate;
    lastEnemy1ATB = 0;

    includeMessage = true;
    snprintf(message, sizeof(message), "%s", "Hello World!");
    selectedTarget = 0;
    selectedAttack = 0;
}

void EnemyControl::draw()
{
    if (game == nullptr)
    {
        ImGui::Text("Not connected.");
        return;
    }

    uint16_t enemy1ATBRate = game->read<uint16_t>(Enemy1ATBRateOffset);
    uint16_t enemy1ATB = game->read<uint16_t>(Enemy1ATBOffset);
    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    std::array<std::string, 3> targetOptionLabels;
    std::array<const char*, 3> targetOptions;

    for (int i = 0; i < 3; ++i)
    {
        if (partyIDs[i] == 0xFF)
        {
            targetOptionLabels[i] = std::to_string(i + 1) + ": Empty";
        }
        else
        {
            targetOptionLabels[i] = std::to_string(i + 1) + ": " + getCharacterName(partyIDs[i]);
        }

        targetOptions[i] = targetOptionLabels[i].c_str();
    }

    uint32_t curHP = game->read<uint32_t>(BattleOffsets::Enemies[0] + BattleOffsets::CurrentHP);
    uint32_t maxHP = game->read<uint32_t>(BattleOffsets::Enemies[0] + BattleOffsets::MaxHP);
    float hpPercent = maxHP > 0 ? (float)curHP / (float)maxHP : 0.0f;
    std::string hpText = std::to_string(curHP) + "/" + std::to_string(maxHP);
    drawProgressBar("HP", hpPercent, hpText, ImVec4(0.75f, 0.15f, 0.12f, 1.0f));

    uint16_t curMP = game->read<uint16_t>(BattleOffsets::Enemies[0] + BattleOffsets::CurrentMP);
    uint16_t maxMP = game->read<uint16_t>(BattleOffsets::Enemies[0] + BattleOffsets::MaxMP);
    float mpPercent = maxMP > 0 ? (float)curMP / (float)maxMP : 0.0f;
    std::string mpText = std::to_string(curMP) + "/" + std::to_string(maxMP);
    drawProgressBar("MP", mpPercent, mpText, ImVec4(0.15f, 0.35f, 0.85f, 1.0f));

    float atbPercent = (float)enemy1ATB / UINT16_MAX;
    std::string enemy1ATBText = std::to_string((int)(atbPercent * 100.0f)) + "%";
    drawProgressBar("ATB", atbPercent, enemy1ATBText, ImVec4(0.85f, 0.70f, 0.15f, 1.0f));

    ImGui::Checkbox("Control", &controlEnemy1);

    if (controlEnemy1 && !previousControlEnemy1)
    {
        if (enemy1ATBRate != 0)
        {
            savedEnemy1ATBRate = enemy1ATBRate;
        }

        hasSavedEnemy1ATBRate = true;

        if (enemy1ATB < UINT16_MAX)
        {
            game->write<uint16_t>(Enemy1ATBRateOffset, 0);
            enemy1ATBRate = 0;
        }
    }
    else if (!controlEnemy1 && previousControlEnemy1)
    {
        if (enemy1ScriptOverwrite.isApplied())
        {
            enemy1ScriptOverwrite.restore();
        }

        if (hasSavedEnemy1ATBRate)
        {
            game->write<uint16_t>(Enemy1ATBRateOffset, savedEnemy1ATBRate);
            enemy1ATBRate = savedEnemy1ATBRate;
        }

        enemy1ActionSubmitted = false;
    }

    if (controlEnemy1)
    {
        bool atbLoopedAfterAction = enemy1ActionSubmitted && enemy1ATB < lastEnemy1ATB;
        if (atbLoopedAfterAction)
        {
            game->write<uint16_t>(Enemy1ATBRateOffset, 0);
            enemy1ATBRate = 0;
            enemy1ActionSubmitted = false;
        }

        if (!enemy1ActionSubmitted && enemy1ATB < UINT16_MAX && enemy1ATBRate != 0)
        {
            if (!hasSavedEnemy1ATBRate)
            {
                savedEnemy1ATBRate = enemy1ATBRate;
                hasSavedEnemy1ATBRate = true;
            }

            game->write<uint16_t>(Enemy1ATBRateOffset, 0);
            enemy1ATBRate = 0;
        }
    }

    bool canSubmitAction = controlEnemy1 && !enemy1ActionSubmitted && enemy1ATB < UINT16_MAX && enemy1ATBRate == 0;
    std::string controlStatus = canSubmitAction ? "Ready" : "Waiting";
    ImGui::Text("Control State: %s", controlStatus.c_str());

    ImGui::SeparatorText("Attack Submission");
    {
        ImGui::BeginDisabled(!canSubmitAction);
        const float fieldOffset = DPI(95.0f);
        const float fieldWidth = DPI(300.0f);

        ImGui::Checkbox("Message", &includeMessage);
        ImGui::SameLine(fieldOffset);
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::BeginDisabled(!includeMessage);
        ImGui::InputText("##EnemyControl_Message", message, IM_ARRAYSIZE(message));
        ImGui::EndDisabled();

        ImGui::Text("Target");
        ImGui::SameLine(fieldOffset);
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::Combo("##EnemyControl_Target", &selectedTarget, targetOptions.data(), (int)targetOptions.size());

        ImGui::Text("Attack");
        ImGui::SameLine(fieldOffset);
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::Combo("##EnemyControl_Attack", &selectedAttack, attackOptionNames, IM_ARRAYSIZE(attackOptionNames));

        if (ImGui::Button("Submit"))
        {
            if (enemy1ScriptOverwrite.isApplied())
            {
                enemy1ScriptOverwrite.restore();
            }

            BattleScriptBuilder script;

            if (includeMessage)
            {
                script.showMessage(message).waitForMessage();
            }

            script.setTargetIndex(static_cast<uint8_t>(selectedTarget))
                .performEnemyAttack(attackOptions[selectedAttack].id)
                .end();

            enemy1ScriptOverwrite = script.writeTo(game, Enemy1MainScriptOffset);

            if (hasSavedEnemy1ATBRate)
            {
                game->write<uint16_t>(Enemy1ATBRateOffset, savedEnemy1ATBRate);
                enemy1ATBRate = savedEnemy1ATBRate;
            }

            enemy1ActionSubmitted = true;
        }
        ImGui::EndDisabled();
    }

    previousControlEnemy1 = controlEnemy1;
    lastEnemy1ATB = enemy1ATB;
}

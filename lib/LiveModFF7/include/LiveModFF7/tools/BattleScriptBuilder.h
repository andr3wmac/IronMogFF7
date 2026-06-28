#pragma once

#include "LiveModFF7/game/GameManager.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class BattleScriptOverwrite
{
public:
    BattleScriptOverwrite() = default;
    BattleScriptOverwrite(GameManager* game, uintptr_t offset, const std::vector<uint8_t>& script);

    bool apply();
    bool restore();

    bool isApplied() const { return applied; }
    uintptr_t getOffset() const { return offset; }
    size_t getSize() const { return scriptBytes.size(); }

private:
    GameManager* game = nullptr;
    uintptr_t offset = 0;
    std::vector<uint8_t> originalBytes;
    std::vector<uint8_t> scriptBytes;
    bool capturedOriginal = false;
    bool applied = false;
};

class BattleScriptBuilder
{
public:
    BattleScriptBuilder& clear();

    BattleScriptBuilder& showMessage(const std::string& text);
    BattleScriptBuilder& waitForMessage();
    BattleScriptBuilder& setTargetIndex(uint8_t targetIndex);
    BattleScriptBuilder& setTargetMask(uint16_t targetMask);
    BattleScriptBuilder& performEnemyAttack(uint16_t attackId);
    BattleScriptBuilder& end();

    const std::vector<uint8_t>& getBytes() const { return bytes; }
    size_t size() const { return bytes.size(); }

    BattleScriptOverwrite writeTo(GameManager* game, uintptr_t offset) const;

private:
    BattleScriptBuilder& pushValue8(uint8_t value);
    BattleScriptBuilder& pushValue16(uint16_t value);
    BattleScriptBuilder& pushAddress16(uint16_t address);
    BattleScriptBuilder& store();

    void append8(uint8_t value);
    void append16(uint16_t value);

    std::vector<uint8_t> bytes;
};

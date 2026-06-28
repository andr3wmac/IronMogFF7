#include "LiveModFF7/tools/BattleScriptBuilder.h"
#include "LiveModFF7/game/GameData.h"

BattleScriptOverwrite::BattleScriptOverwrite(GameManager* game, uintptr_t offset, const std::vector<uint8_t>& script)
    : game(game), offset(offset), scriptBytes(script)
{
}

bool BattleScriptOverwrite::apply()
{
    if (game == nullptr || scriptBytes.empty())
    {
        return false;
    }

    if (!capturedOriginal)
    {
        originalBytes.resize(scriptBytes.size());
        if (!game->read(offset, originalBytes.size(), originalBytes.data()))
        {
            originalBytes.clear();
            return false;
        }

        capturedOriginal = true;
    }

    game->write(offset, scriptBytes.data(), scriptBytes.size());
    applied = true;
    return true;
}

bool BattleScriptOverwrite::restore()
{
    if (game == nullptr || !capturedOriginal || originalBytes.empty())
    {
        return false;
    }

    game->write(offset, originalBytes.data(), originalBytes.size());
    applied = false;
    return true;
}

BattleScriptBuilder& BattleScriptBuilder::clear()
{
    bytes.clear();
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::showMessage(const std::string& text)
{
    append8(0x93);

    std::vector<uint8_t> encodedText = GameData::encodeString(text);
    bytes.insert(bytes.end(), encodedText.begin(), encodedText.end());
    append8(0xFF);

    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::waitForMessage()
{
    append8(0x24);
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::setTargetIndex(uint8_t targetIndex)
{
    return setTargetMask(static_cast<uint16_t>(1 << targetIndex));
}

BattleScriptBuilder& BattleScriptBuilder::setTargetMask(uint16_t targetMask)
{
    pushAddress16(0x2070);
    pushValue16(targetMask);
    store();
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::performEnemyAttack(uint16_t attackId)
{
    pushValue8(0x20);
    pushValue16(attackId);
    append8(0x92);
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::end()
{
    append8(0x73);
    return *this;
}

BattleScriptOverwrite BattleScriptBuilder::writeTo(GameManager* game, uintptr_t offset) const
{
    BattleScriptOverwrite overwrite(game, offset, bytes);
    overwrite.apply();
    return overwrite;
}

BattleScriptBuilder& BattleScriptBuilder::pushValue8(uint8_t value)
{
    append8(0x60);
    append8(value);
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::pushValue16(uint16_t value)
{
    append8(0x61);
    append16(value);
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::pushAddress16(uint16_t address)
{
    append8(0x12);
    append16(address);
    return *this;
}

BattleScriptBuilder& BattleScriptBuilder::store()
{
    append8(0x90);
    return *this;
}

void BattleScriptBuilder::append8(uint8_t value)
{
    bytes.push_back(value);
}

void BattleScriptBuilder::append16(uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

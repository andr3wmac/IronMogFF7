#include "NoSummons.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "rules/Restrictions.h"

REGISTER_RULE(NoSummons, "No Summons", "Summon materia cannot be found or purchased.")

void NoSummons::setup()
{
    BIND_EVENT(game->onStart, NoSummons::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, NoSummons::onFieldChanged);
    BIND_EVENT(game->onShopOpened, NoSummons::onShopOpened);

    // Ban all summon materia, this will prevent them from being chosen by any randomizers.
    Restrictions::banMateria(20); // W-Summon
    Restrictions::banMateria(74); // Choco/Mog
    Restrictions::banMateria(75); // Shiva
    Restrictions::banMateria(76); // Ifrit
    Restrictions::banMateria(77); // Ramuh
    Restrictions::banMateria(78); // Titan
    Restrictions::banMateria(79); // Odin
    Restrictions::banMateria(80); // Leviathan
    Restrictions::banMateria(81); // Bahamut
    Restrictions::banMateria(82); // Kjata
    Restrictions::banMateria(83); // Alexander
    Restrictions::banMateria(84); // Phoenix
    Restrictions::banMateria(85); // Neo Bahamut
    Restrictions::banMateria(86); // Hades
    Restrictions::banMateria(87); // Typoon
    Restrictions::banMateria(88); // Bahamut ZERO
    Restrictions::banMateria(89); // Knights of Round
    Restrictions::banMateria(90); // Master Summon

    if (!game->isRuleEnabled("Randomize Field Items"))
    {
        needsFieldChecks = true;
    }

    if (!game->isRuleEnabled("Randomize Shops"))
    {
        needsShopChecks = true;
    }
}

void NoSummons::onStart()
{
    rng.seed(game->getSeed());
}

void NoSummons::onFieldChanged(uint16_t fieldID)
{
    if (!needsFieldChecks)
    {
        return;
    }

    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    // Randomize materia
    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldScriptItem& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;

        uint8_t oldMateriaID = game->read<uint8_t>(idOffset);
        if (oldMateriaID != materia.id)
        {
            // Data isn't loaded yet or has been changed by something else.
            continue;
        }

        if (!Restrictions::isMateriaBanned(oldMateriaID))
        {
            continue;
        }

        uint8_t newMateriaID = (uint8_t)replaceSummonMateria(oldMateriaID);
        game->write<uint8_t>(idOffset, newMateriaID);

        std::string oldMateriaName = GameData::getMateriaName(oldMateriaID);
        std::string newMateriaName = GameData::getMateriaName(newMateriaID);
        LOG("Randomized materia on field %d: %s changed to: %s", fieldID, oldMateriaName.c_str(), newMateriaName.c_str());

        // Overwrite the popup message
        int msgIndex = game->findPickUpMessage(oldMateriaName, materia.group, materia.script, materia.offset);
        if (msgIndex >= 0)
        {
            FieldScriptMessage& fieldMsg = fieldData.messages[msgIndex];
            game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, newMateriaName);
        }
    }
}

void NoSummons::onShopOpened()
{
    if (!needsShopChecks)
    {
        return;
    }

    FieldData fieldData = GameData::getField(game->getFieldID());
    if (!fieldData.isValid())
    {
        return;
    }

    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        uint8_t shopID = fieldData.shops[i].shopID;
        uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * shopID);
        uint8_t invCount = game->read<uint8_t>(shopOffset + 2);

        for (int j = 0; j < invCount; ++j)
        {
            uintptr_t itemOffset = shopOffset + 4 + (j * 8);
            uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
            
            // Skip if not materia
            if (itemType != 1)
            {
                continue;
            }

            uint16_t materiaID = game->read<uint16_t>(itemOffset + 4);
            uint16_t newMatriaID = replaceSummonMateria(materiaID);

            std::string oldMateriaName = GameData::getMateriaName((uint8_t)materiaID);
            std::string newMateriaName = GameData::getMateriaName((uint8_t)newMatriaID);
            LOG("Replaced summon materia in shop %d: %s changed to: %s", shopID, oldMateriaName.c_str(), newMateriaName.c_str());

            uintptr_t priceOffset = ShopOffsets::MateriaPricesStart + (newMatriaID * 4);
            uint32_t newPrice = game->read<uint32_t>(ShopOffsets::MateriaPricesStart + (materiaID * 4));

            if (priceOffset > 0)
            {
                game->write<uint16_t>(itemOffset + 4, newMatriaID);
                game->write<uint32_t>(priceOffset, newPrice);
            }
        }
    }
}

uint16_t NoSummons::replaceSummonMateria(uint16_t materiaID)
{
    return GameData::getRandomMateria(rng);
}
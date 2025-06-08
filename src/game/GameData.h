#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct ItemData
{
    uint8_t id = 0xFF;
    std::string name = "";

    bool isValid() { return id != 0xFF; }
};

class GameData
{
public:
    static void loadGameData();

    static ItemData getAccessory(uint8_t id);
    static ItemData getArmor(uint8_t id);
    static ItemData getItem(uint8_t id);
    static ItemData getWeapon(uint8_t id);

private:
    static std::unordered_map<uint8_t, ItemData> accessoryData;
    static std::unordered_map<uint8_t, ItemData> armorData;
    static std::unordered_map<uint8_t, ItemData> itemData;
    static std::unordered_map<uint8_t, ItemData> weaponData;
};

#define ADD_ACCESSORY(X, Y) accessoryData[X] = {X, Y};
#define ADD_ARMOR(X, Y) armorData[X] = {X, Y};
#define ADD_ITEM(X, Y) itemData[X] = {X, Y};
#define ADD_WEAPON(X, Y) weaponData[X] = {X, Y};
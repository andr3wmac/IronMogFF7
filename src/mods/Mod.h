#pragma once

#include "LiveModFF7Core/game/GameManager.h"
#include "utilities/ConfigFile.h"

enum class ModDescriptionType : uint8_t
{
    Negation    = 0,
    Randomized  = 1,
    Multiplier  = 2,
    Unique      = 3,
    BanItems    = 4,
    BanMateria  = 5
};

class Mod
{
public:
    virtual ~Mod() = default;
    bool enabled = true;
    std::string name = "";
    std::string description = "";
    bool debugVisible = false;

    virtual void setup() {}
    virtual bool hasSettings() { return false; }
    virtual bool onSettingsGUI() { return false; }
    virtual void loadSettings(const ConfigFile& cfg) {}
    virtual void saveSettings(ConfigFile& cfg) {}
    virtual bool hasDebugGUI() { return false; }
    virtual void onDebugGUI() { }
    virtual std::vector<std::string> describe(ModDescriptionType descType) { return {}; }

    void setManager(GameManager* gameManager)
    {
        game = gameManager;
    }

protected:
    GameManager* game = nullptr;

public:
    static std::vector<Mod*>& getList()
    {
        static std::vector<Mod*> list;
        return list;
    }

    static void registerMod(Mod* mod)
    {
        getList().push_back(mod);
    }
};

#define REGISTER_MOD(ClassName, NameStr, DescStr) \
    namespace { \
        struct ClassName##AutoRegister { \
            ClassName##AutoRegister() { \
                ClassName* tmp = new ClassName(); \
                tmp->name = NameStr; \
                tmp->description = DescStr; \
                Mod::registerMod(tmp); \
            } \
        }; \
        static ClassName##AutoRegister _autoRegister_##ClassName; \
    }

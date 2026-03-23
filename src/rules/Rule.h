#pragma once

#include "core/game/GameManager.h"
#include "core/utilities/ConfigFile.h"

enum class RuleDescripionType : uint8_t
{
    Negation    = 0,
    Randomized  = 1,
    Multiplier  = 2,
    Unique      = 3,
    BanItems    = 4,
    BanMateria  = 5
};

class Rule
{
public:
    bool enabled = true;
    std::string name = "";
    std::string description = "";
    bool settingsVisible = false;
    bool debugVisible = false;

    virtual void setup() {}
    virtual bool hasSettings() { return false; }
    virtual bool onSettingsGUI() { return false; }
    virtual void loadSettings(const ConfigFile& cfg) {}
    virtual void saveSettings(ConfigFile& cfg) {}
    virtual bool hasDebugGUI() { return false; }
    virtual void onDebugGUI() { }
    virtual std::vector<std::string> describe(RuleDescripionType descType) { return {}; }

    void setManager(GameManager* gameManager)
    {
        game = gameManager;
    }

protected:
    GameManager* game = nullptr;

public:
    static std::vector<Rule*>& getList()
    {
        static std::vector<Rule*> list;
        return list;
    }

    static void registerRule(std::string name, Rule* rule)
    {
        getList().push_back(rule);
    }
};

#define REGISTER_RULE(ClassName, NameStr, DescStr) \
    namespace { \
        struct ClassName##AutoRegister { \
            ClassName##AutoRegister() { \
                ClassName* tmp = new ClassName(); \
                tmp->name = NameStr; \
                tmp->description = DescStr; \
                Rule::registerRule(NameStr, tmp); \
            } \
        }; \
        static ClassName##AutoRegister _autoRegister_##ClassName; \
    }
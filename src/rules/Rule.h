#pragma once

#include "core/game/GameManager.h"
#include "core/utilities/ConfigFile.h"

class Rule
{
public:
    bool enabled = true;
    std::string name = "";
    bool settingsVisible = false;
    bool debugVisible = false;

    virtual void setup() {}
    virtual bool hasSettings() { return false; }
    virtual bool onSettingsGUI() { return false; }
    virtual void loadSettings(const ConfigFile& cfg) {}
    virtual void saveSettings(ConfigFile& cfg) {}
    virtual bool hasDebugGUI() { return false; }
    virtual void onDebugGUI() { }

    void setManager(GameManager* gameManager)
    {
        game = gameManager;
    }

protected:
    GameManager* game;

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

#define REGISTER_RULE(NameStr, ClassName) \
    namespace { \
        struct ClassName##AutoRegister { \
            ClassName##AutoRegister() { \
                ClassName* tmp = new ClassName(); \
                tmp->name = NameStr; \
                Rule::registerRule(NameStr, tmp); \
            } \
        }; \
        static ClassName##AutoRegister _autoRegister_##ClassName; \
    }
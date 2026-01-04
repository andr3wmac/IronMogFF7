#pragma once

#include "core/game/GameManager.h"
#include "core/utilities/ConfigFile.h"

class Extra
{
public:
    bool enabled = true;
    std::string name = "";
    bool settingsVisible = false;

    virtual void setup() {}
    virtual bool hasSettings() { return false; }
    virtual void onSettingsGUI() { }
    virtual void loadSettings(const ConfigFile& cfg) {}
    virtual void saveSettings(ConfigFile& cfg) {}
    virtual bool hasDebugGUI() { return false; }
    virtual void onDebugGUI() {}

    void setManager(GameManager* gameManager)
    {
        game = gameManager;
    }

protected:
    GameManager* game;

public:
    static std::vector<Extra*>& getList()
    {
        static std::vector<Extra*> list;
        return list;
    }

    static void registerExtra(std::string name, Extra* extra)
    {
        getList().push_back(extra);
    }
};

#define REGISTER_EXTRA(NameStr, ClassName) \
    namespace { \
        struct ClassName##AutoRegister { \
            ClassName##AutoRegister() { \
                ClassName* tmp = new ClassName(); \
                tmp->name = NameStr; \
                Extra::registerExtra(NameStr, tmp); \
            } \
        }; \
        static ClassName##AutoRegister _autoRegister_##ClassName; \
    }
#pragma once

#include "game/GameManager.h"

class Rule
{
public:
    std::string name = "";

    virtual void setup() {}

    void setManager(GameManager* gameManager)
    {
        game = gameManager;
    }

protected:
    GameManager* game;

public:
    static std::unordered_map<std::string, std::function<Rule*()>>& registry()
    {
        static std::unordered_map<std::string, std::function<Rule*()>> factories;
        return factories;
    }

    static void registerRule(std::string name, std::function<Rule*()> factory)
    {
        registry()[name] = std::move(factory);
    }
};

#define REGISTER_RULE(NameStr, ClassName) \
    namespace { \
        struct ClassName##AutoRegister { \
            ClassName##AutoRegister() { \
                Rule::registerRule(NameStr, [] { ClassName* tmp = new ClassName(); tmp->name = NameStr; return tmp; }); \
            } \
        }; \
        static ClassName##AutoRegister _autoRegister_##ClassName; \
    }
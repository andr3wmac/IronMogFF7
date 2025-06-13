#pragma once

#include "game/GameManager.h"

class Rule
{
public:
    virtual void onStart() {}

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
                Rule::registerRule(NameStr, [] { return new ClassName(); }); \
            } \
        }; \
        static ClassName##AutoRegister _autoRegister_##ClassName; \
    }
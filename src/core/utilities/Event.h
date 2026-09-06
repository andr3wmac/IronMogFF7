#pragma once

#include "core/utilities/Logging.h"

#include <cstring>
#include <functional>
#include <vector>

template<typename... Args>
class Event 
{
    public:
        using Callback = std::function<void(Args...)>;

        // Listeners are identified by the object they belong to and the name of the bound method, so binding 
        // the same method to the same event twice is rejected instead of silently invoking it twice.
        bool addListener(const void* owner, const char* id, const Callback& callback) 
        {
            for (const Listener& listener : listeners)
            {
                if (listener.owner == owner && std::strcmp(listener.id, id) == 0)
                {
                    LOG("Ignored duplicate event listener: %s", id);
                    return false;
                }
            }

            listeners.push_back({ owner, id, callback });
            return true;
        }

        void invoke(Args... args) const 
        {
            for (const auto& listener : listeners) 
            {
                listener.callback(args...);
            }
        }

    private:
        struct Listener
        {
            const void* owner;  // Object the method is bound to.
            const char* id;     // Stringified method name, always a literal.
            Callback callback;
        };

        std::vector<Listener> listeners;
};

#define BIND_EVENT(EVENT, FUNC) EVENT.addListener(this, #FUNC, std::bind(&FUNC, this));
#define BIND_EVENT_ONE_ARG(EVENT, FUNC) EVENT.addListener(this, #FUNC, std::bind(&FUNC, this, std::placeholders::_1));
#define BIND_EVENT_TWO_ARG(EVENT, FUNC) EVENT.addListener(this, #FUNC, std::bind(&FUNC, this, std::placeholders::_1, std::placeholders::_2));
#pragma once

#include <functional>
#include <vector>

template<typename... Args>
class Event 
{
    public:
        using Callback = std::function<void(Args...)>;

        void addListener(const Callback& callback) 
        {
            listeners.push_back(callback);
        }

        void invoke(Args... args) const 
        {
            for (const auto& listener : listeners) 
            {
                listener(args...);
            }
        }

    private:
        std::vector<Callback> listeners;
};

#define BIND_EVENT(EVENT, FUNC) EVENT.addListener(std::bind(&FUNC, this));
#define BIND_EVENT_ONE_ARG(EVENT, FUNC) EVENT.addListener(std::bind(&FUNC, this, std::placeholders::_1));
#define BIND_EVENT_TWO_ARG(EVENT, FUNC) EVENT.addListener(std::bind(&FUNC, this, std::placeholders::_1, std::placeholders::_2));
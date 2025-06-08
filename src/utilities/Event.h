#pragma once

#include <functional>
#include <vector>

template<typename... Args>
class Event 
{
    public:
        using Callback = std::function<void(Args...)>;

        void AddListener(const Callback& callback) 
        {
            listeners.push_back(callback);
        }

        void Invoke(Args... args) const 
        {
            for (const auto& listener : listeners) 
            {
                listener(args...);
            }
        }

    private:
        std::vector<Callback> listeners;
};

#define BIND_EVENT(EVENT, FUNC) EVENT.AddListener(std::bind(&FUNC, this));
#define BIND_EVENT_ONE_ARG(EVENT, FUNC) EVENT.AddListener(std::bind(&FUNC, this, std::placeholders::_1));
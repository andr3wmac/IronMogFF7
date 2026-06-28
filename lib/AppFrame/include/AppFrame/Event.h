#pragma once

#include <functional>
#include <vector>

namespace AppFrame
{
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
}

#pragma once

#include <functional>

template<typename EventType>
class SimpleEventQueueSingleton
{
public:
    using Event = EventType;
    using Listener = std::function<void(const Event&)>;
    using Instance = SimpleEventQueueSingleton<Event>;
    enum class ListenerHandle : size_t { Invalid = -1 };

    static Instance& GetInstance()
    {
        static Instance instance{};
        return instance;
    }

    [[nodiscard]] ListenerHandle RegisterListener(Listener&& listener)
    {
        m_Listeners.push_back(std::move(listener));
        return static_cast<ListenerHandle>(m_Listeners.size() - 1);
    }

    void UnregisterListener(const ListenerHandle handle)
    {
        if(handle <= ListenerHandle::Invalid)
            return;

        m_Listeners.erase(std::begin(m_Listeners) + static_cast<size_t>(handle));
    }

    void QueueEvent(Event&& event)
    {
        m_Events.push_back(std::move(event));
    }

    /// Does not support queueing events or adding/removing listeners during dispatch.
    void DispatchEvents()
    {
        for(const Listener& listener : m_Listeners)
        {
            for(const Event& event : m_Events)
            {
                listener(event);
            }
        }

        m_Events.clear();
    }
private:
    SimpleEventQueueSingleton() = default;
    SimpleEventQueueSingleton(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    std::vector<Listener> m_Listeners{};
    std::vector<Event> m_Events{};
};

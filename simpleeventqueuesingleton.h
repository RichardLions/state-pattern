#pragma once

#include <functional>
#include <algorithm>

template<typename EventType>
class SimpleEventQueueSingleton
{
public:
    using Event = EventType;
    using Listener = std::function<void(const Event&)>;
    using Instance = SimpleEventQueueSingleton<Event>;
    enum class ListenerHandle : size_t { Invalid = -1 };

    struct ListenerRegistration
    {
        explicit ListenerRegistration(Listener&& listener, const ListenerHandle handle)
            : m_Listener{std::move(listener)}
            , m_Handle{handle}
        {
        }

        Listener m_Listener{};
        ListenerHandle m_Handle{ListenerHandle::Invalid};
    };

    static Instance& GetInstance()
    {
        static Instance instance{};
        return instance;
    }

    [[nodiscard]] ListenerHandle RegisterListener(Listener&& listener)
    {
        const ListenerHandle handle{m_Listeners.size()};
        m_Listeners.emplace_back(std::move(listener), handle);
        return handle;
    }

    void UnregisterListener(const ListenerHandle handle)
    {
        std::erase_if(
            m_Listeners,
            [handle](const ListenerRegistration& listenerRegistration)
            {
                return listenerRegistration.m_Handle == handle;
            });
    }

    void QueueEvent(Event&& event)
    {
        m_Events.push_back(std::move(event));
    }

    /// Does not support queueing events or adding/removing listeners during dispatch.
    void DispatchEvents()
    {
        for(const ListenerRegistration& listenerRegistration : m_Listeners)
        {
            for(const Event& event : m_Events)
            {
                listenerRegistration.m_Listener(event);
            }
        }

        m_Events.clear();
    }
private:
    SimpleEventQueueSingleton() = default;
    SimpleEventQueueSingleton(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    std::vector<ListenerRegistration> m_Listeners{};
    std::vector<Event> m_Events{};
};

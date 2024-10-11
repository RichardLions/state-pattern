#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "finitestatemachine.h"
#include "simpleeventqueuesingleton.h"

class Light
{
public:
    void TurnOn() { m_On = true; }
    void TurnOff() { m_On = false; }
    bool IsOn() const { return m_On; }
    bool IsOff() const { return !m_On; }
private:
    bool m_On{false};
};

struct LightSwitchOnEvent
{
    LightSwitchOnEvent(std::shared_ptr<Light> light)
        : m_Light{std::move(light)}
    {
    }

    std::shared_ptr<Light> m_Light{};
};

struct LightSwitchOffEvent
{
    LightSwitchOffEvent(std::shared_ptr<Light> light)
        : m_Light{std::move(light)}
    {
    }

    std::shared_ptr<Light> m_Light{};
};

using LightSwitchOnEventQueue = SimpleEventQueueSingleton<LightSwitchOnEvent>;
using LightSwitchOffEventQueue = SimpleEventQueueSingleton<LightSwitchOffEvent>;

class LightOffState final : public StateModel<LightOffState, Light>
{
public:
    ~LightOffState()
    {
        LightSwitchOnEventQueue::GetInstance().UnregisterListener(m_ListenerHandle);
    }

    void OnEnter(Light& light) override
    {
        m_ListenerHandle = LightSwitchOnEventQueue::GetInstance().RegisterListener(
            [&light](const LightSwitchOnEventQueue::Event& event)
            {
                if(&light == event.m_Light.get())
                    light.TurnOn();
            });
    }

    void Update(Light&) override { }
    void OnExit(Light&) override { }
private:
    LightSwitchOnEventQueue::ListenerHandle m_ListenerHandle{LightSwitchOnEventQueue::ListenerHandle::Invalid};
};

class LightOnState final : public StateModel<LightOnState, Light>
{
public:
    ~LightOnState()
    {
        LightSwitchOffEventQueue::GetInstance().UnregisterListener(m_ListenerHandle);
    }

    void OnEnter(Light& light) override
    {
        m_ListenerHandle = LightSwitchOffEventQueue::GetInstance().RegisterListener(
            [&light](const LightSwitchOffEventQueue::Event& event)
            {
                if(&light == event.m_Light.get())
                    light.TurnOff();
            });
    }

    void Update(Light&) override { }
    void OnExit(Light&) override { }
private:
    LightSwitchOffEventQueue::ListenerHandle m_ListenerHandle{LightSwitchOffEventQueue::ListenerHandle::Invalid};
};

TEST_CASE("Finite State Machine - Unit Tests")
{
    LightOffState::AddTransitionGuard<LightOnState>(
        [](const LightOffState&, const Light& light)
        {
            return light.IsOn();
        });

    LightOnState::AddTransitionGuard<LightOffState>(
        [](const LightOnState&, const Light& light)
        {
            return light.IsOff();
        });

    std::shared_ptr<Light> light{std::make_shared<Light>()};
    FiniteStateMachine<Light> stateMachine{std::make_unique<LightOffState>(), light};
    REQUIRE(light->IsOff());

    // Event handled
    LightSwitchOnEventQueue::GetInstance().QueueEvent(LightSwitchOnEvent{light});
    LightSwitchOnEventQueue::GetInstance().DispatchEvents();
    REQUIRE(light->IsOn());

    // Event unhandled
    LightSwitchOffEventQueue::GetInstance().QueueEvent(LightSwitchOffEvent{light});
    LightSwitchOnEventQueue::GetInstance().DispatchEvents();
    REQUIRE(light->IsOn());

    // State transition LightOffState -> LightOnState
    stateMachine.Update();
    REQUIRE(light->IsOn());

    // Event handled
    LightSwitchOffEventQueue::GetInstance().QueueEvent(LightSwitchOffEvent{light});
    LightSwitchOffEventQueue::GetInstance().DispatchEvents();
    REQUIRE(light->IsOff());

    // State transition LightOnState -> LightOffState
    stateMachine.Update();
    REQUIRE(light->IsOff());

    // Event ignored, different light
    LightSwitchOnEventQueue::GetInstance().QueueEvent(LightSwitchOnEvent{std::make_shared<Light>()});
    LightSwitchOnEventQueue::GetInstance().DispatchEvents();
    REQUIRE(light->IsOff());

    // Event handled
    LightSwitchOnEventQueue::GetInstance().QueueEvent(LightSwitchOnEvent{light});
    LightSwitchOnEventQueue::GetInstance().DispatchEvents();
    REQUIRE(light->IsOn());

    LightOffState::ClearStateTransitions();
    LightOnState::ClearStateTransitions();
}

TEST_CASE("Finite State Machine - Benchmarks")
{
    BENCHMARK("Benchmark")
    {
        constexpr uint32_t updateCount{100'000};

        LightOffState::AddTransitionGuard<LightOnState>(
            [](const LightOffState&, const Light& light)
            {
                return light.IsOn();
            });

        LightOnState::AddTransitionGuard<LightOffState>(
            [](const LightOnState&, const Light& light)
            {
                return light.IsOff();
            });

        std::shared_ptr<Light> light{std::make_shared<Light>()};
        FiniteStateMachine<Light> stateMachine{std::make_unique<LightOffState>(), light};

        for(uint32_t i{0}; i != updateCount; ++i)
        {
            LightSwitchOnEventQueue::GetInstance().QueueEvent(LightSwitchOnEvent{light});
            LightSwitchOnEventQueue::GetInstance().DispatchEvents();

            stateMachine.Update();

            LightSwitchOffEventQueue::GetInstance().QueueEvent(LightSwitchOffEvent{light});
            LightSwitchOnEventQueue::GetInstance().DispatchEvents();

            stateMachine.Update();
        }

        LightOffState::ClearStateTransitions();
        LightOnState::ClearStateTransitions();
    };
}

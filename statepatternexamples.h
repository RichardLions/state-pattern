#pragma once

#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <memory>

#include <iostream>

class State;

template<typename T>
using StateGuard = std::function<bool(const T&)>;

using StateCreator = std::function<std::unique_ptr<State>()>;

template<typename T>
using StateTransition = std::pair<StateGuard<T>, StateCreator>;

template<typename T>
using StateTransitions = std::vector<StateTransition<T>>;

class State
{
public:
    virtual ~State() = default;
    virtual void OnEnter() = 0;
    virtual void Update() = 0;
    virtual void OnExit() = 0;
    virtual std::unique_ptr<State> CheckGuards() const = 0;
};

template<typename T>
class StateGuards : public State
{
public:
    [[nodiscard]] std::unique_ptr<State> CheckGuards() const final
    {
        for(auto& [guard, creator] : ms_Transitions)
        {
            if(guard(static_cast<const T&>(*this)))
            {
                return creator();
            }
        }

        return nullptr;
    }

    template<typename ToState>
    static void AddStateTransition(StateGuard<T>&& guard)
    {
        ms_Transitions.emplace_back(
            std::move(guard),
            []{ return std::make_unique<ToState>(); });
    }

private:
    static StateTransitions<T> ms_Transitions;
};

template<typename T>
StateTransitions<T> StateGuards<T>::ms_Transitions{};

class FiniteStateMachine
{
public:
    explicit FiniteStateMachine(std::unique_ptr<State>&& state)
        : m_State{std::move(state)}
    {
        m_State->OnEnter();
    }

    void Update()
    {
        m_State->Update();
        if(std::unique_ptr<State> state{m_State->CheckGuards()})
        {
            m_State->OnExit();
            m_State = std::move(state);
            m_State->OnEnter();
        }
    }

private:
    std::unique_ptr<State> m_State{};
};

class StateA final : public StateGuards<StateA>
{
public:
    void OnEnter() override { std::cout << "StateA::OnEnter\n"; }
    void Update() override { std::cout << "StateA::Update\n"; }
    void OnExit() override { std::cout << "StateA::OnExit\n"; }
};

class StateB final : public StateGuards<StateB>
{
public:
    void OnEnter() override { std::cout << "StateB::OnEnter\n"; }
    void Update() override { std::cout << "StateB::Update\n"; }
    void OnExit() override { std::cout << "StateB::OnExit\n"; }
};

class StateC final : public StateGuards<StateC>
{
public:
    void OnEnter() override { std::cout << "StateC::OnEnter\n"; }
    void Update() override { std::cout << "StateC::Update\n"; }
    void OnExit() override { std::cout << "StateC::OnExit\n"; }
};

TEST_CASE("Finite State Machine - Unit Tests")
{
    StateA::AddStateTransition<StateB>([](const StateA&){ return true; });
    StateB::AddStateTransition<StateC>([](const StateB&){ return true; });
    StateC::AddStateTransition<StateA>([](const StateC&){ return true; });

    FiniteStateMachine stateMachine{std::make_unique<StateA>()};
    for(;;)
    {
        stateMachine.Update();
    }
}

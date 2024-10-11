#pragma once

#include <functional>
#include <memory>

template<typename TOwner>
class StateConcept;

template<typename TState, typename TOwner>
using StateGuard = std::function<bool(const TState&, const TOwner&)>;

template<typename TOwner>
using StateCreator = std::function<std::unique_ptr<StateConcept<TOwner>>()>;

template<typename TState, typename TOwner>
using StateTransitionGuard = std::pair<StateGuard<TState, TOwner>, StateCreator<TOwner>>;

template<typename TState, typename TOwner>
using StateTransitionGuards = std::vector<StateTransitionGuard<TState, TOwner>>;

template<typename TOwner>
class StateConcept
{
public:
    virtual ~StateConcept() = default;
    virtual void OnEnter(TOwner&) = 0;
    virtual void Update(TOwner&) = 0;
    virtual void OnExit(TOwner&) = 0;
    virtual std::unique_ptr<StateConcept> CheckTransitionGuards(TOwner&) const = 0;
};

template<typename TState, typename TOwner>
class StateModel : public StateConcept<TOwner>
{
public:
    [[nodiscard]] std::unique_ptr<StateConcept<TOwner>> CheckTransitionGuards(TOwner& owner) const final
    {
        for(const auto& [guard, creator] : ms_TransitionGuards)
        {
            if(guard(static_cast<const TState&>(*this), owner))
            {
                return creator();
            }
        }

        return nullptr;
    }

    template<typename ToState>
    static void AddTransitionGuard(StateGuard<TState, TOwner>&& guard)
    {
        ms_TransitionGuards.emplace_back(
            std::move(guard),
            []{ return std::make_unique<ToState>(); });
    }

    static void ClearStateTransitions()
    {
        ms_TransitionGuards.clear();
    }
private:
    static StateTransitionGuards<TState, TOwner> ms_TransitionGuards;
};

template<typename TState, typename TOwner>
StateTransitionGuards<TState, TOwner> StateModel<TState, TOwner>::ms_TransitionGuards{};

template<typename TOwner>
class FiniteStateMachine
{
public:
    explicit FiniteStateMachine(std::unique_ptr<StateConcept<TOwner>>&& state, std::shared_ptr<TOwner> owner)
        : m_State{std::move(state)}
        , m_Owner{std::move(owner)}
    {
        m_State->OnEnter(*m_Owner);
    }

    void Update()
    {
        m_State->Update(*m_Owner);
        if(std::unique_ptr<StateConcept<TOwner>> state{m_State->CheckTransitionGuards(*m_Owner)})
        {
            m_State->OnExit(*m_Owner);
            m_State = std::move(state);
            m_State->OnEnter(*m_Owner);
        }
    }
private:
    std::unique_ptr<StateConcept<TOwner>> m_State{};
    std::shared_ptr<TOwner> m_Owner{};
};

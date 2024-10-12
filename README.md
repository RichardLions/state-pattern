# State Pattern (Finite State Machine)

This pattern was inspired by [Game Programming Patterns](https://gameprogrammingpatterns.com/state.html).

## When To Use

The State pattern is used to reduce the complexity of managing the state an object or system can be in. Using flags and enumerations to manage state for complex behaviours can quickly become hard to reason with and costly to extend and maintain. This pattern alleviates these problems by splitting state up into independent pieces of logic and data. By using state transition guards, the transitions between states can be loosing coupled and programmatically declared. Making state transitions easy to reason with.

This pattern can also be used to reduce dependencies on objects by moving state control logic outside of the object itself.

## Example

Example finite state machine that switches a light on and off.

```cpp
class Light
{
    ...
};

class LightOffState final : public StateModel<LightOffState, Light>
{
public:
    void OnEnter(Light& light) override
    {
        LightSwitchOnEventQueue::GetInstance().RegisterListener(
            [&light](const LightSwitchOnEventQueue::Event&)
            {
                light.TurnOn();
            });
    }
    void Update(Light& light) override { ... }
    void OnExit(Light& light) override { ... }
};

class LightOnState final : public StateModel<LightOnState, Light>
{
public:
    void OnEnter(Light& light) override
    {
        LightSwitchOffEventQueue::GetInstance().RegisterListener(
            [&light](const LightSwitchOffEventQueue::Event&)
            {
                light.TurnOff();
            });
    }
    void Update(Light& light) override { ... }
    void OnExit(Light& light) override { ... }
};

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

    // Start in LightOffState
    std::shared_ptr<Light> light{std::make_shared<Light>()};
    FiniteStateMachine<Light> stateMachine{std::make_unique<LightOffState>(), light};
    REQUIRE(light->IsOff());

    LightSwitchOnEventQueue::GetInstance().QueueEvent(LightSwitchOnEvent{light});
    LightSwitchOnEventQueue::GetInstance().DispatchEvents();

    // Light is on, Transition to LightOnState
    stateMachine.Update();
    REQUIRE(light->IsOn());

    LightSwitchOffEventQueue::GetInstance().QueueEvent(LightSwitchOffEvent{light});
    LightSwitchOnEventQueue::GetInstance().DispatchEvents();

    // Light is off, Transition to LightOffState
    stateMachine.Update();
    REQUIRE(light->IsOff());
}
```

## Setup

This repository uses the .sln/.proj files created by Visual Studio 2022 Community Edition.
Using MSVC compiler, Preview version(C++23 Preview). 

### Catch2
The examples for how to use the pattern are written as Unit Tests.

Launching the program in Debug or Release will run the Unit Tests.

Alternative:
Installing the Test Adapter for Catch2 Visual Studio extension enables running the Unit Tests via the Test Explorer Window. Setup the Test Explorer to use the project's .runsettings file.

### vcpkg
This repository uses vcpkg in manifest mode for it's dependencies. To interact with vcpkg, open a Developer PowerShell (View -> Terminal).

To setup vcpkg, install it via the Visual Studio installer. To enable/disable it run these commands from the Developer PowerShell:
```
vcpkg integrate install
vcpkg integrate remove
```

To add additional dependencies run:
```
vcpkg add port [dependency name]
```

To update the version of a dependency modify the overrides section of vcpkg.json. 

To create a clean vcpkg.json and vcpkg-configuration.json file run:
```
vcpkg new --application
```

### TODO
- [x] Implementation
- [x] Unit Tests
- [x] Benchmarks

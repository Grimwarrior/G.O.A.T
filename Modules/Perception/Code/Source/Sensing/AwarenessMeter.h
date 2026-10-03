#pragma once

#include <GOAT_Perception/PerceptionProfileAsset.h>
#include <GOAT_Perception/PerceptionTypes.h>

namespace GOAT_Perception
{
    //! Meter value where a state begins. Unaware begins at zero.
    float StateEdge(const PerceptionProfileAsset& profile, AwarenessState state);

    //! The highest state a meter value has reached.
    AwarenessState StateForValue(const PerceptionProfileAsset& profile, float value);

    //! The awareness meter and the state it implies, knowing nothing about how a stimulus was sensed.
    class AwarenessMeter final
    {
    public:
        AwarenessState GetState() const { return m_state; }
        float GetValue() const { return m_value; }
        float GetSecondsSinceStimulus() const { return m_since; }

        //! Moves time on. A fill rate above zero means a stimulus is present, otherwise the meter drains. True when the state changed.
        bool Advance(const PerceptionProfileAsset& profile, float deltaTime, float fillPerSecond);

        //! Adds awareness all at once, as a sound or a hit does. True when the state changed.
        bool Add(const PerceptionProfileAsset& profile, float amount);

        //! Lifts the meter to at least the start of a state. True when the state changed.
        bool RaiseTo(const PerceptionProfileAsset& profile, AwarenessState state);

        void Reset();

    private:
        //! Raises the state to match the value. A state only falls through its forget time, never through the value.
        bool Settle(const PerceptionProfileAsset& profile);

        //! The continuous kind of drain: nothing for the hold, then a steady loss, with the state falling as the value does.
        void DrainContinuously(const PerceptionProfileAsset& profile, float deltaTime);

        float m_value = 0.0f;
        float m_since = 0.0f;
        //! Seconds after the last stimulus before a continuous drain begins: the forget time of the state it left the agent in.
        float m_hold = 0.0f;
        AwarenessState m_state = AwarenessState::Unaware;
    };
} // namespace GOAT_Perception

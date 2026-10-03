#pragma once

#include <Sensing/AgentSenses.h>
#include <Sensing/PerceptionKeys.h>

#include <GOAT/Domain/AgentId.h>

namespace GOAT_Perception
{
    //! What was last written for an agent, so a write only happens when something a tree could care about moved.
    struct PublishedState final
    {
        bool m_valid = false;
        SenseOutputs m_outputs;
    };

    //! The last known position is rewritten once the target has moved this far, or when sight of it is lost.
    inline constexpr float LastKnownWriteStep = 0.5f;

    //! The awareness value is rewritten once it has moved this fraction of the ceiling, or when the state changes.
    inline constexpr float AwarenessWriteFraction = 0.1f;

    //! Which of the state Bools are up for a state: Suspicious and Searching mean at least that state, Engaged means exactly it.
    struct StateFlags final
    {
        bool m_suspicious = false;
        bool m_searching = false;
        bool m_engaged = false;
    };

    StateFlags FlagsFor(AwarenessState state);

    bool ShouldPublishLastKnown(const AZ::Vector3& published, const AZ::Vector3& current, bool visiblePublished, bool visibleNow);

    bool ShouldPublishAwareness(float published, float current, AwarenessState publishedState, AwarenessState currentState, float engagedAt);

    //! Writes the outputs to an agent's blackboard, throttling continuous values because every write to a watched variable wakes its guards.
    void Publish(GOAT::IBlackboardSystem& blackboard, const PerceptionKeys& keys, GOAT::AgentId agent, const SenseOutputs& outputs,
        float engagedAt, PublishedState& published);
} // namespace GOAT_Perception

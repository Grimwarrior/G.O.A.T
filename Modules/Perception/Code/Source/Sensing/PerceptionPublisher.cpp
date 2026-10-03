#include <Sensing/PerceptionPublisher.h>

#include <AzCore/std/math.h>

namespace GOAT_Perception
{
    StateFlags FlagsFor(AwarenessState state)
    {
        const AZ::u8 level = static_cast<AZ::u8>(state);
        StateFlags flags;
        flags.m_suspicious = level >= static_cast<AZ::u8>(AwarenessState::Suspicious);
        flags.m_searching = level >= static_cast<AZ::u8>(AwarenessState::Searching);
        flags.m_engaged = state == AwarenessState::Engaged;
        return flags;
    }

    bool ShouldPublishLastKnown(const AZ::Vector3& published, const AZ::Vector3& current, bool visiblePublished, bool visibleNow)
    {
        if (visiblePublished && !visibleNow)
        {
            return true;
        }
        return published.GetDistance(current) >= LastKnownWriteStep;
    }

    bool ShouldPublishAwareness(float published, float current, AwarenessState publishedState, AwarenessState currentState, float engagedAt)
    {
        if (publishedState != currentState)
        {
            return true;
        }
        return AZStd::abs(current - published) >= engagedAt * AwarenessWriteFraction;
    }

    void Publish(GOAT::IBlackboardSystem& blackboard, const PerceptionKeys& keys, GOAT::AgentId agent, const SenseOutputs& outputs,
        float engagedAt, PublishedState& published)
    {
        const SenseOutputs& last = published.m_outputs;
        const bool first = !published.m_valid;

        // These compare for equality inside the blackboard, so writing an unchanged value wakes nobody.
        blackboard.Set<AZ::s64>(keys.m_state, static_cast<AZ::s64>(outputs.m_state), agent);
        const StateFlags flags = FlagsFor(outputs.m_state);
        blackboard.Set<bool>(keys.m_suspicious, flags.m_suspicious, agent);
        blackboard.Set<bool>(keys.m_searching, flags.m_searching, agent);
        blackboard.Set<bool>(keys.m_engaged, flags.m_engaged, agent);
        blackboard.Set<AZ::EntityId>(keys.m_target, outputs.m_target, agent);
        blackboard.Set<bool>(keys.m_targetVisible, outputs.m_targetVisible, agent);
        blackboard.Set<bool>(keys.m_heard, outputs.m_heard, agent);
        blackboard.Set<AZ::Vector3>(keys.m_noisePosition, outputs.m_noisePosition, agent);

        SenseOutputs written = outputs;
        if (first || ShouldPublishLastKnown(last.m_lastKnown, outputs.m_lastKnown, last.m_targetVisible, outputs.m_targetVisible))
        {
            blackboard.Set<AZ::Vector3>(keys.m_lastKnown, outputs.m_lastKnown, agent);
        }
        else
        {
            written.m_lastKnown = last.m_lastKnown;
        }

        if (first || ShouldPublishAwareness(last.m_awareness, outputs.m_awareness, last.m_state, outputs.m_state, engagedAt))
        {
            blackboard.Set<float>(keys.m_awareness, outputs.m_awareness, agent);
        }
        else
        {
            written.m_awareness = last.m_awareness;
        }

        published.m_outputs = written;
        published.m_valid = true;
    }
} // namespace GOAT_Perception

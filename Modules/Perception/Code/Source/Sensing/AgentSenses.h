#pragma once

#include <GOAT_Perception/PerceptionProfileAsset.h>
#include <GOAT_Perception/PerceptionTypes.h>

#include <Sensing/AwarenessMeter.h>
#include <Sensing/PerceptionWorld.h>
#include <Sensing/SightGeometry.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/limits.h>

namespace GOAT_Perception
{
    //! Something a sensor might notice, already placed in the world and already matched against the sensor's target tags.
    struct SenseCandidate final
    {
        AZ::EntityId m_entity;
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
    };

    //! A sound that has been made, already matched against the listener's target tags.
    struct NoiseEvent final
    {
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        float m_loudness = 1.0f;
        AZ::EntityId m_source;
    };

    //! What an agent's senses hold right now, in the shape they are published in.
    struct SenseOutputs final
    {
        AwarenessState m_state = AwarenessState::Unaware;
        AZ::EntityId m_target;
        bool m_targetVisible = false;
        AZ::Vector3 m_lastKnown = AZ::Vector3::CreateZero();
        bool m_heard = false;
        AZ::Vector3 m_noisePosition = AZ::Vector3::CreateZero();
        float m_awareness = 0.0f;
    };

    //! What the last sight pass was refused, kept so a sensor that sees nothing can explain why.
    struct SenseDebug final
    {
        //! The nearest target that was inside the cone but out of sight, and what stood in the way.
        AZ::EntityId m_blockedTarget;
        AZ::EntityId m_blocker;
        float m_blockedDistance = AZStd::numeric_limits<float>::max();

        bool HasBlockedView() const { return m_blockedTarget.IsValid(); }
    };

    //! One agent's senses, memory and awareness; it reads the world only through IPerceptionWorld and writes nothing, so a test runs it as a level does.
    class AgentSenses final
    {
    public:
        void SetOwner(AZ::EntityId owner) { m_owner = owner; }

        //! One sight pass covering deltaTime seconds since the last. Candidates must already match the profile's target tags.
        void Update(const PerceptionProfileAsset& profile, const IPerceptionWorld& world, const EyePose& eye,
            AZStd::span<const SenseCandidate> candidates, float deltaTime);

        //! Hears a sound now. The sound must already match the profile's target tags.
        void Hear(const PerceptionProfileAsset& profile, const IPerceptionWorld& world, const EyePose& eye, const NoiseEvent& noise);

        //! Takes a hit from an attacker, whether or not it can be seen. The position is where the attacker was, when known.
        void ApplyDamage(const PerceptionProfileAsset& profile, AZ::EntityId attacker, const AZ::Vector3* attackerPosition);

        //! Is told by an ally where a target is, and becomes engaged with it.
        void ReceiveAlert(const PerceptionProfileAsset& profile, AZ::EntityId target, const AZ::Vector3& position);

        //! True once each time the agent becomes engaged. The flag says an ally told it, which is not worth calling for help over again.
        bool TakeEngagedEdge(bool& calledByAnother);

        SenseOutputs GetOutputs() const;
        AwarenessState GetState() const { return m_meter.GetState(); }
        const SenseDebug& GetDebug() const { return m_debug; }

    private:
        void NoteStateChange(AwarenessState before, bool calledByAnother);

        AZ::EntityId m_owner;
        SenseDebug m_debug;
        AwarenessMeter m_meter;
        AZ::EntityId m_target;
        bool m_visible = false;
        AZ::Vector3 m_lastKnown = AZ::Vector3::CreateZero();
        bool m_hasNoise = false;
        AZ::Vector3 m_noisePosition = AZ::Vector3::CreateZero();
        float m_clock = 0.0f;
        float m_heardUntil = 0.0f;
        bool m_engagedEdge = false;
        bool m_edgeCalledByAnother = false;
    };
} // namespace GOAT_Perception

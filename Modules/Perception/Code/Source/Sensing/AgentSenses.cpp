#include <Sensing/AgentSenses.h>

#include <Sensing/HearingModel.h>

#include <AzCore/std/limits.h>

namespace GOAT_Perception
{
    namespace
    {
        const SightCone& ConeFor(const PerceptionProfileAsset& profile, AwarenessState state)
        {
            switch (state)
            {
            case AwarenessState::Engaged:
                return profile.m_combatSight;
            case AwarenessState::Suspicious:
            case AwarenessState::Searching:
                return profile.m_alertSight;
            default:
                return profile.m_idleSight;
            }
        }
    } // namespace

    void AgentSenses::NoteStateChange(AwarenessState before, bool calledByAnother)
    {
        if (before != AwarenessState::Engaged && m_meter.GetState() == AwarenessState::Engaged)
        {
            m_engagedEdge = true;
            m_edgeCalledByAnother = calledByAnother;
        }
    }

    void AgentSenses::Update(const PerceptionProfileAsset& profile, const IPerceptionWorld& world, const EyePose& eye,
        AZStd::span<const SenseCandidate> candidates, float deltaTime)
    {
        const AwarenessState before = m_meter.GetState();
        const SightCone& cone = ConeFor(profile, before);
        m_debug = SenseDebug();

        // Nearest first: each round picks the nearest candidate not yet tried that could be seen, and the
        // first one with a clear line wins, so a crowd in the cone costs one ray rather than one each.
        const SenseCandidate* best = nullptr;
        float bestDistance = AZStd::numeric_limits<float>::max();
        bool bestByProximity = false;

        bool tried = false;
        float triedDistance = 0.0f;
        size_t triedIndex = 0;

        while (best == nullptr)
        {
            const SenseCandidate* next = nullptr;
            float nextDistance = AZStd::numeric_limits<float>::max();
            size_t nextIndex = 0;
            bool nextByProximity = false;

            for (size_t i = 0; i < candidates.size(); ++i)
            {
                const SenseCandidate& candidate = candidates[i];
                if (candidate.m_entity == m_owner)
                {
                    continue;
                }

                const float distance = candidate.m_position.GetDistance(eye.m_position);
                if (tried && (distance < triedDistance || (distance == triedDistance && i <= triedIndex)))
                {
                    continue;
                }
                if (next != nullptr && distance >= nextDistance)
                {
                    continue;
                }

                const bool byProximity = profile.m_proximityRange > 0.0f && distance <= profile.m_proximityRange;
                if (!byProximity && !InsideSightCone(eye, cone, candidate.m_position))
                {
                    continue;
                }

                next = &candidate;
                nextDistance = distance;
                nextIndex = i;
                nextByProximity = byProximity;
            }

            if (next == nullptr)
            {
                break;
            }

            AZ::EntityId blocker;
            if (nextByProximity ||
                world.HasLineOfSight(eye.m_position, next->m_position, profile.m_sightBlockerMask, m_owner, next->m_entity, &blocker))
            {
                best = next;
                bestDistance = nextDistance;
                bestByProximity = nextByProximity;
                break;
            }

            // Remembered so a sensor that sees nothing can say what it was looking at and what was in the way.
            if (nextDistance < m_debug.m_blockedDistance)
            {
                m_debug.m_blockedTarget = next->m_entity;
                m_debug.m_blocker = blocker;
                m_debug.m_blockedDistance = nextDistance;
            }

            tried = true;
            triedDistance = nextDistance;
            triedIndex = nextIndex;
        }

        float fillPerSecond = 0.0f;
        m_visible = best != nullptr;
        if (best != nullptr)
        {
            m_target = best->m_entity;
            m_lastKnown = best->m_position;
            fillPerSecond = profile.m_sightFillPerSecond * (bestByProximity ? 1.0f : SightCloseness(bestDistance, cone.m_range));
        }

        m_meter.Advance(profile, deltaTime, fillPerSecond);

        // Close enough to a visible target to stop weighing it up.
        if (best != nullptr && !bestByProximity && bestDistance <= profile.m_engageDistance)
        {
            m_meter.RaiseTo(profile, AwarenessState::Engaged);
        }

        // Forgotten entirely once the agent is unaware again, so a later sound does not resurrect an old target.
        if (m_meter.GetState() == AwarenessState::Unaware && !m_visible)
        {
            m_target = AZ::EntityId();
        }

        m_clock += deltaTime;
        NoteStateChange(before, false);
    }

    void AgentSenses::Hear(const PerceptionProfileAsset& profile, const IPerceptionWorld& world, const EyePose& eye, const NoiseEvent& noise)
    {
        if (noise.m_source == m_owner)
        {
            return;
        }

        const float distance = noise.m_position.GetDistance(eye.m_position);
        const bool blocked = !world.HasLineOfSight(eye.m_position, noise.m_position, profile.m_sightBlockerMask, m_owner, noise.m_source);
        const HeardSound heard = EvaluateSound(profile, distance, noise.m_loudness, blocked);
        if (!heard.m_heard)
        {
            return;
        }

        const AwarenessState before = m_meter.GetState();
        m_meter.Add(profile, heard.m_fill);
        m_hasNoise = true;
        m_noisePosition = noise.m_position;
        m_heardUntil = m_clock + profile.m_soundMemory;
        NoteStateChange(before, false);
    }

    void AgentSenses::ApplyDamage(const PerceptionProfileAsset& profile, AZ::EntityId attacker, const AZ::Vector3* attackerPosition)
    {
        const AwarenessState before = m_meter.GetState();
        m_meter.Add(profile, profile.m_damageAwareness);

        if (attacker.IsValid() && attacker != m_owner)
        {
            m_target = attacker;
            if (attackerPosition != nullptr)
            {
                m_lastKnown = *attackerPosition;
            }
        }
        NoteStateChange(before, false);
    }

    void AgentSenses::ReceiveAlert(const PerceptionProfileAsset& profile, AZ::EntityId target, const AZ::Vector3& position)
    {
        const AwarenessState before = m_meter.GetState();
        m_target = target;
        m_lastKnown = position;
        m_meter.RaiseTo(profile, AwarenessState::Engaged);
        NoteStateChange(before, true);
    }

    bool AgentSenses::TakeEngagedEdge(bool& calledByAnother)
    {
        if (!m_engagedEdge)
        {
            return false;
        }

        m_engagedEdge = false;
        calledByAnother = m_edgeCalledByAnother;
        return true;
    }

    SenseOutputs AgentSenses::GetOutputs() const
    {
        SenseOutputs outputs;
        outputs.m_state = m_meter.GetState();
        outputs.m_awareness = m_meter.GetValue();

        // What an agent that is still unaware has glimpsed is not yet something a tree should react to.
        if (outputs.m_state != AwarenessState::Unaware)
        {
            outputs.m_target = m_target;
            outputs.m_targetVisible = m_visible;
            outputs.m_lastKnown = m_lastKnown;
        }

        outputs.m_heard = m_hasNoise && m_clock <= m_heardUntil;
        outputs.m_noisePosition = m_noisePosition;
        return outputs;
    }
} // namespace GOAT_Perception

#include <Sensing/AwarenessMeter.h>

#include <AzCore/std/algorithm.h>

namespace GOAT_Perception
{
    namespace
    {
        //! Seconds a state lasts with no fresh stimulus. Unaware never ends by itself.
        float ForgetTime(const PerceptionProfileAsset& profile, AwarenessState state)
        {
            switch (state)
            {
            case AwarenessState::Engaged:
                return profile.m_sightMemory;
            case AwarenessState::Searching:
                return profile.m_searchingForget;
            case AwarenessState::Suspicious:
                return profile.m_suspiciousForget;
            default:
                return 0.0f;
            }
        }

        AwarenessState StateBelow(AwarenessState state)
        {
            return state == AwarenessState::Unaware ? state : static_cast<AwarenessState>(static_cast<AZ::u8>(state) - 1);
        }
    } // namespace

    float StateEdge(const PerceptionProfileAsset& profile, AwarenessState state)
    {
        switch (state)
        {
        case AwarenessState::Suspicious:
            return profile.m_suspiciousAt;
        case AwarenessState::Searching:
            return profile.m_searchingAt;
        case AwarenessState::Engaged:
            return profile.m_engagedAt;
        default:
            return 0.0f;
        }
    }

    AwarenessState StateForValue(const PerceptionProfileAsset& profile, float value)
    {
        if (value >= profile.m_engagedAt)
        {
            return AwarenessState::Engaged;
        }
        if (value >= profile.m_searchingAt)
        {
            return AwarenessState::Searching;
        }
        if (value >= profile.m_suspiciousAt)
        {
            return AwarenessState::Suspicious;
        }
        return AwarenessState::Unaware;
    }

    bool AwarenessMeter::Settle(const PerceptionProfileAsset& profile)
    {
        const AwarenessState reached = StateForValue(profile, m_value);
        if (static_cast<AZ::u8>(reached) > static_cast<AZ::u8>(m_state))
        {
            m_state = reached;
            return true;
        }
        return false;
    }

    bool AwarenessMeter::Advance(const PerceptionProfileAsset& profile, float deltaTime, float fillPerSecond)
    {
        const AwarenessState before = m_state;

        if (fillPerSecond > 0.0f)
        {
            m_value = AZStd::min(profile.m_engagedAt, m_value + fillPerSecond * deltaTime);
            m_since = 0.0f;
            Settle(profile);
            m_hold = ForgetTime(profile, m_state);
            return m_state != before;
        }

        if (profile.m_continuousDrain)
        {
            DrainContinuously(profile, deltaTime);
            return m_state != before;
        }

        m_since += deltaTime;

        // The value drains, but never below where the current state began until its forget time is up.
        m_value = AZStd::max(m_value - profile.m_drainPerSecond * deltaTime, StateEdge(profile, m_state));

        if (m_state != AwarenessState::Unaware && m_since >= ForgetTime(profile, m_state))
        {
            m_state = StateBelow(m_state);
            m_value = StateEdge(profile, m_state);
            m_since = 0.0f;
        }
        return m_state != before;
    }

    void AwarenessMeter::DrainContinuously(const PerceptionProfileAsset& profile, float deltaTime)
    {
        // Only the part of this step that falls after the hold drains.
        const float sinceBefore = m_since;
        m_since += deltaTime;
        const float draining = AZStd::max(m_since - AZStd::max(sinceBefore, m_hold), 0.0f);
        m_value = AZStd::max(m_value - profile.m_drainPerSecond * draining, 0.0f);

        const AwarenessState reached = StateForValue(profile, m_value);
        if (static_cast<AZ::u8>(reached) < static_cast<AZ::u8>(m_state))
        {
            m_state = reached;
        }
    }

    bool AwarenessMeter::Add(const PerceptionProfileAsset& profile, float amount)
    {
        const AwarenessState before = m_state;
        m_value = AZStd::min(profile.m_engagedAt, m_value + AZStd::max(amount, 0.0f));
        m_since = 0.0f;
        Settle(profile);
        m_hold = ForgetTime(profile, m_state);
        return m_state != before;
    }

    bool AwarenessMeter::RaiseTo(const PerceptionProfileAsset& profile, AwarenessState state)
    {
        const AwarenessState before = m_state;
        m_value = AZStd::max(m_value, StateEdge(profile, state));
        m_since = 0.0f;
        Settle(profile);
        m_hold = ForgetTime(profile, m_state);
        return m_state != before;
    }

    void AwarenessMeter::Reset()
    {
        *this = AwarenessMeter();
    }
} // namespace GOAT_Perception

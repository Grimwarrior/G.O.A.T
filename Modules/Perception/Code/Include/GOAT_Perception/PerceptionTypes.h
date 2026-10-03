#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/base.h>

namespace GOAT_Perception
{
    //! How aware an agent is of something worth reacting to. Published as the Int perc_state.
    enum class AwarenessState : AZ::u8
    {
        Unaware = 0,
        Suspicious = 1,
        Searching = 2,
        Engaged = 3
    };

    //! What an agent's senses currently hold, for code that is not a tree.
    struct PerceptionSnapshot final
    {
        AwarenessState m_state = AwarenessState::Unaware;
        AZ::EntityId m_target;
        bool m_targetVisible = false;
        AZ::Vector3 m_lastKnown = AZ::Vector3::CreateZero();
        float m_awareness = 0.0f;
    };
} // namespace GOAT_Perception

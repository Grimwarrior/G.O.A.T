#pragma once

#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AzCore/Math/Vector3.h>

namespace GOAT_Perception
{
    //! Where an agent's eyes are and which way they face. Z is up.
    struct EyePose final
    {
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Vector3 m_forward = AZ::Vector3::CreateAxisY();
    };

    //! True when a point lies inside a sight cone, ignoring anything that blocks the view; the range scale multiplies the cone's range.
    bool InsideSightCone(const EyePose& eye, const SightCone& cone, const AZ::Vector3& target, float rangeScale = 1.0f);

    //! How strongly a sighting counts: one point blank, falling to a quarter at the edge of the range.
    float SightCloseness(float distance, float range);
} // namespace GOAT_Perception

#include <Sensing/SightGeometry.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/math.h>

namespace GOAT_Perception
{
    namespace
    {
        constexpr float Tiny = 1.0e-4f;
        constexpr float FarEdgeCloseness = 0.25f;

        float PitchDegrees(const AZ::Vector3& direction)
        {
            return AZ::RadToDeg(AZStd::asin(AZ::GetClamp(direction.GetZ(), -1.0f, 1.0f)));
        }
    } // namespace

    bool InsideSightCone(const EyePose& eye, const SightCone& cone, const AZ::Vector3& target, float rangeScale)
    {
        const float range = cone.m_range * rangeScale;
        if (range <= 0.0f)
        {
            return false;
        }

        // The origin sits behind the eyes by the back offset, so a target at the agent's back is still inside.
        const AZ::Vector3 origin = eye.m_position - eye.m_forward * cone.m_backOffset;
        const AZ::Vector3 offset = target - origin;
        const float distance = offset.GetLength();
        if (distance > range)
        {
            return false;
        }
        if (distance < Tiny)
        {
            return true;
        }

        const AZ::Vector3 toTarget = offset / distance;

        if (cone.m_yawDegrees < 360.0f)
        {
            const AZ::Vector3 flatForward(eye.m_forward.GetX(), eye.m_forward.GetY(), 0.0f);
            const AZ::Vector3 flatTarget(toTarget.GetX(), toTarget.GetY(), 0.0f);

            // Looking straight up or down, or a target straight above, has no horizontal direction to compare.
            if (flatForward.GetLength() > Tiny && flatTarget.GetLength() > Tiny)
            {
                const float cosine = AZ::GetClamp(flatForward.GetNormalized().Dot(flatTarget.GetNormalized()), -1.0f, 1.0f);
                if (AZ::RadToDeg(AZStd::acos(cosine)) > cone.m_yawDegrees * 0.5f)
                {
                    return false;
                }
            }
        }

        if (cone.m_pitchDegrees < 180.0f)
        {
            const float tilt = AZStd::abs(PitchDegrees(toTarget) - PitchDegrees(eye.m_forward.GetNormalized()));
            if (tilt > cone.m_pitchDegrees * 0.5f)
            {
                return false;
            }
        }

        return true;
    }

    float SightCloseness(float distance, float range)
    {
        if (range <= 0.0f)
        {
            return 0.0f;
        }

        const float fraction = AZ::GetClamp(distance / range, 0.0f, 1.0f);
        return 1.0f - (1.0f - FarEdgeCloseness) * fraction;
    }
} // namespace GOAT_Perception

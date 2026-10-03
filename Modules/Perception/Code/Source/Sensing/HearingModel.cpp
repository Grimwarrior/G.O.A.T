#include <Sensing/HearingModel.h>

#include <AzCore/std/algorithm.h>

namespace GOAT_Perception
{
    HeardSound EvaluateSound(const PerceptionProfileAsset& profile, float distance, float loudness, bool blocked)
    {
        HeardSound result;
        if (loudness <= 0.0f || loudness < profile.m_hearingMinLoudness)
        {
            return result;
        }

        const float range = AZStd::max(profile.m_hearingRange * loudness - (blocked ? profile.m_hearingWallCut : 0.0f), 0.0f);
        if (range <= 0.0f || distance > range)
        {
            return result;
        }

        // A sound at the edge of range adds half as much as one point blank.
        result.m_heard = true;
        result.m_fill = profile.m_hearingFill * loudness * (1.0f - 0.5f * (distance / range));
        return result;
    }
} // namespace GOAT_Perception

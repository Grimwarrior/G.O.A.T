#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>

namespace GOAT_Perception
{
    namespace
    {
        constexpr float MaxYawDegrees = 360.0f;
        constexpr float MaxPitchDegrees = 180.0f;

        //! True for a finite number of at least zero, which is what every distance and duration needs.
        bool NotNegative(float value)
        {
            return AZ::IsFiniteFloat(value) && value >= 0.0f;
        }

        //! Remembers the first problem found, so Validate reads as a list of rules.
        class Findings final
        {
        public:
            void Require(bool holds, const char* problem)
            {
                if (!holds && m_first.empty())
                {
                    m_first = problem;
                }
            }

            void RequireCone(const char* label, const SightCone& cone)
            {
                if (!m_first.empty())
                {
                    return;
                }

                const bool valid = NotNegative(cone.m_range) && NotNegative(cone.m_backOffset) &&
                    NotNegative(cone.m_yawDegrees) && cone.m_yawDegrees <= MaxYawDegrees &&
                    NotNegative(cone.m_pitchDegrees) && cone.m_pitchDegrees <= MaxPitchDegrees;
                if (!valid)
                {
                    m_first = AZStd::string::format(
                        "%s sight needs a range and back offset of zero or more, a yaw of 0 to %.0f degrees and a pitch of 0 to %.0f degrees",
                        label, MaxYawDegrees, MaxPitchDegrees);
                }
            }

            AZ::Outcome<void, AZStd::string> Result() const
            {
                if (m_first.empty())
                {
                    return AZ::Success();
                }
                return AZ::Failure(m_first);
            }

        private:
            AZStd::string m_first;
        };
    } // namespace

    void SightCone::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext == nullptr)
        {
            return;
        }

        serializeContext->Class<SightCone>()
            ->Version(1)
            ->Field("Range", &SightCone::m_range)
            ->Field("Yaw", &SightCone::m_yawDegrees)
            ->Field("Pitch", &SightCone::m_pitchDegrees)
            ->Field("BackOffset", &SightCone::m_backOffset);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext == nullptr)
        {
            return;
        }

        editContext->Class<SightCone>("Sight Cone", "How far and how wide an agent sees")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SightCone::m_range, "Range", "Metres. Zero turns sight off in this state")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SightCone::m_yawDegrees, "Horizontal Angle", "Total angle left to right, centred on the facing")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, MaxYawDegrees)
                ->Attribute(AZ::Edit::Attributes::Suffix, " deg")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SightCone::m_pitchDegrees, "Vertical Angle", "Total angle up to down")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, MaxPitchDegrees)
                ->Attribute(AZ::Edit::Attributes::Suffix, " deg")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SightCone::m_backOffset, "Back Offset", "Moves the cone's origin this far behind the eyes, so a target touching the agent's back is still inside it")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m");
    }

    AZ::Outcome<void, AZStd::string> PerceptionProfileAsset::Validate() const
    {
        Findings findings;

        findings.RequireCone("Idle", m_idleSight);
        findings.RequireCone("Alert", m_alertSight);
        findings.RequireCone("Combat", m_combatSight);
        findings.Require(NotNegative(m_darknessSightScale) && m_darknessSightScale <= 1.0f,
            "Darkness Sight Scale must be between 0 and 1");

        findings.Require(NotNegative(m_hearingRange), "Hearing Range must be zero or more");
        findings.Require(NotNegative(m_hearingWallCut), "Hearing Wall Cut must be zero or more");
        findings.Require(NotNegative(m_hearingMinLoudness), "Hearing Minimum Loudness must be zero or more");
        findings.Require(NotNegative(m_proximityRange), "Proximity Range must be zero or more");

        findings.Require(NotNegative(m_suspiciousAt) && m_suspiciousAt <= m_searchingAt && m_searchingAt <= m_engagedAt,
            "Awareness thresholds must rise: Suspicious At, then Searching At, then Engaged At");
        findings.Require(AZ::IsFiniteFloat(m_engagedAt) && m_engagedAt > 0.0f, "Engaged At must be above zero");
        findings.Require(NotNegative(m_drainPerSecond), "Drain Per Second must be zero or more");
        findings.Require(NotNegative(m_engageDistance), "Engage Distance must be zero or more");
        findings.Require(NotNegative(m_sightFillPerSecond), "Sight Fill Per Second must be zero or more");
        findings.Require(NotNegative(m_hearingFill), "Hearing Fill must be zero or more");

        findings.Require(NotNegative(m_suspiciousForget), "Suspicious Forget must be zero or more");
        findings.Require(NotNegative(m_searchingForget), "Searching Forget must be zero or more");
        findings.Require(NotNegative(m_sightMemory), "Sight Memory must be zero or more");
        findings.Require(NotNegative(m_soundMemory), "Sound Memory must be zero or more");

        findings.Require(NotNegative(m_damageAwareness), "Damage Awareness must be zero or more");

        findings.Require(NotNegative(m_alertRadius), "Alert Radius must be zero or more");
        findings.Require(NotNegative(m_alertReplyDelay), "Alert Reply Delay must be zero or more");
        findings.Require(NotNegative(m_alertReplyJitter), "Alert Reply Jitter must be zero or more");

        findings.Require(AZ::IsFiniteFloat(m_senseInterval) && m_senseInterval > 0.0f,
            "Sense Interval must be above zero, or sensing would never run");

        return findings.Result();
    }

    void PerceptionProfileAsset::Reflect(AZ::ReflectContext* context)
    {
        SightCone::Reflect(context);

        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext == nullptr)
        {
            return;
        }

        // EnableForAssetEditor is what puts this type in the Asset Editor's new asset list.
        serializeContext->Class<PerceptionProfileAsset, AZ::Data::AssetData>()
            ->Version(1)
            ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
            ->Field("IdleSight", &PerceptionProfileAsset::m_idleSight)
            ->Field("AlertSight", &PerceptionProfileAsset::m_alertSight)
            ->Field("CombatSight", &PerceptionProfileAsset::m_combatSight)
            ->Field("DarknessSightScale", &PerceptionProfileAsset::m_darknessSightScale)
            ->Field("SightBlockerMask", &PerceptionProfileAsset::m_sightBlockerMask)
            ->Field("HearingRange", &PerceptionProfileAsset::m_hearingRange)
            ->Field("HearingWallCut", &PerceptionProfileAsset::m_hearingWallCut)
            ->Field("HearingMinLoudness", &PerceptionProfileAsset::m_hearingMinLoudness)
            ->Field("ProximityRange", &PerceptionProfileAsset::m_proximityRange)
            ->Field("SuspiciousAt", &PerceptionProfileAsset::m_suspiciousAt)
            ->Field("SearchingAt", &PerceptionProfileAsset::m_searchingAt)
            ->Field("EngagedAt", &PerceptionProfileAsset::m_engagedAt)
            ->Field("DrainPerSecond", &PerceptionProfileAsset::m_drainPerSecond)
            ->Field("ContinuousDrain", &PerceptionProfileAsset::m_continuousDrain)
            ->Field("EngageDistance", &PerceptionProfileAsset::m_engageDistance)
            ->Field("SightFillPerSecond", &PerceptionProfileAsset::m_sightFillPerSecond)
            ->Field("HearingFill", &PerceptionProfileAsset::m_hearingFill)
            ->Field("SuspiciousForget", &PerceptionProfileAsset::m_suspiciousForget)
            ->Field("SearchingForget", &PerceptionProfileAsset::m_searchingForget)
            ->Field("SightMemory", &PerceptionProfileAsset::m_sightMemory)
            ->Field("SoundMemory", &PerceptionProfileAsset::m_soundMemory)
            ->Field("TargetTags", &PerceptionProfileAsset::m_targetTags)
            ->Field("DamageAwareness", &PerceptionProfileAsset::m_damageAwareness)
            ->Field("AlertRadius", &PerceptionProfileAsset::m_alertRadius)
            ->Field("AlertSquad", &PerceptionProfileAsset::m_alertSquad)
            ->Field("AlertReplyDelay", &PerceptionProfileAsset::m_alertReplyDelay)
            ->Field("AlertReplyJitter", &PerceptionProfileAsset::m_alertReplyJitter)
            ->Field("SenseInterval", &PerceptionProfileAsset::m_senseInterval);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext == nullptr)
        {
            return;
        }

        using Self = PerceptionProfileAsset;
        editContext->Class<Self>("Perception", "What an agent can sense and how quickly it notices")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

            ->ClassElement(AZ::Edit::ClassElements::Group, "Sight")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_idleSight, "Idle", "Sight while the agent is unaware")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_alertSight, "Alert", "Sight while the agent is suspicious or searching")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_combatSight, "Combat", "Sight while the agent is engaged")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_darknessSightScale, "Darkness Scale", "Multiplies sight range in dark areas. One means darkness changes nothing")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_sightBlockerMask, "Blocker Mask", "Physics groups that block line of sight, whatever kind of body they are on. Zero means only static world geometry blocks it, so characters and their weapons never do")

            ->ClassElement(AZ::Edit::ClassElements::Group, "Hearing")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_hearingRange, "Range", "A sound of loudness one is heard out to this many metres")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_hearingWallCut, "Wall Cut", "Metres of range lost to each wall in the way. Zero means walls do not muffle")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_hearingMinLoudness, "Minimum Loudness", "Anything quieter than this is never heard")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_proximityRange, "Proximity Range", "Notices anything this close, whatever its facing or the light")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m")

            ->ClassElement(AZ::Edit::ClassElements::Group, "Awareness")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_suspiciousAt, "Suspicious At", "Meter value where the agent goes from unaware to suspicious")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_searchingAt, "Searching At", "Meter value where the agent goes from suspicious to searching")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_engagedAt, "Engaged At", "Meter value where the agent goes from searching to engaged. Also the meter's ceiling")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_drainPerSecond, "Drain Per Second", "Meter lost each second with nothing sensed")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_continuousDrain, "Continuous Drain", "Off, each state is held at its own start until its forget time is up and then steps down. On, the forget time is only a hold before the meter drains smoothly, and the state follows the value down")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_engageDistance, "Engage Distance", "A visible target this close skips straight to engaged")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_sightFillPerSecond, "Sight Fill Per Second", "Awareness per second a visible target adds point blank, falling to a quarter at the edge of sight range")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_hearingFill, "Hearing Fill", "Awareness a sound of loudness one adds point blank, falling to half at the edge of hearing range")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

            ->ClassElement(AZ::Edit::ClassElements::Group, "Memory")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_suspiciousForget, "Suspicious Forget", "Seconds the suspicious state lasts with no fresh stimulus")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_searchingForget, "Searching Forget", "Seconds the searching state lasts with no fresh stimulus")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_sightMemory, "Sight Memory", "Seconds the last known position is kept after sight of the target is lost")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_soundMemory, "Sound Memory", "Seconds the position of a heard sound is kept")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")

            ->ClassElement(AZ::Edit::ClassElements::Group, "Targets")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_targetTags, "Target Tags", "What counts as something to notice. Empty means everything an agent could target")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_damageAwareness, "Damage Awareness", "Awareness added when the agent is hit by something it cannot see")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

            ->ClassElement(AZ::Edit::ClassElements::Group, "Calling for Help")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_alertRadius, "Alert Radius", "Radius within which the agent calls for help on becoming engaged. Zero never calls")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_alertSquad, "Alert Squad", "Squad the call goes to. Empty means every agent in range")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_alertReplyDelay, "Reply Delay", "Seconds before a helper reacts")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_alertReplyJitter, "Reply Jitter", "A random extra delay up to this, so a pack does not react in lockstep")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")

            ->ClassElement(AZ::Edit::ClassElements::Group, "Cost")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_senseInterval, "Sense Interval", "Seconds between sight checks for the nearest agents. Farther ones are checked less often")
                ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s");
    }
} // namespace GOAT_Perception

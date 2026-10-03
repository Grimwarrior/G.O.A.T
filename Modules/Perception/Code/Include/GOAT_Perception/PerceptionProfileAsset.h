
#pragma once

#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace GOAT_Perception
{
    //! How far and how wide an agent sees. One per awareness state, so an alerted agent can look wider than an idle one.
    struct SightCone final
    {
        AZ_TYPE_INFO(SightCone, SightConeTypeId);

        static void Reflect(AZ::ReflectContext* context);

        //! Metres. Zero turns sight off in this state.
        float m_range = 20.0f;
        //! Total horizontal angle in degrees, centred on the facing.
        float m_yawDegrees = 90.0f;
        //! Total vertical angle in degrees.
        float m_pitchDegrees = 60.0f;
        //! The cone's origin is moved this far behind the eyes, so a target touching the agent's back is still inside it.
        float m_backOffset = 0.0f;
    };

    //! What one kind of agent can sense and how quickly it notices, authored in the Asset Editor and shareable between agents.
    class PerceptionProfileAsset final
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(PerceptionProfileAsset, PerceptionProfileAssetTypeId, AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(PerceptionProfileAsset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        //! Source extension the Asset Processor watches for.
        static constexpr const char* FileExtension = "prx";
        //! Group this asset is filed under in the Asset Editor.
        static constexpr const char* AssetGroup = "GOAT";
        //! Name shown in the Asset Editor's new asset list.
        static constexpr const char* DisplayName = "Perception";

        //! Succeeds when every value is one sensing can use, otherwise names the first one that is not.
        AZ::Outcome<void, AZStd::string> Validate() const;

        //! Sight, one cone per awareness state.
        SightCone m_idleSight;
        SightCone m_alertSight{ 15.0f, 50.0f, 60.0f, 0.0f };
        SightCone m_combatSight{ 50.0f, 90.0f, 60.0f, 0.0f };
        //! Multiplies sight range in dark areas. One means darkness changes nothing.
        float m_darknessSightScale = 1.0f;
        //! Physics groups that block line of sight, on any kind of body. Zero means only static world geometry blocks it.
        AZ::u32 m_sightBlockerMask = 0;

        //! Hearing. A sound of loudness one is heard out to this many metres.
        float m_hearingRange = 10.0f;
        //! Metres of hearing range lost to each wall in the way. Zero means walls do not muffle.
        float m_hearingWallCut = 0.0f;
        //! Anything quieter than this is never heard.
        float m_hearingMinLoudness = 0.0f;

        //! Notices anything this close, whatever its facing or the light.
        float m_proximityRange = 1.0f;

        //! Awareness meter edges: a stimulus fills it and silence drains it. This one is Unaware to Suspicious.
        float m_suspiciousAt = 5.0f;
        //! Suspicious to Searching.
        float m_searchingAt = 35.0f;
        //! Searching to Engaged, and the meter's ceiling.
        float m_engagedAt = 100.0f;
        //! Meter lost per second with nothing sensed.
        float m_drainPerSecond = 10.0f;
        //! Off, each state is held at its own start until its forget time is up and then steps down. On, the forget time is only a hold before the meter drains smoothly and the state follows the value.
        bool m_continuousDrain = false;
        //! A visible target this close skips straight to Engaged.
        float m_engageDistance = 8.0f;
        //! Awareness per second a visible target adds point blank, falling to a quarter at the edge of sight range.
        float m_sightFillPerSecond = 60.0f;
        //! Awareness a sound of loudness one adds point blank, falling to half at the edge of hearing range.
        float m_hearingFill = 40.0f;

        //! Seconds each state lasts with no fresh stimulus.
        float m_suspiciousForget = 10.0f;
        float m_searchingForget = 16.0f;
        //! Seconds the last known position is kept after sight of the target is lost.
        float m_sightMemory = 24.0f;
        //! Seconds the position of a heard sound is kept.
        float m_soundMemory = 10.0f;

        //! What counts as something to notice. Empty means everything an agent could target.
        AZStd::vector<AZStd::string> m_targetTags;
        //! Awareness added when this agent is hit by something it cannot see.
        float m_damageAwareness = 100.0f;

        //! Radius within which this agent calls for help on becoming Engaged. Zero never calls.
        float m_alertRadius = 0.0f;
        //! Squad the call goes to. Empty means every agent in range.
        AZStd::string m_alertSquad;
        //! Seconds before a helper reacts, plus a random extra so a pack does not react in lockstep.
        float m_alertReplyDelay = 0.0f;
        float m_alertReplyJitter = 2.0f;

        //! Seconds between sight checks for the nearest agents. Farther ones are checked less often.
        float m_senseInterval = 0.2f;
    };
} // namespace GOAT_Perception

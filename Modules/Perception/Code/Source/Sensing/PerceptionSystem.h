#pragma once

#include <GOAT_Perception/GOAT_PerceptionBus.h>

#include <Sensing/AgentSenses.h>
#include <Sensing/PerceptionKeys.h>
#include <Sensing/PerceptionPublisher.h>
#include <Sensing/PhysicsPerceptionWorld.h>

#include <GOAT/Domain/AgentId.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace GOAT_Perception
{
    //! Owns every sensor and everything sensors can notice, runs their sensing passes, and publishes the results to agent blackboards.
    class PerceptionSystem final
    {
    public:
        PerceptionSystem();

        //! Declares the perc_* variables. Called once the blackboard exists.
        bool DeclareVariables();

        void RegisterSensor(AZ::EntityId entity, const AZ::Data::Asset<PerceptionProfileAsset>& profile, float eyeHeight);
        void UnregisterSensor(AZ::EntityId entity);
        void RegisterPerceivable(AZ::EntityId entity, const PerceivableDescription& description);
        void UnregisterPerceivable(AZ::EntityId entity);
        void EmitNoise(const AZ::Vector3& position, float loudness, AZ::EntityId source, const AZ::Name& tag);
        void ReportDamage(AZ::EntityId victim, AZ::EntityId attacker);
        PerceptionSnapshot GetSnapshot(AZ::EntityId sensor) const;

        //! Runs whichever sensors are due and delivers any calls for help that have come due.
        void Tick(float deltaTime);

    private:
        struct Sensor final
        {
            AZ::EntityId m_entity;
            AZ::Data::Asset<PerceptionProfileAsset> m_asset;
            float m_eyeHeight = 1.6f;
            AgentSenses m_senses;
            GOAT::AgentId m_agent;
            float m_sinceSense = 0.0f;
            float m_sinceBlockedLog = 0.0f;
            PublishedState m_published;
            //! Which tags the profile targets, as bits; none set with m_acceptsAll false matches nothing.
            AZ::u64 m_targetMask = 0;
            //! The profile names no target tags, so every perceivable and every sound counts.
            bool m_acceptsAll = true;
        };

        //! A sensor whose turn has come, and how many of its intervals it has waited.
        struct DueSensor final
        {
            Sensor* m_sensor = nullptr;
            float m_overdue = 0.0f;
        };

        struct Perceivable final
        {
            PerceivableDescription m_description;
            //! The description's tags as bits, so matching a sensor is one AND.
            AZ::u64 m_tagMask = 0;
        };

        struct PendingAlert final
        {
            AZ::EntityId m_helper;
            AZ::EntityId m_target;
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();
            float m_dueAt = 0.0f;
        };

        //! The profile a sensor uses: its asset when loaded, otherwise the built-in defaults.
        const PerceptionProfileAsset& ProfileOf(const Sensor& sensor) const;

        //! Finds the agent driving a sensor's entity, re-finding it if the agent was replaced. False until one is registered.
        bool ResolveAgent(Sensor& sensor) const;

        //! True when a profile accepts something carrying these tags. An empty target list accepts everything.
        //! The bit a tag is known by. Registering gives a new tag the next free bit; a lookup leaves an
        //! unknown tag at zero, which no sensor that names tags can match.
        AZ::u64 TagBit(const AZStd::string& tag, bool create);
        AZ::u64 TagMask(const AZStd::vector<AZStd::string>& tags);
        static bool Accepts(const Sensor& sensor, AZ::u64 tagMask);
        EyePose EyeOf(const Sensor& sensor, const EntityPose& pose) const;

        void PublishSensor(Sensor& sensor);

        //! Says in the log, every few seconds, when a sensor has something in its view cone that it cannot see and what is in the way.
        void ReportWhyBlind(Sensor& sensor, float elapsed);

        //! Sends a call for help the first time a sensor becomes engaged on its own account.
        void CallForHelp(Sensor& sensor);

        void DeliverAlerts();

        AZStd::unordered_map<AZ::EntityId, Sensor> m_sensors;
        AZStd::unordered_map<AZ::EntityId, Perceivable> m_perceivables;
        AZStd::vector<PendingAlert> m_alerts;
        //! Scratch for one tick: every perceivable placed in the world, and the tags each carries.
        AZStd::unordered_map<AZStd::string, AZ::u32> m_tagBits;
        AZStd::vector<DueSensor> m_due;
        AZStd::vector<SenseCandidate> m_candidates;
        AZStd::vector<AZ::u64> m_candidateMasks;
        //! Scratch for one sensor: the candidates its profile accepts.
        AZStd::vector<SenseCandidate> m_matched;
        PerceptionProfileAsset m_defaultProfile;
        PerceptionKeys m_keys;
        PhysicsPerceptionWorld m_world;
        AZ::SimpleLcgRandom m_random;
        float m_clock = 0.0f;
        float m_lastEmptyLogAt = -100.0f;
    };
} // namespace GOAT_Perception

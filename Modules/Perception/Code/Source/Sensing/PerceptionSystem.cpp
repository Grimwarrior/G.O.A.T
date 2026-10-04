#include <Sensing/PerceptionSystem.h>

#include <GOAT/Interfaces/IAgentSystem.h>
#include <GOAT/Interfaces/IBlackboardSystem.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace GOAT_Perception
{
    namespace
    {
        constexpr AZ::u64 RandomSeed = 0x600A7;

        AZ_CVAR(AZ::u32, goat_perceptionRayBudget, 64, nullptr, AZ::ConsoleFunctorFlags::Null,
            "Line of sight rays GOAT perception casts in one frame before the remaining sensors wait for the next. 0 is unlimited");

        //! An entity as a person would look for it, by name and id.
        AZStd::string Describe(AZ::EntityId entity)
        {
            if (!entity.IsValid())
            {
                return "something with no entity";
            }

            AZStd::string name;
            AZ::ComponentApplicationBus::BroadcastResult(name, &AZ::ComponentApplicationRequests::GetEntityName, entity);
            return AZStd::string::format("'%s' %s", name.c_str(), entity.ToString().c_str());
        }
    } // namespace

    PerceptionSystem::PerceptionSystem()
        : m_random(RandomSeed)
    {
    }

    bool PerceptionSystem::DeclareVariables()
    {
        GOAT::IBlackboardSystem* blackboard = GOAT::BlackboardSystemInterface::Get();
        if (blackboard == nullptr)
        {
            AZ_Error("GOAT", false, "The GOAT blackboard system is not running, so perception cannot declare its variables");
            return false;
        }
        return m_keys.Declare(*blackboard);
    }

    void PerceptionSystem::RegisterSensor(AZ::EntityId entity, const AZ::Data::Asset<PerceptionProfileAsset>& profile, float eyeHeight)
    {
        Sensor fresh;
        fresh.m_entity = entity;
        fresh.m_asset = profile;
        fresh.m_eyeHeight = eyeHeight;
        fresh.m_senses.SetOwner(entity);

        const PerceptionProfileAsset& targets = ProfileOf(fresh);
        fresh.m_acceptsAll = targets.m_targetTags.empty();
        fresh.m_targetMask = TagMask(targets.m_targetTags);

        // Spread the first look across one interval, so sensors registered together do not all fire on the same frame.
        fresh.m_sinceSense = m_random.GetRandomFloat() * ProfileOf(fresh).m_senseInterval;
        m_sensors[entity] = AZStd::move(fresh);

        AZ_TracePrintf("GOAT_Perception", "Sensor registered on %s (%s); %zu sensor(s), %zu perceivable(s)\n", entity.ToString().c_str(),
            profile.IsReady() ? "profile loaded" : "built-in defaults", m_sensors.size(), m_perceivables.size());
    }

    void PerceptionSystem::UnregisterSensor(AZ::EntityId entity)
    {
        m_sensors.erase(entity);
        AZStd::erase_if(m_alerts, [entity](const PendingAlert& alert) { return alert.m_helper == entity; });
    }

    void PerceptionSystem::RegisterPerceivable(AZ::EntityId entity, const PerceivableDescription& description)
    {
        Perceivable& perceivable = m_perceivables[entity];
        perceivable.m_description = description;
        perceivable.m_tagMask = TagMask(description.m_tags);

        AZ_TracePrintf("GOAT_Perception", "Perceivable registered on %s; %zu sensor(s), %zu perceivable(s)\n", entity.ToString().c_str(),
            m_sensors.size(), m_perceivables.size());
    }

    void PerceptionSystem::UnregisterPerceivable(AZ::EntityId entity)
    {
        m_perceivables.erase(entity);
    }

    const PerceptionProfileAsset& PerceptionSystem::ProfileOf(const Sensor& sensor) const
    {
        return sensor.m_asset.IsReady() ? *sensor.m_asset.Get() : m_defaultProfile;
    }

    bool PerceptionSystem::ResolveAgent(Sensor& sensor) const
    {
        GOAT::IAgentSystem* agents = GOAT::AgentSystemInterface::Get();
        if (agents == nullptr)
        {
            return false;
        }

        if (!sensor.m_agent.IsNull() && agents->GetAgentEntity(sensor.m_agent) == sensor.m_entity)
        {
            return true;
        }

        // A replaced agent has an empty blackboard, so everything is written again.
        sensor.m_agent = agents->FindAgent(sensor.m_entity);
        sensor.m_published = PublishedState();
        return !sensor.m_agent.IsNull();
    }

    AZ::u64 PerceptionSystem::TagBit(const AZStd::string& tag, bool create)
    {
        constexpr size_t MaxTags = 64;

        const auto known = m_tagBits.find(tag);
        if (known != m_tagBits.end())
        {
            return AZ::u64{ 1 } << known->second;
        }
        if (!create)
        {
            return 0;
        }
        if (m_tagBits.size() >= MaxTags)
        {
            AZ_Error("GOAT", false, "Perception knows %zu distinct tags already, so '%s' is ignored and matches nothing", MaxTags, tag.c_str());
            return 0;
        }

        const AZ::u32 bit = static_cast<AZ::u32>(m_tagBits.size());
        m_tagBits.emplace(tag, bit);
        return AZ::u64{ 1 } << bit;
    }

    AZ::u64 PerceptionSystem::TagMask(const AZStd::vector<AZStd::string>& tags)
    {
        AZ::u64 mask = 0;
        for (const AZStd::string& tag : tags)
        {
            mask |= TagBit(tag, true);
        }
        return mask;
    }

    bool PerceptionSystem::Accepts(const Sensor& sensor, AZ::u64 tagMask)
    {
        return sensor.m_acceptsAll || (sensor.m_targetMask & tagMask) != 0;
    }

    EyePose PerceptionSystem::EyeOf(const Sensor& sensor, const EntityPose& pose) const
    {
        EyePose eye;
        eye.m_position = pose.m_position + AZ::Vector3(0.0f, 0.0f, sensor.m_eyeHeight);
        eye.m_forward = pose.m_forward;
        return eye;
    }

    void PerceptionSystem::PublishSensor(Sensor& sensor)
    {
        GOAT::IBlackboardSystem* blackboard = GOAT::BlackboardSystemInterface::Get();
        if (blackboard == nullptr || !m_keys.IsValid())
        {
            return;
        }

        Publish(*blackboard, m_keys, sensor.m_agent, sensor.m_senses.GetOutputs(), ProfileOf(sensor).m_engagedAt, sensor.m_published);
    }

    void PerceptionSystem::ReportWhyBlind(Sensor& sensor, float elapsed)
    {
        constexpr float BlockedLogInterval = 3.0f;

        sensor.m_sinceBlockedLog += elapsed;
        const SenseDebug& debug = sensor.m_senses.GetDebug();
        if (!debug.HasBlockedView() || sensor.m_sinceBlockedLog < BlockedLogInterval)
        {
            return;
        }

        sensor.m_sinceBlockedLog = 0.0f;
        AZ_TracePrintf("GOAT_Perception", "%s has %s in its view cone %.1f m away, but %s is in the way\n", Describe(sensor.m_entity).c_str(),
            Describe(debug.m_blockedTarget).c_str(), debug.m_blockedDistance, Describe(debug.m_blocker).c_str());
    }

    void PerceptionSystem::CallForHelp(Sensor& sensor)
    {
        bool calledByAnother = false;
        if (!sensor.m_senses.TakeEngagedEdge(calledByAnother) || calledByAnother)
        {
            return;
        }

        const PerceptionProfileAsset& profile = ProfileOf(sensor);
        GOAT::IAgentSystem* agents = GOAT::AgentSystemInterface::Get();
        EntityPose selfPose;
        if (profile.m_alertRadius <= 0.0f || agents == nullptr || !m_world.GetPose(sensor.m_entity, selfPose))
        {
            return;
        }

        const SenseOutputs outputs = sensor.m_senses.GetOutputs();
        const AZ::Name squad = profile.m_alertSquad.empty() ? AZ::Name() : AZ::Name(profile.m_alertSquad);

        for (auto& entry : m_sensors)
        {
            Sensor& other = entry.second;
            if (other.m_entity == sensor.m_entity || other.m_senses.GetState() == AwarenessState::Engaged)
            {
                continue;
            }

            EntityPose otherPose;
            if (!m_world.GetPose(other.m_entity, otherPose) || otherPose.m_position.GetDistance(selfPose.m_position) > profile.m_alertRadius)
            {
                continue;
            }

            if (!squad.IsEmpty() && (!ResolveAgent(other) || agents->GetAgentSquad(other.m_agent) != squad))
            {
                continue;
            }

            // The random extra keeps a pack from reacting in the same frame.
            PendingAlert alert;
            alert.m_helper = other.m_entity;
            alert.m_target = outputs.m_target;
            alert.m_position = outputs.m_lastKnown;
            alert.m_dueAt = m_clock + profile.m_alertReplyDelay + m_random.GetRandomFloat() * profile.m_alertReplyJitter;
            m_alerts.push_back(alert);
        }
    }

    void PerceptionSystem::DeliverAlerts()
    {
        for (size_t i = 0; i < m_alerts.size();)
        {
            if (m_alerts[i].m_dueAt > m_clock)
            {
                ++i;
                continue;
            }

            const PendingAlert alert = m_alerts[i];
            m_alerts[i] = m_alerts.back();
            m_alerts.pop_back();

            auto found = m_sensors.find(alert.m_helper);
            if (found == m_sensors.end())
            {
                continue;
            }

            Sensor& helper = found->second;
            if (helper.m_senses.GetState() == AwarenessState::Engaged || !ResolveAgent(helper))
            {
                continue;
            }

            helper.m_senses.ReceiveAlert(ProfileOf(helper), alert.m_target, alert.m_position);
            PublishSensor(helper);
            CallForHelp(helper);
        }
    }

    void PerceptionSystem::EmitNoise(const AZ::Vector3& position, float loudness, AZ::EntityId source, const AZ::Name& tag)
    {
        // An untagged sound is heard by everyone, so it is the one case that skips the mask.
        const bool tagged = !tag.IsEmpty();
        const AZ::u64 tagMask = tagged ? TagBit(AZStd::string(tag.GetStringView()), false) : 0;

        for (auto& entry : m_sensors)
        {
            Sensor& sensor = entry.second;
            if (sensor.m_entity == source || !ResolveAgent(sensor))
            {
                continue;
            }

            const PerceptionProfileAsset& profile = ProfileOf(sensor);
            EntityPose pose;
            if ((tagged && !Accepts(sensor, tagMask)) || !m_world.GetPose(sensor.m_entity, pose))
            {
                continue;
            }

            // Out of range even with nothing in the way, so the line of sight test is not worth paying for.
            const EyePose eye = EyeOf(sensor, pose);
            if (position.GetDistance(eye.m_position) > profile.m_hearingRange * loudness)
            {
                continue;
            }

            NoiseEvent noise;
            noise.m_position = position;
            noise.m_loudness = loudness;
            noise.m_source = source;
            sensor.m_senses.Hear(profile, m_world, eye, noise);
            PublishSensor(sensor);
            CallForHelp(sensor);
        }
    }

    void PerceptionSystem::ReportDamage(AZ::EntityId victim, AZ::EntityId attacker)
    {
        auto found = m_sensors.find(victim);
        if (found == m_sensors.end() || !ResolveAgent(found->second))
        {
            return;
        }

        Sensor& sensor = found->second;
        EntityPose attackerPose;
        const bool attackerPlaced = m_world.GetPose(attacker, attackerPose);
        sensor.m_senses.ApplyDamage(ProfileOf(sensor), attacker, attackerPlaced ? &attackerPose.m_position : nullptr);
        PublishSensor(sensor);
        CallForHelp(sensor);
    }

    PerceptionSnapshot PerceptionSystem::GetSnapshot(AZ::EntityId sensor) const
    {
        PerceptionSnapshot snapshot;
        const auto found = m_sensors.find(sensor);
        if (found == m_sensors.end())
        {
            return snapshot;
        }

        const SenseOutputs outputs = found->second.m_senses.GetOutputs();
        snapshot.m_state = outputs.m_state;
        snapshot.m_target = outputs.m_target;
        snapshot.m_targetVisible = outputs.m_targetVisible;
        snapshot.m_lastKnown = outputs.m_lastKnown;
        snapshot.m_awareness = outputs.m_awareness;
        return snapshot;
    }

    void PerceptionSystem::Tick(float deltaTime)
    {
        m_clock += deltaTime;

        GOAT::IAgentSystem* agents = GOAT::AgentSystemInterface::Get();
        if (agents == nullptr || !m_keys.IsValid() || m_sensors.empty())
        {
            return;
        }

        DeliverAlerts();

        m_world.BeginFrame();

        // Gather who is due, then serve the most overdue first, so a budget that runs out starves nobody for good.
        m_due.clear();
        for (auto& entry : m_sensors)
        {
            Sensor& sensor = entry.second;
            sensor.m_sinceSense += deltaTime;

            if (!ResolveAgent(sensor))
            {
                sensor.m_sinceSense = 0.0f;
                continue;
            }

            // Agents in a slower pacing band are sensed less often, which is the level of detail lever.
            const PerceptionProfileAsset& profile = ProfileOf(sensor);
            const float interval = profile.m_senseInterval * static_cast<float>(1 + agents->GetAgentBand(sensor.m_agent));
            if (sensor.m_sinceSense >= interval)
            {
                m_due.push_back(DueSensor{ &sensor, sensor.m_sinceSense / interval });
            }
        }

        AZStd::sort(m_due.begin(), m_due.end(),
            [](const DueSensor& left, const DueSensor& right)
            {
                return left.m_overdue > right.m_overdue;
            });

        const AZ::u32 rayBudget = goat_perceptionRayBudget;
        bool candidatesPlaced = false;
        for (const DueSensor& due : m_due)
        {
            // Left as it is, still due, and first in line next frame.
            if (rayBudget > 0 && m_world.GetRayCount() >= rayBudget)
            {
                break;
            }

            Sensor& sensor = *due.m_sensor;
            const PerceptionProfileAsset& profile = ProfileOf(sensor);

            const float elapsed = sensor.m_sinceSense;
            sensor.m_sinceSense = 0.0f;

            EntityPose pose;
            if (!m_world.GetPose(sensor.m_entity, pose))
            {
                continue;
            }

            if (!candidatesPlaced)
            {
                m_candidates.clear();
                m_candidateMasks.clear();
                for (const auto& perceivable : m_perceivables)
                {
                    EntityPose placed;
                    if (m_world.GetPose(perceivable.first, placed))
                    {
                        SenseCandidate candidate;
                        candidate.m_entity = perceivable.first;
                        candidate.m_position = placed.m_position + perceivable.second.m_description.m_aimOffset;
                        m_candidates.push_back(candidate);
                        m_candidateMasks.push_back(perceivable.second.m_tagMask);
                    }
                }
                candidatesPlaced = true;

                if (m_candidates.empty() && m_clock - m_lastEmptyLogAt >= 5.0f)
                {
                    m_lastEmptyLogAt = m_clock;
                    AZ_TracePrintf("GOAT_Perception", "%zu sensor(s) are looking but no perceivable is placed in the world (%zu registered)\n",
                        m_sensors.size(), m_perceivables.size());
                }
            }

            m_matched.clear();
            for (size_t i = 0; i < m_candidates.size(); ++i)
            {
                if (Accepts(sensor, m_candidateMasks[i]))
                {
                    m_matched.push_back(m_candidates[i]);
                }
            }

            sensor.m_senses.Update(profile, m_world, EyeOf(sensor, pose), m_matched, elapsed);
            ReportWhyBlind(sensor, elapsed);
            PublishSensor(sensor);
            CallForHelp(sensor);
        }
    }
} // namespace GOAT_Perception

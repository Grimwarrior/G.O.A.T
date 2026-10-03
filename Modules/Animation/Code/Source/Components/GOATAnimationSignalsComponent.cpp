#include <Components/GOATAnimationSignalsComponent.h>

#include <Signals/SignalVariables.h>

#include <GOAT/Interfaces/IAgentSystem.h>
#include <GOAT/Interfaces/IBlackboardSystem.h>

#include <GOAT_Animation/GOAT_AnimationTypeIds.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/lock.h>

namespace GOAT_Animation
{
    AZ_COMPONENT_IMPL(GOATAnimationSignalsComponent, "GOATAnimationSignalsComponent", GOATAnimationSignalsComponentTypeId);

    namespace
    {
        //! More than this waiting for one tick means something is raising events far faster than anything can use them.
        constexpr size_t MaxQueuedEvents = 64;
    } // namespace

    void GOATAnimationSignalsComponent::Reflect(AZ::ReflectContext* context)
    {
        SignalBinding::Reflect(context);

        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext == nullptr)
        {
            return;
        }

        serializeContext->Class<GOATAnimationSignalsComponent, AZ::Component>()
            ->Version(1)
            ->Field("EventType", &GOATAnimationSignalsComponent::m_eventType)
            ->Field("MinWeight", &GOATAnimationSignalsComponent::m_minWeight)
            ->Field("Bindings", &GOATAnimationSignalsComponent::m_bindings);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext == nullptr)
        {
            return;
        }

        editContext->Class<GOATAnimationSignalsComponent>(
                "GOAT Animation Signals", "Turns animation events into variables on the agent's blackboard")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Category, "AI")
            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/GOAT/Components/GOAT.svg")
            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/GOAT/Components/Viewport/GOAT.svg")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATAnimationSignalsComponent::m_eventType, "Event type",
                "The motion event type, set in the Animation Editor, that carries a signal. Other events are ignored")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATAnimationSignalsComponent::m_minWeight, "Minimum weight",
                "Start events from a motion contributing less than this share of the pose are ignored, so a fading clip opens nothing")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATAnimationSignalsComponent::m_bindings, "Bindings",
                "Which signals write which agent variables")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
    }

    void GOATAnimationSignalsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GOATAnimationSignalsService"));
    }

    void GOATAnimationSignalsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GOATAnimationSignalsService"));
    }

    void GOATAnimationSignalsComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // The events come from the actor, so there has to be one on this entity.
        required.push_back(AZ_CRC_CE("EMotionFXActorService"));
    }

    void GOATAnimationSignalsComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("GOATAgentService"));
    }

    void GOATAnimationSignalsComponent::Init()
    {
        // Declared before any component activates, so the agent's tree can name these variables when it compiles.
        GOAT::IBlackboardSystem* blackboard = GOAT::BlackboardSystemInterface::Get();
        if (blackboard == nullptr)
        {
            return;
        }

        for (const SignalBinding& binding : m_bindings)
        {
            if (!binding.m_key.empty())
            {
                DeclareAgentVariable(*blackboard, AZ::Name(binding.m_key), GOAT::BlackboardType::Bool, AZStd::any(false));
            }
            if (!binding.m_readyKey.empty() && binding.m_readyKey != binding.m_key)
            {
                DeclareAgentVariable(*blackboard, AZ::Name(binding.m_readyKey), GOAT::BlackboardType::Bool, AZStd::any(false));
            }
        }
    }

    void GOATAnimationSignalsComponent::Activate()
    {
        AZ_Assert(GetEntityId().IsValid(), "A component only activates on a valid entity");

        m_tracker.Configure(m_bindings, m_minWeight);
        m_written.assign(m_bindings.size(), static_cast<signed char>(-1));
        m_writtenReady.assign(m_bindings.size(), static_cast<signed char>(-1));
        m_warnedUnresolved.assign(m_bindings.size(), static_cast<signed char>(0));
        m_writtenSerial = -1;
        m_agent = GOAT::AgentId();

        m_keys.clear();
        m_readyKeys.clear();
        m_requirements.clear();
        m_requirementKeys.clear();
        GOAT::IBlackboardSystem* blackboard = GOAT::BlackboardSystemInterface::Get();
        for (const SignalBinding& binding : m_bindings)
        {
            GOAT::BlackboardKey key;
            if (blackboard != nullptr && !binding.m_key.empty())
            {
                key = blackboard->FindKey(AZ::Name(binding.m_key));
            }

            AZ_Warning("GOAT", key.IsValid(), "Animation signal '%s' on entity %s names no usable variable, so it writes nothing",
                binding.m_signal.c_str(), GetEntityId().ToString().c_str());
            m_keys.push_back(key);

            // The ready variable has to be a different variable, or the window and the combined value would overwrite each other.
            GOAT::BlackboardKey ready;
            if (!binding.m_readyKey.empty() && binding.m_readyKey != binding.m_key && blackboard != nullptr)
            {
                ready = blackboard->FindKey(AZ::Name(binding.m_readyKey));
            }
            AZ_Warning("GOAT", binding.m_readyKey != binding.m_key || binding.m_readyKey.empty(),
                "Animation signal '%s' on entity %s uses one variable as both its window and its ready variable, so the ready variable is ignored",
                binding.m_signal.c_str(), GetEntityId().ToString().c_str());
            AZ_Warning("GOAT", binding.m_requires.empty() || ready.IsValid(),
                "Animation signal '%s' on entity %s lists required variables but no ready variable to hold them, so they are ignored",
                binding.m_signal.c_str(), GetEntityId().ToString().c_str());
            m_readyKeys.push_back(ready);

            m_requirements.push_back(ParseRequirements(binding.m_requires));
            m_requirementKeys.emplace_back(m_requirements.back().size());
        }

        if (blackboard != nullptr)
        {
            m_signalKey = blackboard->FindKey(SignalNameVariable());
            m_serialKey = blackboard->FindKey(SignalSerialVariable());
        }

        EMotionFX::Integration::ActorNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
    }

    void GOATAnimationSignalsComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        EMotionFX::Integration::ActorNotificationBus::Handler::BusDisconnect();

        AZStd::lock_guard<AZStd::mutex> lock(m_queueMutex);
        m_queue.clear();
    }

    void GOATAnimationSignalsComponent::OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent)
    {
        // Anything that is not ours is dropped here, on whichever thread raised it, so it never reaches the queue.
        if (motionEvent.m_eventTypeName == nullptr || m_eventType != motionEvent.m_eventTypeName)
        {
            return;
        }

        SignalEvent event;
        event.m_parameter = motionEvent.m_parameter != nullptr ? motionEvent.m_parameter : "";
        event.m_start = motionEvent.m_isEventStart;
        event.m_weight = motionEvent.m_globalWeight;

        AZStd::lock_guard<AZStd::mutex> lock(m_queueMutex);
        if (m_queue.size() < MaxQueuedEvents)
        {
            m_queue.push_back(event);
        }
    }

    bool GOATAnimationSignalsComponent::ResolveAgent()
    {
        GOAT::IAgentSystem* agents = GOAT::AgentSystemInterface::Get();
        if (agents == nullptr)
        {
            return false;
        }

        if (!m_agent.IsNull() && agents->GetAgentEntity(m_agent) == GetEntityId())
        {
            return true;
        }

        // A replaced agent has an empty blackboard, so everything is written again.
        m_agent = agents->FindAgent(GetEntityId());
        m_written.assign(m_bindings.size(), static_cast<signed char>(-1));
        m_writtenReady.assign(m_bindings.size(), static_cast<signed char>(-1));
        m_writtenSerial = -1;
        return !m_agent.IsNull();
    }

    bool GOATAnimationSignalsComponent::RequirementsHold(GOAT::IBlackboardSystem& blackboard, size_t binding)
    {
        const AZStd::vector<Requirement>& requirements = m_requirements[binding];
        if (requirements.empty())
        {
            return true;
        }

        m_values.clear();
        const Requirement* unresolved = nullptr;
        for (size_t i = 0; i < requirements.size(); ++i)
        {
            // Another system may declare the variable later, so a missing one is looked for again on every tick.
            GOAT::BlackboardKey& key = m_requirementKeys[binding][i];
            if (!key.IsValid())
            {
                const GOAT::BlackboardKey found = blackboard.FindKey(AZ::Name(requirements[i].m_name));
                if (found.IsValid() && found.GetType() == GOAT::BlackboardType::Bool)
                {
                    key = found;
                }
            }

            const bool* value = key.IsValid() ? blackboard.Find<bool>(key, m_agent) : nullptr;
            if (value == nullptr && unresolved == nullptr)
            {
                unresolved = &requirements[i];
            }
            m_values.push_back(value != nullptr ? AZStd::optional<bool>(*value) : AZStd::nullopt);
        }

        AZ_Warning("GOAT", unresolved == nullptr || m_warnedUnresolved[binding] != 0,
            "Animation signal '%s' on entity %s requires variable '%s', which is missing or not a Bool, so it is not ready until that exists",
            m_bindings[binding].m_signal.c_str(), GetEntityId().ToString().c_str(), unresolved != nullptr ? unresolved->m_name.c_str() : "");
        if (unresolved != nullptr)
        {
            m_warnedUnresolved[binding] = 1;
        }

        return AllRequirementsHold(requirements, m_values);
    }

    void GOATAnimationSignalsComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // Ageing comes first, so an event taken below is seen for at least one tick.
        m_tracker.Advance(deltaTime);

        AZStd::vector<SignalEvent> taken;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_queueMutex);
            taken.swap(m_queue);
        }
        for (const SignalEvent& event : taken)
        {
            m_tracker.OnEvent(event);
        }

        GOAT::IBlackboardSystem* blackboard = GOAT::BlackboardSystemInterface::Get();
        GOAT::IAgentSystem* agents = GOAT::AgentSystemInterface::Get();
        if (blackboard == nullptr || agents == nullptr || !ResolveAgent())
        {
            return;
        }

        bool wake = false;
        for (size_t i = 0; i < m_bindings.size(); ++i)
        {
            const bool desired = m_tracker.Desired(i);
            const signed char desiredValue = desired ? 1 : 0;
            if (m_written[i] == desiredValue)
            {
                continue;
            }

            m_written[i] = desiredValue;
            if (m_keys[i].IsValid())
            {
                blackboard->Set<bool>(m_keys[i], desired, m_agent);
                wake = wake || m_bindings[i].m_wakeAgent;
            }
        }

        for (size_t i = 0; i < m_bindings.size(); ++i)
        {
            if (!m_readyKeys[i].IsValid())
            {
                continue;
            }

            // Only worth reading the other variables while the window is open.
            const bool ready = m_tracker.Desired(i) && RequirementsHold(*blackboard, i);
            const signed char readyValue = ready ? 1 : 0;
            if (m_writtenReady[i] == readyValue)
            {
                continue;
            }

            m_writtenReady[i] = readyValue;
            blackboard->Set<bool>(m_readyKeys[i], ready, m_agent);
            wake = wake || m_bindings[i].m_wakeAgent;
        }

        if (m_tracker.GetSerial() != m_writtenSerial)
        {
            m_writtenSerial = m_tracker.GetSerial();
            if (m_serialKey.IsValid() && m_signalKey.IsValid())
            {
                blackboard->Set<AZ::Name>(m_signalKey, AZ::Name(m_tracker.GetLastSignal()), m_agent);
                blackboard->Set<AZ::s64>(m_serialKey, m_writtenSerial, m_agent);
            }
        }

        if (wake)
        {
            agents->WakeAgents(AZStd::span<const GOAT::AgentId>(&m_agent, 1));
        }
    }
} // namespace GOAT_Animation

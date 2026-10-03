#pragma once

#include <Signals/ReadyGate.h>
#include <Signals/SignalBinding.h>
#include <Signals/SignalTracker.h>

#include <GOAT/Domain/AgentId.h>
#include <GOAT/Domain/BlackboardKey.h>
#include <GOAT/Interfaces/IBlackboardSystem.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

#include <Integration/AnimationBus.h>

namespace GOAT_Animation
{
    //! Turns animation events into blackboard variables, so an animation can open a decision window for the agent's tree.
    class GOATAnimationSignalsComponent final
        : public AZ::Component
        , protected EMotionFX::Integration::ActorNotificationBus::Handler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(GOATAnimationSignalsComponent);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AZ_DISABLE_COPY_MOVE(GOATAnimationSignalsComponent);
        GOATAnimationSignalsComponent() = default;

    protected:
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //! EMotionFX::Integration::ActorNotificationBus, which can be raised from a job thread.
        void OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent) override;

        //! AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        //! Finds the agent driving this entity, re-finding it if the agent was replaced. False until one is registered.
        bool ResolveAgent();

        //! True when every variable a binding's ready variable also requires holds right now.
        bool RequirementsHold(GOAT::IBlackboardSystem& blackboard, size_t binding);

        //! The motion event type this listens for. Events of any other type are ignored.
        AZStd::string m_eventType = "GoatSignal";

        //! Start events from a motion contributing less than this share of the pose are ignored, so a fading clip opens nothing.
        float m_minWeight = 0.5f;

        AZStd::vector<SignalBinding> m_bindings;

        SignalTracker m_tracker;
        AZStd::vector<GOAT::BlackboardKey> m_keys;
        GOAT::BlackboardKey m_signalKey;
        GOAT::BlackboardKey m_serialKey;

        //! Per binding: its optional ready variable, the variables that must also hold, and the keys those resolve to once they exist.
        AZStd::vector<GOAT::BlackboardKey> m_readyKeys;
        AZStd::vector<AZStd::vector<Requirement>> m_requirements;
        AZStd::vector<AZStd::vector<GOAT::BlackboardKey>> m_requirementKeys;
        AZStd::vector<signed char> m_warnedUnresolved;
        AZStd::vector<AZStd::optional<bool>> m_values;

        //! What was last written for each binding and each ready variable: -1 not yet, 0 false, 1 true. A new agent starts all over.
        AZStd::vector<signed char> m_written;
        AZStd::vector<signed char> m_writtenReady;
        AZ::s64 m_writtenSerial = -1;
        GOAT::AgentId m_agent;

        //! Events raised on another thread wait here until the main thread tick takes them.
        AZStd::mutex m_queueMutex;
        AZStd::vector<SignalEvent> m_queue;
    };
} // namespace GOAT_Animation

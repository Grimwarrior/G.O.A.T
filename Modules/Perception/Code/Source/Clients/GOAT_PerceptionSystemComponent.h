#pragma once

#include <GOAT_Perception/GOAT_PerceptionBus.h>

#include <Sensing/PerceptionSystem.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace GOAT_Perception
{
    //! Registers the .prx asset type and its handler, and runs sensing for every agent that has senses.
    class GOAT_PerceptionSystemComponent
        : public AZ::Component
        , protected GOAT_PerceptionRequestBus::Handler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(GOAT_PerceptionSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        GOAT_PerceptionSystemComponent();
        ~GOAT_PerceptionSystemComponent();

        AZ_DISABLE_COPY_MOVE(GOAT_PerceptionSystemComponent);

    protected:
        //! GOAT_PerceptionRequestBus
        void RegisterSensor(AZ::EntityId entity, const AZ::Data::Asset<PerceptionProfileAsset>& profile, float eyeHeight) override;
        void UnregisterSensor(AZ::EntityId entity) override;
        void RegisterPerceivable(AZ::EntityId entity, const PerceivableDescription& description) override;
        void UnregisterPerceivable(AZ::EntityId entity) override;
        void EmitNoise(const AZ::Vector3& position, float loudness, AZ::EntityId source, const AZ::Name& tag) override;
        void ReportDamage(AZ::EntityId victim, AZ::EntityId attacker) override;
        PerceptionSnapshot GetSnapshot(AZ::EntityId sensor) const override;

        //! AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        AZStd::unique_ptr<PerceptionSystem> m_perception;
    };
} // namespace GOAT_Perception

#pragma once

#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>

namespace GOAT_Perception
{
    //! Gives a GOAT agent senses: sight, hearing and an awareness meter, published as the perc_* blackboard variables.
    class GOATPerceptionComponent final
        : public AZ::Component
        , protected AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(GOATPerceptionComponent);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        void Activate() override;
        void Deactivate() override;

        //! AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        void RegisterSensor();

        //! Set once the sensor is registered, so the asset announcing itself a second time cannot restart the agent's awareness.
        bool m_registered = false;

        //! What this agent can sense. Empty uses the built-in defaults.
        AZ::Data::Asset<PerceptionProfileAsset> m_profile;

        //! Height above the entity's origin the agent sees from, in metres.
        float m_eyeHeight = 1.6f;
    };
} // namespace GOAT_Perception

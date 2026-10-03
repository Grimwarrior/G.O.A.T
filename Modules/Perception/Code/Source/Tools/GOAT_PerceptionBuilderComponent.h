#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace GOAT_Perception
{
    //! Registers the .prx handler in the Asset Processor's builder processes, which only activate components tagged AssetBuilder.
    class GOAT_PerceptionBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT_DECL(GOAT_PerceptionBuilderComponent);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        GOAT_PerceptionBuilderComponent();
        ~GOAT_PerceptionBuilderComponent();

        AZ_DISABLE_COPY_MOVE(GOAT_PerceptionBuilderComponent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
} // namespace GOAT_Perception

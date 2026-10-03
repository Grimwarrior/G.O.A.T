#include <Tools/GOAT_PerceptionBuilderComponent.h>

#include <Assets/PerceptionProfileAssetHandler.h>

#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>
#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace GOAT_Perception
{
    AZ_COMPONENT_IMPL(GOAT_PerceptionBuilderComponent, "GOAT_PerceptionBuilderComponent",
        GOAT_PerceptionBuilderComponentTypeId);

    void GOAT_PerceptionBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // The tag is what makes the Asset Processor activate this in a builder process.
            serializeContext->Class<GOAT_PerceptionBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(
                    AZ::Edit::Attributes::SystemComponentTags,
                    AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

    void GOAT_PerceptionBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        // The generic asset builder waits on this before it enumerates registered handlers.
        provided.push_back(AzFramework::s_GenericAssetRegistrar);
    }

    GOAT_PerceptionBuilderComponent::GOAT_PerceptionBuilderComponent() = default;

    GOAT_PerceptionBuilderComponent::~GOAT_PerceptionBuilderComponent() = default;

    void GOAT_PerceptionBuilderComponent::Activate()
    {
        AZ_Assert(m_assetHandlers.empty(), "A builder component activates with no handler already registered");

        if (!AZ::Data::AssetManager::IsReady() ||
            AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<PerceptionProfileAsset>()) != nullptr)
        {
            // Either too early to register, or the game module already did it in this process.
            return;
        }

        auto handler = AZStd::make_unique<PerceptionProfileAssetHandler>();
        handler->Register();
        m_assetHandlers.emplace_back(AZStd::move(handler));

        AZ_Assert(AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<PerceptionProfileAsset>()) != nullptr,
            "Registering the perception handler must make it findable, or .prx files never build");
        AZLOG_INFO("GOAT_Perception: the perception asset handler is registered in this builder process");
    }

    void GOAT_PerceptionBuilderComponent::Deactivate()
    {
        if (AZ::Data::AssetManager::IsReady())
        {
            for (auto& handler : m_assetHandlers)
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(handler.get());
            }
        }
        m_assetHandlers.clear();
    }
} // namespace GOAT_Perception

#include <AzCore/Serialization/SerializeContext.h>
#include "GOAT_PerceptionEditorSystemComponent.h"

#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>

#include <AzCore/std/string/wildcard.h>

namespace GOAT_Perception
{
    AZ_COMPONENT_IMPL(GOAT_PerceptionEditorSystemComponent, "GOAT_PerceptionEditorSystemComponent",
        GOAT_PerceptionEditorSystemComponentTypeId, BaseSystemComponent);

    void GOAT_PerceptionEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GOAT_PerceptionEditorSystemComponent, GOAT_PerceptionSystemComponent>()
                ->Version(0);
        }
    }

    GOAT_PerceptionEditorSystemComponent::GOAT_PerceptionEditorSystemComponent() = default;

    GOAT_PerceptionEditorSystemComponent::~GOAT_PerceptionEditorSystemComponent() = default;

    void GOAT_PerceptionEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("GOAT_PerceptionEditorService"));
    }

    void GOAT_PerceptionEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("GOAT_PerceptionEditorService"));
    }

    void GOAT_PerceptionEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void GOAT_PerceptionEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void GOAT_PerceptionEditorSystemComponent::Activate()
    {
        GOAT_PerceptionSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    void GOAT_PerceptionEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        GOAT_PerceptionSystemComponent::Deactivate();
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails GOAT_PerceptionEditorSystemComponent::GetSourceFileDetails(
        const char* fullSourceFileName)
    {
        AZ_Assert(fullSourceFileName != nullptr, "The asset browser always asks about a named file");
        if (fullSourceFileName == nullptr)
        {
            return {};
        }

        // Source rows are not covered by the handler's product icon, so they are answered here.
        if (AZStd::wildcard_match("*.prx", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails(
                "Editor/Icons/GOAT/AssetBrowser/Perception.svg");
        }
        return {};
    }
} // namespace GOAT_Perception

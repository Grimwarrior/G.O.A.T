#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <Clients/GOAT_PerceptionSystemComponent.h>

namespace GOAT_Perception
{
    /// System component for GOAT_Perception editor
    class GOAT_PerceptionEditorSystemComponent
        : public GOAT_PerceptionSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
        using BaseSystemComponent = GOAT_PerceptionSystemComponent;
    public:
        AZ_COMPONENT_DECL(GOAT_PerceptionEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        GOAT_PerceptionEditorSystemComponent();
        ~GOAT_PerceptionEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AssetBrowserInteractionNotificationBus: gives .prx source rows an icon, which the handler's product icon does not cover.
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
    };
} // namespace GOAT_Perception

#include <Components/GOATPerceivableComponent.h>

#include <GOAT_Perception/GOAT_PerceptionBus.h>
#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GOAT_Perception
{
    AZ_COMPONENT_IMPL(GOATPerceivableComponent, "GOATPerceivableComponent", GOATPerceivableComponentTypeId);

    void GOATPerceivableComponent::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext == nullptr)
        {
            return;
        }

        serializeContext->Class<GOATPerceivableComponent, AZ::Component>()
            ->Version(1)
            ->Field("Tags", &GOATPerceivableComponent::m_tags)
            ->Field("AimOffset", &GOATPerceivableComponent::m_aimOffset);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext == nullptr)
        {
            return;
        }

        editContext->Class<GOATPerceivableComponent>("GOAT Perceivable", "Makes this entity something GOAT agents can notice")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Category, "AI")
            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/GOAT/Components/GOAT.svg")
            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/GOAT/Components/Viewport/GOAT.svg")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATPerceivableComponent::m_tags, "Tags",
                "Labels a profile's target tags are matched against, as in \"player\". A profile with no target tags notices everything.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATPerceivableComponent::m_aimOffset, "Aim offset",
                "Where sensors aim, relative to this entity's origin.");
    }

    void GOATPerceivableComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GOATPerceivableService"));
    }

    void GOATPerceivableComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GOATPerceivableService"));
    }

    void GOATPerceivableComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void GOATPerceivableComponent::Activate()
    {
        AZ_Assert(GetEntityId().IsValid(), "A component only activates on a valid entity");

        if (GOAT_PerceptionInterface::Get() == nullptr)
        {
            AZ_Error("GOAT", false, "The GOAT perception system is not running, so entity %s cannot be noticed",
                GetEntityId().ToString().c_str());
            return;
        }

        PerceivableDescription description;
        description.m_tags = m_tags;
        description.m_aimOffset = m_aimOffset;
        GOAT_PerceptionRequestBus::Broadcast(&GOAT_PerceptionRequests::RegisterPerceivable, GetEntityId(), description);
    }

    void GOATPerceivableComponent::Deactivate()
    {
        if (GOAT_PerceptionInterface::Get() != nullptr)
        {
            GOAT_PerceptionRequestBus::Broadcast(&GOAT_PerceptionRequests::UnregisterPerceivable, GetEntityId());
        }
    }
} // namespace GOAT_Perception

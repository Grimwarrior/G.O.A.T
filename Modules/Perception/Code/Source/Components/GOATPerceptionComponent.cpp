#include <Components/GOATPerceptionComponent.h>

#include <GOAT_Perception/GOAT_PerceptionBus.h>
#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GOAT_Perception
{
    AZ_COMPONENT_IMPL(GOATPerceptionComponent, "GOATPerceptionComponent", GOATPerceptionComponentTypeId);

    void GOATPerceptionComponent::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext == nullptr)
        {
            return;
        }

        serializeContext->Class<GOATPerceptionComponent, AZ::Component>()
            ->Version(1)
            ->Field("Profile", &GOATPerceptionComponent::m_profile)
            ->Field("EyeHeight", &GOATPerceptionComponent::m_eyeHeight);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext == nullptr)
        {
            return;
        }

        editContext->Class<GOATPerceptionComponent>("GOAT Perception", "Gives a GOAT agent senses and an awareness meter")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Category, "AI")
            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/GOAT/Components/GOAT.svg")
            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/GOAT/Components/Viewport/GOAT.svg")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATPerceptionComponent::m_profile, "Profile",
                "A .prx perception profile. Empty uses the built-in defaults.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GOATPerceptionComponent::m_eyeHeight, "Eye height",
                "Height above this entity's origin the agent sees from.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " m");
    }

    void GOATPerceptionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GOATPerceptionService"));
    }

    void GOATPerceptionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GOATPerceptionService"));
    }

    void GOATPerceptionComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void GOATPerceptionComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        // The agent registers first when it is on the same entity, so its blackboard exists by the first sensing pass.
        dependent.push_back(AZ_CRC_CE("GOATAgentService"));
    }

    void GOATPerceptionComponent::Activate()
    {
        AZ_Assert(GetEntityId().IsValid(), "A component only activates on a valid entity");

        m_registered = false;
        if (m_profile.GetId().IsValid())
        {
            // Sensing starts when the profile has loaded, so the first pass never runs on defaults by mistake.
            AZ::Data::AssetBus::Handler::BusConnect(m_profile.GetId());
            m_profile.QueueLoad();

            // An asset that was already loaded announces nothing, so it is picked up here.
            if (m_profile.IsReady())
            {
                OnAssetReady(m_profile);
            }
            return;
        }

        RegisterSensor();
    }

    void GOATPerceptionComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_registered = false;
        if (GOAT_PerceptionInterface::Get() != nullptr)
        {
            GOAT_PerceptionRequestBus::Broadcast(&GOAT_PerceptionRequests::UnregisterSensor, GetEntityId());
        }
    }

    void GOATPerceptionComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // The profile can announce itself from the check in Activate and again from the bus; the second time changes nothing.
        if (m_registered)
        {
            return;
        }

        m_profile = AZ::Data::Asset<PerceptionProfileAsset>(asset);

        const auto valid = m_profile->Validate();
        AZ_Warning("GOAT", valid.IsSuccess(), "Perception profile for entity %s has a value sensing cannot use: %s",
            GetEntityId().ToString().c_str(), valid.IsSuccess() ? "" : valid.GetError().c_str());

        RegisterSensor();
    }

    void GOATPerceptionComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Registering again starts the agent's awareness over, which is the simplest honest result of a retuned profile.
        m_registered = false;
        OnAssetReady(asset);
    }

    void GOATPerceptionComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_registered)
        {
            return;
        }

        AZ_Error("GOAT", false, "Perception profile '%s' failed to load, so entity %s senses with the defaults",
            asset.GetHint().c_str(), GetEntityId().ToString().c_str());
        m_profile = AZ::Data::Asset<PerceptionProfileAsset>();
        RegisterSensor();
    }

    void GOATPerceptionComponent::RegisterSensor()
    {
        if (GOAT_PerceptionInterface::Get() == nullptr)
        {
            AZ_Error("GOAT", false, "The GOAT perception system is not running, so entity %s senses nothing",
                GetEntityId().ToString().c_str());
            return;
        }

        GOAT_PerceptionRequestBus::Broadcast(
            &GOAT_PerceptionRequests::RegisterSensor, GetEntityId(), m_profile, m_eyeHeight);
        m_registered = true;
    }
} // namespace GOAT_Perception

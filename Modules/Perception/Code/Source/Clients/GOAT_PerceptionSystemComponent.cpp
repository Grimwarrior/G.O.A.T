#include "GOAT_PerceptionSystemComponent.h"

#include <Assets/PerceptionProfileAssetHandler.h>

#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>
#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GOAT_Perception
{
    AZ_COMPONENT_IMPL(GOAT_PerceptionSystemComponent, "GOAT_PerceptionSystemComponent",
        GOAT_PerceptionSystemComponentTypeId);

    void GOAT_PerceptionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        PerceptionProfileAsset::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GOAT_PerceptionSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void GOAT_PerceptionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GOAT_PerceptionService"));
    }

    void GOAT_PerceptionSystemComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GOAT_PerceptionService"));
    }

    void GOAT_PerceptionSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // The core must be running first: sensing writes to its blackboard.
        required.push_back(AZ_CRC_CE("GOATService"));
    }

    void GOAT_PerceptionSystemComponent::GetDependentServices(
        [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    GOAT_PerceptionSystemComponent::GOAT_PerceptionSystemComponent()
    {
        if (GOAT_PerceptionInterface::Get() == nullptr)
        {
            GOAT_PerceptionInterface::Register(this);
        }
    }

    GOAT_PerceptionSystemComponent::~GOAT_PerceptionSystemComponent()
    {
        if (GOAT_PerceptionInterface::Get() == this)
        {
            GOAT_PerceptionInterface::Unregister(this);
        }
    }

    void GOAT_PerceptionSystemComponent::Init()
    {
    }

    void GOAT_PerceptionSystemComponent::Activate()
    {
        AZ_Assert(m_assetHandlers.empty(), "A system component activates with no handler already registered");

        // The launcher and the editor load different modules, so only the first registration wins.
        if (AZ::Data::AssetManager::IsReady() &&
            AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<PerceptionProfileAsset>()) == nullptr)
        {
            auto handler = AZStd::make_unique<PerceptionProfileAssetHandler>();
            handler->Register();
            m_assetHandlers.emplace_back(AZStd::move(handler));
        }

        m_perception = AZStd::make_unique<PerceptionSystem>();
        m_perception->DeclareVariables();

        GOAT_PerceptionRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void GOAT_PerceptionSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        GOAT_PerceptionRequestBus::Handler::BusDisconnect();
        m_perception.reset();

        if (AZ::Data::AssetManager::IsReady())
        {
            for (auto& handler : m_assetHandlers)
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(handler.get());
            }
        }
        m_assetHandlers.clear();
    }

    void GOAT_PerceptionSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_perception->Tick(deltaTime);
    }

    void GOAT_PerceptionSystemComponent::RegisterSensor(
        AZ::EntityId entity, const AZ::Data::Asset<PerceptionProfileAsset>& profile, float eyeHeight)
    {
        m_perception->RegisterSensor(entity, profile, eyeHeight);
    }

    void GOAT_PerceptionSystemComponent::UnregisterSensor(AZ::EntityId entity)
    {
        m_perception->UnregisterSensor(entity);
    }

    void GOAT_PerceptionSystemComponent::RegisterPerceivable(AZ::EntityId entity, const PerceivableDescription& description)
    {
        m_perception->RegisterPerceivable(entity, description);
    }

    void GOAT_PerceptionSystemComponent::UnregisterPerceivable(AZ::EntityId entity)
    {
        m_perception->UnregisterPerceivable(entity);
    }

    void GOAT_PerceptionSystemComponent::EmitNoise(
        const AZ::Vector3& position, float loudness, AZ::EntityId source, const AZ::Name& tag)
    {
        m_perception->EmitNoise(position, loudness, source, tag);
    }

    void GOAT_PerceptionSystemComponent::ReportDamage(AZ::EntityId victim, AZ::EntityId attacker)
    {
        m_perception->ReportDamage(victim, attacker);
    }

    PerceptionSnapshot GOAT_PerceptionSystemComponent::GetSnapshot(AZ::EntityId sensor) const
    {
        return m_perception->GetSnapshot(sensor);
    }
} // namespace GOAT_Perception

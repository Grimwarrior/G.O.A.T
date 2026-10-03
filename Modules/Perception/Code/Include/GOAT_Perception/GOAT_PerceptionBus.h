#pragma once

#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>
#include <GOAT_Perception/PerceptionProfileAsset.h>
#include <GOAT_Perception/PerceptionTypes.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace GOAT_Perception
{
    //! What makes an entity worth noticing, reported when it registers itself.
    struct PerceivableDescription final
    {
        //! Labels a profile's target tags are matched against. An empty list matches only profiles that accept everything.
        AZStd::vector<AZStd::string> m_tags;
        //! Where sensors aim, relative to the entity's origin, in world axes.
        AZ::Vector3 m_aimOffset = AZ::Vector3(0.0f, 0.0f, 1.0f);
    };

    class GOAT_PerceptionRequests
    {
    public:
        AZ_RTTI(GOAT_PerceptionRequests, GOAT_PerceptionRequestsTypeId);
        virtual ~GOAT_PerceptionRequests() = default;

        //! Starts sensing for an entity. A null profile uses the built-in defaults. Registering again replaces what it had.
        virtual void RegisterSensor(AZ::EntityId entity, const AZ::Data::Asset<PerceptionProfileAsset>& profile, float eyeHeight) = 0;

        //! Stops sensing for an entity.
        virtual void UnregisterSensor(AZ::EntityId entity) = 0;

        //! Makes an entity something sensors can notice.
        virtual void RegisterPerceivable(AZ::EntityId entity, const PerceivableDescription& description) = 0;

        //! Withdraws an entity from being noticed.
        virtual void UnregisterPerceivable(AZ::EntityId entity) = 0;

        //! Makes a sound at a place. Loudness one is heard out to a profile's hearing range. The tag is matched against target tags; empty matches any.
        virtual void EmitNoise(const AZ::Vector3& position, float loudness, AZ::EntityId source, const AZ::Name& tag) = 0;

        //! Tells a sensor it was hit, raising its awareness by the profile's damage awareness and turning it toward the attacker.
        virtual void ReportDamage(AZ::EntityId victim, AZ::EntityId attacker) = 0;

        //! What a sensor currently holds, or a default snapshot when the entity is not a sensor.
        virtual PerceptionSnapshot GetSnapshot(AZ::EntityId sensor) const = 0;
    };

    class GOAT_PerceptionBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using GOAT_PerceptionRequestBus = AZ::EBus<GOAT_PerceptionRequests, GOAT_PerceptionBusTraits>;
    using GOAT_PerceptionInterface = AZ::Interface<GOAT_PerceptionRequests>;
} // namespace GOAT_Perception

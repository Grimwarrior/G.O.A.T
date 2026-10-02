
#pragma once

#include <GOAT_SmartObject/GOAT_SmartObjectTypeIds.h>

#include <GOAT/Domain/AgentId.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace GOAT_SmartObject
{
    //! What one entity offers agents, reported when it registers itself.
    struct SmartObjectDescription final
    {
        //! Names an agent asks for, as in "sit" or "repair".
        AZStd::vector<AZ::Name> m_uses;
        //! Where the agent should stand, relative to the entity.
        AZ::Vector3 m_anchorOffset = AZ::Vector3::CreateZero();
        //! How many agents may use it at once.
        AZ::u32 m_capacity = 1;
        //! Free-form labels a query can require, as in "indoor".
        AZStd::vector<AZ::Name> m_tags;
        //! Who it belongs to, as in a household. Empty means anyone may use it.
        AZ::Name m_owner;
    };

    //! What an agent is looking for when it claims a use.
    struct SmartObjectQuery final
    {
        //! The use wanted, as in "sit".
        AZ::Name m_use;
        //! Where the agent looks from, in world space.
        AZ::Vector3 m_from = AZ::Vector3::CreateZero();
        //! How far it looks, in metres.
        float m_radius = 0.0f;
        //! Only objects with this owner, or with no owner at all. Empty accepts any owner.
        AZ::Name m_owner;
        //! Only objects carrying every one of these tags.
        AZStd::vector<AZ::Name> m_requiredTags;
    };

    //! What an agent got when it claimed a use. An invalid entity means nothing was free.
    struct SmartObjectClaim final
    {
        AZ::EntityId m_entity;
        //! Where to stand, in world space, which is what the agent walks to.
        AZ::Vector3 m_anchor = AZ::Vector3::CreateZero();

        bool IsValid() const { return m_entity.IsValid(); }
    };

    class GOAT_SmartObjectRequests
    {
    public:
        AZ_RTTI(GOAT_SmartObjectRequests, GOAT_SmartObjectRequestsTypeId);
        virtual ~GOAT_SmartObjectRequests() = default;

        //! Offers this entity to agents. Registering again replaces what it offered before.
        virtual void RegisterObject(AZ::EntityId entity, SmartObjectDescription description) = 0;

        //! Withdraws an entity, releasing any agent still holding a slot on it.
        virtual void UnregisterObject(AZ::EntityId entity) = 0;

        //! Takes a slot on the nearest entity matching @query.
        //! An agent holds at most one claim, so this releases whatever it held before.
        virtual SmartObjectClaim Claim(GOAT::AgentId agent, const SmartObjectQuery& query) = 0;

        //! The claim an agent holds, or an invalid one when it holds none.
        virtual SmartObjectClaim FindClaim(GOAT::AgentId agent) const = 0;

        //! Gives a registered entity a new owner, as when a household moves in. Its users keep their slots.
        virtual void SetOwner(AZ::EntityId entity, const AZ::Name& owner) = 0;

        //! Gives back whatever slot an agent holds. Safe to call when it holds none.
        virtual void Release(GOAT::AgentId agent) = 0;

        //! How many slots an entity has left, for console output and diagnostics.
        virtual AZ::u32 GetFreeSlots(AZ::EntityId entity) const = 0;
    };

    class GOAT_SmartObjectBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using GOAT_SmartObjectRequestBus = AZ::EBus<GOAT_SmartObjectRequests, GOAT_SmartObjectBusTraits>;
    using GOAT_SmartObjectInterface = AZ::Interface<GOAT_SmartObjectRequests>;

    //! One smart object's own requests, addressed by its entity.
    class GOAT_SmartObjectEntityRequests : public AZ::ComponentBus
    {
    public:
        virtual ~GOAT_SmartObjectEntityRequests() = default;

        //! Gives this object a new owner, kept with its component so a saved entity keeps it too.
        virtual void SetOwner(const AZ::Name& owner) = 0;

        //! Who it belongs to; empty when anyone may use it.
        virtual AZ::Name GetOwner() const = 0;
    };

    using GOAT_SmartObjectEntityRequestBus = AZ::EBus<GOAT_SmartObjectEntityRequests>;
} // namespace GOAT_SmartObject

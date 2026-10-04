#pragma once

#include <GOAT_SmartObject/GOAT_SmartObjectBus.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

namespace GOAT_SmartObject
{
    //! Tracks which entities offer which uses, and who is currently using them.
    //!
    //! Objects are indexed by the use they offer, so a claim only looks at objects that could
    //! satisfy it. Within a use it is still a scan, on purpose: a spatial index would have to be kept
    //! correct against entities that move, and anchors are read live from the transform for the
    //! same reason. If one use ever has enough objects for that to matter, the scan is the thing
    //! to replace and nothing outside this file would change.
    class SmartObjectRegistry final
    {
    public:
        //! Offers an entity to agents, replacing whatever it offered before.
        void Add(AZ::EntityId entity, SmartObjectDescription description);

        //! Withdraws an entity, releasing any agent still holding a slot on it.
        void Remove(AZ::EntityId entity);

        //! Takes a slot on the nearest entity matching @query.
        SmartObjectClaim Claim(GOAT::AgentId agent, const SmartObjectQuery& query);

        //! The claim an agent holds, or an invalid one when it holds none.
        SmartObjectClaim FindClaim(GOAT::AgentId agent) const;

        //! Gives a registered entity a new owner. False when it isn't registered.
        bool SetOwner(AZ::EntityId entity, const AZ::Name& owner);

        //! Gives back whatever slot an agent holds. Safe to call when it holds none.
        void Release(GOAT::AgentId agent);

        //! How many slots an entity has left.
        AZ::u32 GetFreeSlots(AZ::EntityId entity) const;

        //! How many entities are registered, for console output.
        size_t GetObjectCount() const { return m_objects.size(); }

    private:
        struct Object
        {
            SmartObjectDescription m_description;
            //! Agents currently holding a slot. Never longer than the capacity.
            AZStd::vector<GOAT::AgentId> m_users;
        };

        //! True when an object's owner and tags suit a query. The use is already settled by the index,
        //! and distance and capacity are judged by the caller.
        static bool Matches(const SmartObjectDescription& description, const SmartObjectQuery& query);

        //! The world anchor of an entity, or false when it has no transform to read.
        static bool FindAnchor(AZ::EntityId entity, const AZ::Vector3& offset, AZ::Vector3& outAnchor);

        AZStd::unordered_map<AZ::EntityId, Object> m_objects;

        //! One object offering a use. The pointer stays valid for as long as the object is registered,
        //! because unordered_map nodes do not move.
        struct Offer
        {
            AZ::EntityId m_entity;
            Object* m_object = nullptr;
        };

        //! Every registered object under each use it offers.
        AZStd::unordered_map<AZ::Name, AZStd::vector<Offer>> m_byUse;

        //! An agent holds at most one claim, which is what bounds a leaked slot to one per agent.
        AZStd::unordered_map<GOAT::AgentId, AZ::EntityId> m_claims;
    };
} // namespace GOAT_SmartObject

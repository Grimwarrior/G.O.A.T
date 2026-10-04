#include <SmartObject/SmartObjectRegistry.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/algorithm.h>

namespace GOAT_SmartObject
{
    void SmartObjectRegistry::Add(AZ::EntityId entity, SmartObjectDescription description)
    {
        AZ_Assert(entity.IsValid(), "A smart object must be offered by a valid entity");
        AZ_Assert(description.m_capacity > 0, "A smart object with no capacity can never be used");

        if (!entity.IsValid() || description.m_capacity == 0)
        {
            AZ_Error("GOAT", false, "Entity %s cannot be a smart object: it has no capacity",
                entity.ToString().c_str());
            return;
        }

        AZ_Warning("GOAT", !description.m_uses.empty(),
            "Smart object %s offers no uses, so no agent can ever claim it", entity.ToString().c_str());

        // Re-registering replaces the offer, and drops whoever was using the old one.
        Remove(entity);

        Object object;
        object.m_description = AZStd::move(description);
        object.m_users.reserve(object.m_description.m_capacity);
        Object& stored = m_objects[entity] = AZStd::move(object);

        // Once per distinct use, so an entity that lists a use twice is not offered twice.
        for (const AZ::Name& use : stored.m_description.m_uses)
        {
            AZStd::vector<Offer>& offers = m_byUse[use];
            const bool listed = AZStd::any_of(offers.begin(), offers.end(),
                [entity](const Offer& offer)
                {
                    return offer.m_entity == entity;
                });
            if (!listed)
            {
                offers.push_back(Offer{ entity, &stored });
            }
        }

        AZ_Assert(m_objects.find(entity) != m_objects.end(), "Adding a smart object must leave it findable");
    }

    void SmartObjectRegistry::Remove(AZ::EntityId entity)
    {
        const auto found = m_objects.find(entity);
        if (found == m_objects.end())
        {
            return;
        }

        // Whoever was holding a slot here now holds nothing, or their claim would outlive it.
        for (const GOAT::AgentId agent : found->second.m_users)
        {
            m_claims.erase(agent);
        }

        for (const AZ::Name& use : found->second.m_description.m_uses)
        {
            const auto offers = m_byUse.find(use);
            if (offers == m_byUse.end())
            {
                continue;
            }

            AZStd::vector<Offer>& list = offers->second;
            list.erase(AZStd::remove_if(list.begin(), list.end(),
                           [entity](const Offer& offer)
                           {
                               return offer.m_entity == entity;
                           }),
                list.end());
            if (list.empty())
            {
                m_byUse.erase(offers);
            }
        }

        m_objects.erase(found);
    }

    bool SmartObjectRegistry::FindAnchor(AZ::EntityId entity, const AZ::Vector3& offset, AZ::Vector3& outAnchor)
    {
        AZ_Assert(entity.IsValid(), "An anchor is only read from a valid entity");

        // One handler lookup, then a direct call, rather than a HasHandlers check and then a second dispatch.
        AZ::TransformInterface* transform = AZ::TransformBus::FindFirstHandler(entity);
        if (transform == nullptr)
        {
            AZ_Error("GOAT", false, "Smart object %s has no transform, so it has no anchor",
                entity.ToString().c_str());
            return false;
        }

        outAnchor = transform->GetWorldTM().TransformPoint(offset);
        return true;
    }

    bool SmartObjectRegistry::Matches(const SmartObjectDescription& description, const SmartObjectQuery& query)
    {
        if (!query.m_owner.IsEmpty() && !description.m_owner.IsEmpty() && description.m_owner != query.m_owner)
        {
            return false;
        }

        const auto& tags = description.m_tags;
        return AZStd::all_of(query.m_requiredTags.begin(), query.m_requiredTags.end(),
            [&tags](const AZ::Name& tag)
            {
                return AZStd::find(tags.begin(), tags.end(), tag) != tags.end();
            });
    }

    SmartObjectClaim SmartObjectRegistry::Claim(GOAT::AgentId agent, const SmartObjectQuery& query)
    {
        AZ_Assert(!agent.IsNull(), "A null agent cannot claim a smart object");
        AZ_Assert(!query.m_use.IsEmpty(), "A smart object is always claimed by the name of a use");
        AZ_Assert(query.m_radius > 0.0f, "A search radius must be positive");

        SmartObjectClaim claim;
        if (agent.IsNull() || query.m_use.IsEmpty())
        {
            return claim;
        }

        // One claim per agent, so taking a new one gives back the old.
        Release(agent);
        AZ_Assert(m_claims.find(agent) == m_claims.end(), "Claiming must start from an agent holding nothing");

        float bestDistanceSq = query.m_radius * query.m_radius;
        AZ::EntityId bestEntity;
        AZ::Vector3 bestAnchor = AZ::Vector3::CreateZero();

        Object* bestObject = nullptr;

        // Only objects offering the use are looked at, and capacity, owner and tags are settled before
        // the transform is read, since that is the expensive part.
        const auto offers = m_byUse.find(query.m_use);
        if (offers != m_byUse.end())
        {
            for (const Offer& offer : offers->second)
            {
                Object& object = *offer.m_object;
                if (object.m_users.size() >= object.m_description.m_capacity || !Matches(object.m_description, query))
                {
                    continue;
                }

                AZ::Vector3 anchor = AZ::Vector3::CreateZero();
                if (!FindAnchor(offer.m_entity, object.m_description.m_anchorOffset, anchor))
                {
                    continue;
                }

                const float distanceSq = query.m_from.GetDistanceSq(anchor);
                if (distanceSq >= bestDistanceSq)
                {
                    continue;
                }

                bestDistanceSq = distanceSq;
                bestEntity = offer.m_entity;
                bestObject = &object;
                bestAnchor = anchor;
            }
        }

        if (!bestEntity.IsValid())
        {
            AZLOG(GoatSmartObject, "GOAT: agent %u found nothing offering '%s' within %.1f m",
                agent.GetIndex(), query.m_use.GetCStr(), query.m_radius);
            return claim;
        }

        bestObject->m_users.push_back(agent);
        m_claims[agent] = bestEntity;

        claim.m_entity = bestEntity;
        claim.m_anchor = bestAnchor;

        AZ_Assert(m_claims.find(agent) != m_claims.end(), "A successful claim must be recorded against the agent");
        AZ_Assert(bestObject->m_users.size() <= bestObject->m_description.m_capacity,
            "A smart object must never hold more users than its capacity");

        AZLOG(GoatSmartObject, "GOAT: agent %u claimed '%s' on entity %s",
            agent.GetIndex(), query.m_use.GetCStr(), bestEntity.ToString().c_str());
        return claim;
    }

    SmartObjectClaim SmartObjectRegistry::FindClaim(GOAT::AgentId agent) const
    {
        SmartObjectClaim claim;
        const auto held = m_claims.find(agent);
        if (held == m_claims.end())
        {
            return claim;
        }

        const auto object = m_objects.find(held->second);
        AZ_Assert(object != m_objects.end(), "A recorded claim must point at an object that exists");
        if (object == m_objects.end() ||
            !FindAnchor(held->second, object->second.m_description.m_anchorOffset, claim.m_anchor))
        {
            return claim;
        }

        claim.m_entity = held->second;
        return claim;
    }

    bool SmartObjectRegistry::SetOwner(AZ::EntityId entity, const AZ::Name& owner)
    {
        const auto found = m_objects.find(entity);
        AZ_Warning("GOAT", found != m_objects.end(), "Entity %s isn't a smart object, so it can't be owned",
            entity.ToString().c_str());
        if (found == m_objects.end())
        {
            return false;
        }

        found->second.m_description.m_owner = owner;
        return true;
    }

    void SmartObjectRegistry::Release(GOAT::AgentId agent)
    {
        const auto claim = m_claims.find(agent);
        if (claim == m_claims.end())
        {
            return;
        }

        const auto object = m_objects.find(claim->second);
        AZ_Assert(object != m_objects.end(), "A recorded claim must point at an object that exists");
        if (object != m_objects.end())
        {
            auto& users = object->second.m_users;
            users.erase(AZStd::remove(users.begin(), users.end(), agent), users.end());
        }

        m_claims.erase(claim);
    }

    AZ::u32 SmartObjectRegistry::GetFreeSlots(AZ::EntityId entity) const
    {
        const auto found = m_objects.find(entity);
        if (found == m_objects.end())
        {
            return 0;
        }

        const AZ::u32 used = static_cast<AZ::u32>(found->second.m_users.size());
        AZ_Assert(used <= found->second.m_description.m_capacity,
            "A smart object must never hold more users than its capacity");

        return found->second.m_description.m_capacity - used;
    }
} // namespace GOAT_SmartObject

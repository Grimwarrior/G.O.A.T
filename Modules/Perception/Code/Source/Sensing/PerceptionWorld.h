#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>

namespace GOAT_Perception
{
    struct EntityPose final
    {
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        //! The way the entity faces, as a unit vector.
        AZ::Vector3 m_forward = AZ::Vector3::CreateAxisY();
    };

    //! Everything sensing asks of the world, so the rules can be tested without a running level.
    class IPerceptionWorld
    {
    public:
        virtual ~IPerceptionWorld() = default;

        //! Where an entity is and which way it faces. False when it has no transform.
        virtual bool GetPose(AZ::EntityId entity, EntityPose& outPose) const = 0;

        //! True when nothing but the two named entities stands between the points. Says what stood in the way when it was something.
        virtual bool HasLineOfSight(const AZ::Vector3& from, const AZ::Vector3& to, AZ::u32 blockerMask, AZ::EntityId ignoreA,
            AZ::EntityId ignoreB, AZ::EntityId* outBlocker = nullptr) const = 0;
    };
} // namespace GOAT_Perception

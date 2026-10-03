#pragma once

#include <Sensing/PerceptionWorld.h>

namespace GOAT_Perception
{
    //! The running level: transforms from the transform bus, line of sight from a physics raycast.
    class PhysicsPerceptionWorld final
        : public IPerceptionWorld
    {
    public:
        bool GetPose(AZ::EntityId entity, EntityPose& outPose) const override;
        bool HasLineOfSight(const AZ::Vector3& from, const AZ::Vector3& to, AZ::u32 blockerMask, AZ::EntityId ignoreA,
            AZ::EntityId ignoreB, AZ::EntityId* outBlocker = nullptr) const override;
    };
} // namespace GOAT_Perception

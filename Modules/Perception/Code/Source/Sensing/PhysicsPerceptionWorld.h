#pragma once

#include <Sensing/PerceptionWorld.h>

#include <AzFramework/Physics/PhysicsScene.h>

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

        //! Looks the physics scene up once for the frame and zeroes the ray count, so a ray does not repeat the lookup.
        void BeginFrame();

        //! Rays cast since BeginFrame, which is what a per frame budget spends.
        AZ::u32 GetRayCount() const { return m_rays; }

    private:
        void ResolveScene() const;

        mutable AzPhysics::SceneInterface* m_scenes = nullptr;
        mutable AzPhysics::SceneHandle m_scene = AzPhysics::InvalidSceneHandle;
        mutable AZ::u32 m_rays = 0;
    };
} // namespace GOAT_Perception

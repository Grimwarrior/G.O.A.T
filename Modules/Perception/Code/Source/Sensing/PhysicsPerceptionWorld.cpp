#include <Sensing/PhysicsPerceptionWorld.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/PhysicsScene.h>

namespace GOAT_Perception
{
    bool PhysicsPerceptionWorld::GetPose(AZ::EntityId entity, EntityPose& outPose) const
    {
        AZ::TransformInterface* transform = AZ::TransformBus::FindFirstHandler(entity);
        if (transform == nullptr)
        {
            return false;
        }

        const AZ::Transform world = transform->GetWorldTM();
        outPose.m_position = world.GetTranslation();
        outPose.m_forward = world.GetBasisY().GetNormalizedSafe();
        return true;
    }

    void PhysicsPerceptionWorld::ResolveScene() const
    {
        m_scenes = AZ::Interface<AzPhysics::SceneInterface>::Get();
        m_scene = m_scenes != nullptr ? m_scenes->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName) : AzPhysics::InvalidSceneHandle;
    }

    void PhysicsPerceptionWorld::BeginFrame()
    {
        m_rays = 0;
        ResolveScene();
    }

    bool PhysicsPerceptionWorld::HasLineOfSight(const AZ::Vector3& from, const AZ::Vector3& to, AZ::u32 blockerMask,
        AZ::EntityId ignoreA, AZ::EntityId ignoreB, AZ::EntityId* outBlocker) const
    {
        // Resolved once per frame in BeginFrame; this only repeats the lookup when there was no scene then.
        if (m_scenes == nullptr || m_scene == AzPhysics::InvalidSceneHandle)
        {
            ResolveScene();
        }

        AzPhysics::SceneInterface* scenes = m_scenes;
        const AzPhysics::SceneHandle scene = m_scene;
        if (scenes == nullptr || scene == AzPhysics::InvalidSceneHandle)
        {
            return true;
        }

        const AZ::Vector3 path = to - from;
        const float distance = path.GetLength();
        if (distance < 1.0e-3f)
        {
            return true;
        }

        ++m_rays;

        AzPhysics::RayCastRequest request;
        request.m_start = from;
        request.m_direction = path / distance;
        request.m_distance = distance;
        request.m_hitFlags = AzPhysics::SceneQuery::HitFlags::Position;
        request.m_collisionGroup = blockerMask == 0 ? AzPhysics::CollisionGroup::All : AzPhysics::CollisionGroup(static_cast<AZ::u64>(blockerMask));

        // With no groups chosen only the static world blocks a view, so characters and the weapons they carry never hide each other.
        if (blockerMask == 0)
        {
            request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        }

        // The two named entities never block, so a ray starting inside the viewer's own collider is not blocked by it.
        request.m_filterCallback = [ignoreA, ignoreB](const AzPhysics::SimulatedBody* body, const Physics::Shape* shape)
        {
            const AZ::EntityId hit = body->GetEntityId();
            if (hit == ignoreA || hit == ignoreB)
            {
                return AzPhysics::SceneQuery::QueryHitType::None;
            }

            // A shape that collides with nothing, such as a hurtbox, is there to be hit by attacks and cannot block a view.
            if (shape != nullptr && shape->GetCollisionGroup().GetMask() == 0)
            {
                return AzPhysics::SceneQuery::QueryHitType::None;
            }
            return AzPhysics::SceneQuery::QueryHitType::Block;
        };

        const AzPhysics::SceneQueryHits hits = scenes->QueryScene(scene, &request);
        if (hits && outBlocker != nullptr)
        {
            *outBlocker = hits.m_hits.front().m_entityId;
        }
        return !hits;
    }
} // namespace GOAT_Perception

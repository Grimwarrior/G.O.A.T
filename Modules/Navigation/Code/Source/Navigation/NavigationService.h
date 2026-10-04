#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <RecastNavigation/RecastNavigationMeshBus.h>
#include <RecastNavigation/RecastSmartPointer.h>


namespace GOAT_Navigation
{
    //! Identifies one path request for the lifetime of that request.
    using PathRequestId = AZ::u32;

    //! Value meaning "no request".
    inline constexpr PathRequestId InvalidPathRequestId = 0;

    //! How a finished request turned out.
    enum class PathStatus : AZ::u8
    {
        Pending,  //!< Queued, not yet handed to a worker.
        Running,  //!< Handed to a worker; must not be erased until it reports back.
        Ready,    //!< A path was found.
        NotFound, //!< The query ran but no path exists.
        Cancelled
    };

    //! Answers path queries off the main thread.
    //!
    //! RecastNavigation hands out a single dtNavMeshQuery behind one exclusive mutex, and a
    //! dtNavMeshQuery carries mutable node pools, so sharing it would serialise every query.
    //! This service borrows only the dtNavMesh and gives each worker its own query object.
    //!
    //! The mesh itself is still mutated by Recast's own tile tasks, so reads are guarded by a
    //! shared_mutex owned here whose write window is driven by the nav mesh notifications.
    class NavigationService final
        : private RecastNavigation::RecastNavigationMeshNotificationBus::Handler
    {
    public:
        NavigationService();
        ~NavigationService();

        //! Binds to a navigation mesh entity. Safe to call again to rebind.
        void SetNavigationMesh(AZ::EntityId navMeshEntity);

        //! Releases the mesh binding and cancels everything in flight.
        void ClearNavigationMesh();

        //! True once a mesh is bound and its worker queries are usable.
        bool IsReady() const;

        //! True when a navigation mesh entity is bound, built or not.
        //! Told apart from IsReady on purpose: not built yet is normal at level start and
        //! resolves itself, while nothing bound at all never will.
        bool HasNavigationMesh() const;

        //! Queues a path query. Returns InvalidPathRequestId when no mesh is bound.
        PathRequestId RequestPath(const AZ::Vector3& from, const AZ::Vector3& to);

        //! Reports how a request is doing without consuming it.
        PathStatus GetStatus(PathRequestId request) const;

        //! Moves a finished path out of the service. Returns false while it is still pending.
        bool TakePath(PathRequestId request, AZStd::vector<AZ::Vector3>& outPath);

        //! Abandons a request. Safe for an id that already completed.
        void CancelRequest(PathRequestId request);

        //! Recycles requests cancelled while a worker held them and re-binds a lost mesh. Call once per frame.
        void Update();

        //! How many requests are queued or running, for console output.
        size_t GetPendingCount() const;

    private:
        //! One in flight query and its result.
        struct Request final
        {
            PathRequestId m_id = InvalidPathRequestId;
            AZ::Vector3 m_from = AZ::Vector3::CreateZero();
            AZ::Vector3 m_to = AZ::Vector3::CreateZero();
            AZStd::vector<AZ::Vector3> m_path;
            PathStatus m_status = PathStatus::Pending;
        };

        //! A worker's private query object, so node pools are never shared.
        struct Worker final
        {
            RecastNavigation::RecastPointer<dtNavMeshQuery> m_query;
            bool m_initialised = false;
        };

        ////////////////////////////////////////////////////////////////////////
        // RecastNavigation::RecastNavigationMeshNotificationBus
        void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) override;
        void OnNavigationMeshBeganRecalculating(AZ::EntityId navigationMeshEntity) override;
        ////////////////////////////////////////////////////////////////////////

        //! Points every worker query at the current mesh. Takes the write lock.
        void RebindWorkers();

        //! Drops requests cancelled while their worker was running. No tasks may be in flight.
        void ReapCancelled();

        //! Blocks until no worker is running a query. Must be called before anything a worker reads is torn down.
        void WaitForIdle();

        //! Re-binds when a mesh should be usable but is not, so a missed notification does not
        //! leave path queries failing for the rest of the level.
        void RecoverBinding();

        //! A worker thread's body: takes the oldest queued request the moment it is free, so one slow
        //! query never holds back the others.
        void WorkerLoop(size_t workerIndex);

        //! One query on one worker, stored by id once it finishes.
        void RunQueryAndStore(PathRequestId id, const AZ::Vector3& from, const AZ::Vector3& to, Worker& worker);

        //! The query itself, on a worker's own objects, under a shared read lock.
        void RunQueryImpl(
            const AZ::Vector3& from,
            const AZ::Vector3& to,
            Worker& worker,
            AZStd::vector<AZ::Vector3>& outPath,
            PathStatus& outStatus) const;

        AZ::EntityId m_navMeshEntity;
        //! Kept alive so the mesh outlives a rebuild that swaps Recast's own object.
        AZStd::shared_ptr<RecastNavigation::NavMeshQuery> m_navObject;
        //! Snapshot taken under Recast's lock; only read afterwards.
        dtNavMesh* m_navMesh = nullptr;

        //! Guards m_navMesh reads against the rebuild window. Not Recast's mutex, which is exclusive.
        mutable AZStd::shared_mutex m_meshLock;
        //! Guards the request table.
        mutable AZStd::mutex m_requestLock;

        AZStd::vector<Worker> m_workers;
        //! Requests by id, so every lookup is constant time however many agents are waiting.
        AZStd::unordered_map<PathRequestId, Request> m_requests;
        //! Ids waiting to be submitted, oldest first. An id whose request was cancelled or taken is
        //! skipped when it reaches the front.
        AZStd::deque<PathRequestId> m_pendingOrder;
        //! Ids cancelled while a worker held them, erased by ReapCancelled once nothing is in flight.
        AZStd::vector<PathRequestId> m_cancelledIds;
        PathRequestId m_nextRequestId = 1;

        //! One persistent thread per worker, started at construction and joined at destruction.
        AZStd::vector<AZStd::thread> m_threads;
        //! Both wait on m_requestLock: workers for queued work, ClearNavigationMesh for the last query to end.
        AZStd::condition_variable m_workAvailable;
        AZStd::condition_variable m_idle;
        //! How many workers are inside a query right now. Guarded by m_requestLock.
        size_t m_running = 0;
        bool m_stopping = false;

        //! True between a mesh rebuild starting and finishing, when reads must not run.
        //! Atomic because Recast raises those notifications from whichever thread drives the
        //! rebuild, while Update reads this on the main thread.
        AZStd::atomic<bool> m_recalculating{ false };
        //! When that window opened, so a rebuild that never reports finishing cannot stall paths.
        AZStd::atomic<AZ::TimeMs> m_recalculatingSince{ AZ::TimeMs{ 0 } };
    };
} // namespace GOAT_Navigation

#include <Navigation/NavigationService.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/parallel/lock.h>

#include <RecastNavigation/RecastHelpers.h>

#include <DetourNavMeshQuery.h>

namespace GOAT_Navigation
{
    namespace
    {
        //! Worker threads used for path queries. One private dtNavMeshQuery is kept per worker.
        AZ_CVAR(AZ::u32, goat_pathQueryThreads, 2, nullptr, AZ::ConsoleFunctorFlags::Null,
            "Number of threads GOAT uses to answer navigation path queries");

        //! Search nodes each worker query may use. Recast's own mesh controller uses the same value.
        constexpr int MaxSearchNodes = 2048;
        //! Longest polygon path a single query may return.
        constexpr int MaxPathPolygons = 256;
        //! Longest waypoint path a single query may return.
        constexpr int MaxPathPoints = 256;
        //! How far from a given position a walkable polygon is looked for.
        constexpr float PolygonSearchExtents = 3.0f;

        //! How long a mesh rebuild may run before queries stop waiting for it to report finishing.
        constexpr AZ::TimeMs RecalculationGrace{ 5000 };
    } // namespace

    NavigationService::NavigationService()
    {
        const AZ::u32 workerCount = AZStd::max<AZ::u32>(goat_pathQueryThreads, 1);
        m_workers.resize(workerCount);

        AZ_Assert(!m_workers.empty(), "A navigation service must have at least one worker");

        m_threads.reserve(workerCount);
        for (size_t worker = 0; worker < workerCount; ++worker)
        {
            AZStd::thread_desc desc;
            desc.m_name = "GOAT Navigation";
            m_threads.emplace_back(desc, [this, worker]() { WorkerLoop(worker); });
        }
    }

    NavigationService::~NavigationService()
    {
        ClearNavigationMesh();

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);
            m_stopping = true;
        }
        m_workAvailable.notify_all();
        for (AZStd::thread& thread : m_threads)
        {
            thread.join();
        }
    }

    void NavigationService::SetNavigationMesh(AZ::EntityId navMeshEntity)
    {
        AZ_Assert(navMeshEntity.IsValid(), "A navigation mesh must be bound to a valid entity");
        if (!navMeshEntity.IsValid())
        {
            return;
        }

        ClearNavigationMesh();

        m_navMeshEntity = navMeshEntity;
        RecastNavigation::RecastNavigationMeshNotificationBus::Handler::BusConnect(navMeshEntity);
        RebindWorkers();

        AZ_Warning("GOAT", m_navMesh != nullptr,
            "Navigation mesh entity %s has no built mesh yet; paths will fail until it is built",
            navMeshEntity.ToString().c_str());
    }

    void NavigationService::WaitForIdle()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_requestLock);
        m_idle.wait(lock, [this] { return m_running == 0; });
        AZ_Assert(m_running == 0, "Waiting must leave no worker running");
    }

    void NavigationService::ClearNavigationMesh()
    {
        RecastNavigation::RecastNavigationMeshNotificationBus::Handler::BusDisconnect();

        // Cancelled first, so nothing still queued can start; then the queries already running finish.
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);
            m_pendingOrder.clear();
            for (auto& [id, request] : m_requests)
            {
                request.m_status = PathStatus::Cancelled;
                m_cancelledIds.push_back(id);
            }
        }
        WaitForIdle();

        // Nothing may read the mesh pointer past this point.
        AZStd::unique_lock<AZStd::shared_mutex> writeLock(m_meshLock);
        m_navMesh = nullptr;
        m_navObject.reset();
        m_navMeshEntity = AZ::EntityId{};
        m_recalculating = false;
        for (Worker& worker : m_workers)
        {
            worker.m_initialised = false;
        }
    }

    bool NavigationService::IsReady() const
    {
        AZStd::shared_lock<AZStd::shared_mutex> readLock(m_meshLock);
        return m_navMesh != nullptr;
    }

    bool NavigationService::HasNavigationMesh() const
    {
        AZStd::shared_lock<AZStd::shared_mutex> readLock(m_meshLock);
        return m_navMeshEntity.IsValid();
    }

    void NavigationService::RebindWorkers()
    {
        AZStd::shared_ptr<RecastNavigation::NavMeshQuery> navObject;
        RecastNavigation::RecastNavigationMeshRequestBus::EventResult(
            navObject, m_navMeshEntity, &RecastNavigation::RecastNavigationMeshRequests::GetNavigationObject);

        AZStd::unique_lock<AZStd::shared_mutex> writeLock(m_meshLock);

        m_navObject = navObject;
        m_navMesh = nullptr;
        for (Worker& worker : m_workers)
        {
            worker.m_initialised = false;
        }

        if (navObject == nullptr)
        {
            return;
        }

        // Recast's own mutex is only taken to read the mesh pointer out; queries below never take it.
        {
            RecastNavigation::NavMeshQuery::LockGuard recastLock(*navObject);
            m_navMesh = recastLock.GetNavMesh();
        }

        if (m_navMesh == nullptr)
        {
            return;
        }

        for (Worker& worker : m_workers)
        {
            if (worker.m_query == nullptr)
            {
                worker.m_query.reset(dtAllocNavMeshQuery());
            }

            AZ_Assert(worker.m_query != nullptr, "Detour failed to allocate a navigation query");
            if (worker.m_query == nullptr)
            {
                continue;
            }

            // init takes a const dtNavMesh*, so a worker query only ever reads the mesh.
            worker.m_initialised = dtStatusSucceed(worker.m_query->init(m_navMesh, MaxSearchNodes));
            AZ_Warning("GOAT", worker.m_initialised, "A navigation worker query failed to bind to the mesh");
        }
    }

    void NavigationService::OnNavigationMeshBeganRecalculating([[maybe_unused]] AZ::EntityId navigationMeshEntity)
    {
        AZ_Assert(navigationMeshEntity == m_navMeshEntity,
            "A navigation notification arrived for an entity this service is not bound to");

        m_recalculating = true;
        m_recalculatingSince = AZ::GetElapsedTimeMs();

        // Recast is about to add and remove tiles, so stop every reader until it is done.
        AZStd::unique_lock<AZStd::shared_mutex> writeLock(m_meshLock);
        m_navMesh = nullptr;
        for (Worker& worker : m_workers)
        {
            worker.m_initialised = false;
        }
    }

    void NavigationService::OnNavigationMeshUpdated([[maybe_unused]] AZ::EntityId navigationMeshEntity)
    {
        AZ_Assert(navigationMeshEntity == m_navMeshEntity,
            "A navigation notification arrived for an entity this service is not bound to");

        m_recalculating = false;

        // The mesh object itself may have been replaced, so re-fetch and re-init rather than reuse.
        RebindWorkers();
    }

    PathRequestId NavigationService::RequestPath(const AZ::Vector3& from, const AZ::Vector3& to)
    {
        if (!IsReady())
        {
            return InvalidPathRequestId;
        }

        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);

        Request request;
        request.m_id = m_nextRequestId++;
        request.m_from = from;
        request.m_to = to;
        request.m_status = PathStatus::Pending;

        AZ_Assert(request.m_id != InvalidPathRequestId, "A path request id must never collide with the null id");

        const PathRequestId id = request.m_id;
        m_requests.emplace(id, AZStd::move(request));
        m_pendingOrder.push_back(id);
        m_workAvailable.notify_one();
        return id;
    }

    PathStatus NavigationService::GetStatus(PathRequestId request) const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);
        const auto found = m_requests.find(request);
        return found != m_requests.end() ? found->second.m_status : PathStatus::Cancelled;
    }

    bool NavigationService::TakePath(PathRequestId request, AZStd::vector<AZ::Vector3>& outPath)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);

        const auto found = m_requests.find(request);
        if (found == m_requests.end())
        {
            return false;
        }

        if (found->second.m_status == PathStatus::Pending || found->second.m_status == PathStatus::Running)
        {
            return false;
        }

        outPath = AZStd::move(found->second.m_path);
        m_requests.erase(found);
        return true;
    }

    void NavigationService::CancelRequest(PathRequestId request)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);
        const auto found = m_requests.find(request);
        if (found == m_requests.end())
        {
            return;
        }

        // A worker may be writing to this entry, so mark it and let Update reap it instead.
        if (found->second.m_status == PathStatus::Running)
        {
            found->second.m_status = PathStatus::Cancelled;
            m_cancelledIds.push_back(request);
            return;
        }

        m_requests.erase(found);
    }

    size_t NavigationService::GetPendingCount() const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);
        return m_requests.size();
    }

    void NavigationService::WorkerLoop(size_t workerIndex)
    {
        Worker& worker = m_workers[workerIndex];
        AZStd::unique_lock<AZStd::mutex> lock(m_requestLock);
        while (true)
        {
            m_workAvailable.wait(lock, [this] { return m_stopping || !m_pendingOrder.empty(); });
            if (m_stopping)
            {
                return;
            }

            const PathRequestId id = m_pendingOrder.front();
            m_pendingOrder.pop_front();

            // Cancelled or taken since it was queued: nothing to run.
            const auto found = m_requests.find(id);
            if (found == m_requests.end() || found->second.m_status != PathStatus::Pending)
            {
                continue;
            }

            // Endpoints are read in the same locked section that marks it running, so the entry
            // cannot be reaped between being picked and being read.
            found->second.m_status = PathStatus::Running;
            const AZ::Vector3 from = found->second.m_from;
            const AZ::Vector3 to = found->second.m_to;
            ++m_running;

            lock.unlock();
            RunQueryAndStore(id, from, to, worker);
            lock.lock();

            if (--m_running == 0)
            {
                m_idle.notify_all();
            }
        }
    }

    void NavigationService::RunQueryAndStore(
        PathRequestId id, const AZ::Vector3& from, const AZ::Vector3& to, Worker& worker)
    {
        AZStd::vector<AZ::Vector3> path;
        PathStatus status = PathStatus::NotFound;
        RunQueryImpl(from, to, worker, path, status);

        // Store by id under the lock. If the request was cancelled meanwhile the lookup fails and
        // the result is simply dropped, which is why the task never holds a pointer to it.
        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);
        const auto found = m_requests.find(id);
        if (found == m_requests.end())
        {
            return;
        }

        Request& request = found->second;
        AZ_Assert(request.m_status == PathStatus::Running || request.m_status == PathStatus::Cancelled,
            "A worker finished a request that was not handed to it");

        // Cancelled while this ran: keep it cancelled and let Update reap it.
        if (request.m_status == PathStatus::Cancelled)
        {
            return;
        }

        request.m_path = AZStd::move(path);
        request.m_status = status;
    }

    void NavigationService::RunQueryImpl(
        const AZ::Vector3& from, const AZ::Vector3& to, Worker& worker,
        AZStd::vector<AZ::Vector3>& outPath, PathStatus& outStatus) const
    {
        // A shared lock, so queries run concurrently but never during a mesh rebuild.
        AZStd::shared_lock<AZStd::shared_mutex> readLock(m_meshLock);

        outStatus = PathStatus::NotFound;
        if (m_navMesh == nullptr || !worker.m_initialised || worker.m_query == nullptr)
        {
            return;
        }

        // Detour works in +Y up; O3DE is +Z up.
        const RecastNavigation::RecastVector3 start = RecastNavigation::RecastVector3::CreateFromVector3SwapYZ(from);
        const RecastNavigation::RecastVector3 end = RecastNavigation::RecastVector3::CreateFromVector3SwapYZ(to);
        const float extents[3] = { PolygonSearchExtents, PolygonSearchExtents, PolygonSearchExtents };

        // Each worker owns its filter; dtQueryFilter is not shareable across concurrent queries.
        const dtQueryFilter filter;

        dtPolyRef startPoly = 0;
        dtPolyRef endPoly = 0;
        RecastNavigation::RecastVector3 nearestStart;
        RecastNavigation::RecastVector3 nearestEnd;

        worker.m_query->findNearestPoly(start.m_xyz, extents, &filter, &startPoly, nearestStart.m_xyz);
        worker.m_query->findNearestPoly(end.m_xyz, extents, &filter, &endPoly, nearestEnd.m_xyz);

        if (startPoly == 0 || endPoly == 0)
        {
            return;
        }

        dtPolyRef polygons[MaxPathPolygons] = {};
        int polygonCount = 0;
        worker.m_query->findPath(
            startPoly, endPoly, nearestStart.m_xyz, nearestEnd.m_xyz, &filter, polygons, &polygonCount, MaxPathPolygons);

        if (polygonCount <= 0)
        {
            return;
        }

        float points[MaxPathPoints * 3] = {};
        int pointCount = 0;
        worker.m_query->findStraightPath(
            nearestStart.m_xyz, nearestEnd.m_xyz, polygons, polygonCount, points, nullptr, nullptr, &pointCount,
            MaxPathPoints);

        if (pointCount <= 0)
        {
            return;
        }

        outPath.clear();
        outPath.reserve(static_cast<size_t>(pointCount));
        for (int i = 0; i < pointCount; ++i)
        {
            const auto point = RecastNavigation::RecastVector3::CreateFromFloatValuesWithoutAxisSwapping(&points[i * 3]);
            outPath.push_back(point.AsVector3WithZup());
        }

        AZ_Assert(!outPath.empty(), "A ready path must contain at least one waypoint");
        outStatus = PathStatus::Ready;
    }

    void NavigationService::ReapCancelled()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_requestLock);

        for (const PathRequestId id : m_cancelledIds)
        {
            const auto found = m_requests.find(id);
            if (found == m_requests.end())
            {
                continue;
            }

            AZ_Assert(found->second.m_status != PathStatus::Running,
                "A request is still marked running after its task graph completed");
            if (found->second.m_status == PathStatus::Cancelled)
            {
                m_requests.erase(found);
            }
        }
        m_cancelledIds.clear();
    }

    void NavigationService::RecoverBinding()
    {
        if (!m_navMeshEntity.IsValid())
        {
            return;
        }

        {
            AZStd::shared_lock<AZStd::shared_mutex> readLock(m_meshLock);
            if (m_navMesh != nullptr)
            {
                return;
            }
        }

        // Inside a rebuild, having no mesh is correct and reads must keep waiting. Past the
        // grace period it means the finishing notification never arrived, and continuing to
        // wait would fail every path query for the rest of the level.
        const bool recalculating = m_recalculating;
        if (recalculating && AZ::GetElapsedTimeMs() - m_recalculatingSince.load() < RecalculationGrace)
        {
            return;
        }

        AZ_Warning("GOAT", !recalculating,
            "Navigation mesh entity %s never reported finishing its rebuild; re-binding anyway",
            m_navMeshEntity.ToString().c_str());

        m_recalculating = false;
        RebindWorkers();
    }

    void NavigationService::Update()
    {
        RecoverBinding();
        ReapCancelled();
    }
} // namespace GOAT_Navigation

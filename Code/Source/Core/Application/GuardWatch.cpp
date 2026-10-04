#include <Core/Application/GuardWatch.h>

#include <AzCore/Debug/Trace.h>

namespace GOAT
{
    void GuardWatch::Connect(const AgentProgram& program, IBlackboardSystem& blackboard, AgentId agent)
    {
        Disconnect();

        for (size_t scopeIndex = 0; scopeIndex < ScopeCount; ++scopeIndex)
        {
            if (!program.m_watchedScopes[scopeIndex])
            {
                continue;
            }

            AZ_Warning("GOAT", blackboard.FindStorage(static_cast<BlackboardScope>(scopeIndex), agent) != nullptr,
                "Agent %u guards a variable in a scope it has no storage for, so those guards never fire",
                agent.GetIndex());

            m_watched[scopeIndex] = true;
        }

        m_blackboard = &blackboard;
        m_program = &program;
        m_agent = agent;

        // A freshly connected agent has never evaluated its guards.
        m_forced = true;
    }

    void GuardWatch::Disconnect()
    {
        m_blackboard = nullptr;
        m_program = nullptr;
        m_agent = AgentId{};
        m_watched.fill(false);
        m_seen.fill(0);
        m_forced = true;
    }

    AZ::u32 GuardWatch::EpochOf(size_t scopeIndex) const
    {
        const BlackboardStorage* storage =
            m_blackboard->FindStorage(static_cast<BlackboardScope>(scopeIndex), m_agent);
        return storage != nullptr ? storage->GetEpoch() : 0;
    }

    bool GuardWatch::IsDirty() const
    {
        if (m_forced)
        {
            return true;
        }

        if (m_blackboard == nullptr)
        {
            return false;
        }

        for (size_t scopeIndex = 0; scopeIndex < ScopeCount; ++scopeIndex)
        {
            if (!m_watched[scopeIndex])
            {
                continue;
            }

            const BlackboardStorage* storage =
                m_blackboard->FindStorage(static_cast<BlackboardScope>(scopeIndex), m_agent);
            const AZ::u32 epoch = storage != nullptr ? storage->GetEpoch() : 0;
            if (epoch == m_seen[scopeIndex])
            {
                continue;
            }

            // Something in the scope changed. It only matters when it was a slot this program reads.
            if (storage == nullptr || m_program == nullptr || m_program->WatchesWholeScope(scopeIndex))
            {
                return true;
            }

            for (const BlackboardKey key : m_program->m_watchedKeys[scopeIndex])
            {
                if (storage->GetStamp(key) > m_seen[scopeIndex])
                {
                    return true;
                }
            }

            // Nothing watched moved, so what changed is already dealt with.
            m_seen[scopeIndex] = epoch;
        }

        return false;
    }

    void GuardWatch::Clear()
    {
        m_forced = false;
        if (m_blackboard == nullptr)
        {
            return;
        }

        for (size_t scopeIndex = 0; scopeIndex < ScopeCount; ++scopeIndex)
        {
            if (m_watched[scopeIndex])
            {
                m_seen[scopeIndex] = EpochOf(scopeIndex);
            }
        }
    }
} // namespace GOAT

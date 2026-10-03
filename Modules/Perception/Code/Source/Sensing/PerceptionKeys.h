#pragma once

#include <GOAT/Domain/BlackboardKey.h>
#include <GOAT/Interfaces/IBlackboardSystem.h>

namespace GOAT_Perception
{
    //! The agent-scoped blackboard variables this module publishes, so no .bbx has to mention them.
    struct PerceptionKeys final
    {
        //! Declares the variables and caches their keys. Safe to call more than once.
        bool Declare(GOAT::IBlackboardSystem& blackboard);

        //! True once every key resolved.
        bool IsValid() const;

        //! AwarenessState as an Int: 0 Unaware, 1 Suspicious, 2 Searching, 3 Engaged.
        GOAT::BlackboardKey m_state;
        //! At least Suspicious, at least Searching, and Engaged, as Bools because a tree condition can only read a Bool.
        GOAT::BlackboardKey m_suspicious;
        GOAT::BlackboardKey m_searching;
        GOAT::BlackboardKey m_engaged;
        GOAT::BlackboardKey m_target;
        GOAT::BlackboardKey m_targetVisible;
        GOAT::BlackboardKey m_lastKnown;
        GOAT::BlackboardKey m_heard;
        GOAT::BlackboardKey m_noisePosition;
        GOAT::BlackboardKey m_awareness;
    };
} // namespace GOAT_Perception

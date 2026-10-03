#include <Animation/WaitSignalAction.h>

#include <GOAT/Interfaces/IBlackboardSystem.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Name/NameDictionary.h>

#include <cstring>

namespace GOAT_Animation
{
    namespace
    {
        static_assert(sizeof(float) <= AZStd::tuple_size<GOAT::ActionScratch>::value, "Wait state does not fit in the action scratch");

        // The scratch is plain bytes with no alignment promise, so the elapsed time is copied in and out rather than cast.
        float LoadElapsed(const GOAT::ActionContext& context)
        {
            float elapsed = 0.0f;
            std::memcpy(&elapsed, context.m_scratch->data(), sizeof(float));
            return elapsed;
        }

        void StoreElapsed(const GOAT::ActionContext& context, float elapsed)
        {
            std::memcpy(context.m_scratch->data(), &elapsed, sizeof(float));
        }
    } // namespace

    AZ::Name WaitSignalAction::GetName() const
    {
        return AZ_NAME_LITERAL("wait_signal");
    }

    void WaitSignalAction::Begin(const GOAT::ActionContext& context)
    {
        AZ_Assert(context.m_scratch != nullptr, "A wait always runs with agent scratch to count in");
        StoreElapsed(context, 0.0f);
    }

    GOAT::ActionResult WaitSignalAction::Step(const GOAT::ActionContext& context, float deltaTime)
    {
        AZ_Assert(context.m_request != nullptr, "A wait always runs with a request");
        AZ_Assert(context.m_blackboard != nullptr, "A wait always runs with a blackboard");
        AZ_Assert(deltaTime >= 0.0f, "A wait cannot be stepped backwards in time");

        const GOAT::BlackboardKey key = context.m_request->m_targetKey;
        if (!key.IsValid())
        {
            AZ_Error("GOAT", false, "wait_signal names no usable variable, so it can never be satisfied");
            return GOAT::ActionResult::Failure;
        }

        // A signal that is already up counts, so a branch that arrives mid-window does not wait for the next one.
        const bool* signal = context.m_blackboard->Find<bool>(key, context.m_agent);
        if (signal != nullptr && *signal)
        {
            return GOAT::ActionResult::Success;
        }

        const float elapsed = LoadElapsed(context) + deltaTime;
        StoreElapsed(context, elapsed);

        // The node's number is the timeout in seconds. Zero waits as long as it takes.
        const float timeout = context.m_request->m_amount;
        if (timeout > 0.0f && elapsed >= timeout)
        {
            return GOAT::ActionResult::Failure;
        }

        // Nothing to do until the signal is written, which wakes the agent, or until the timeout comes due.
        if (context.m_wake != nullptr)
        {
            if (timeout > 0.0f)
            {
                context.m_wake->m_when = GOAT::WakeWhen::AtTime;
                context.m_wake->m_in = timeout - elapsed;
            }
            else
            {
                context.m_wake->m_when = GOAT::WakeWhen::OnSignal;
            }
        }
        return GOAT::ActionResult::Running;
    }
} // namespace GOAT_Animation

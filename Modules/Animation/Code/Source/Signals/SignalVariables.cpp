#include <Signals/SignalVariables.h>

#include <AzCore/Debug/Trace.h>

namespace GOAT_Animation
{
    GOAT::BlackboardKey DeclareAgentVariable(
        GOAT::IBlackboardSystem& blackboard, const AZ::Name& name, GOAT::BlackboardType type, AZStd::any defaultValue)
    {
        auto declared = blackboard.Declare(name, GOAT::BlackboardScope::Agent, type, AZStd::move(defaultValue));
        if (declared.IsSuccess())
        {
            return declared.GetValue();
        }

        const GOAT::BlackboardKey existing = blackboard.FindKey(name);
        AZ_Error("GOAT", existing.IsValid(), "Animation variable '%s' could not be declared: %s",
            name.GetCStr(), declared.GetError().c_str());
        return existing;
    }
} // namespace GOAT_Animation

#pragma once

#include <GOAT/Domain/BlackboardKey.h>
#include <GOAT/Interfaces/IBlackboardSystem.h>

#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/std/any.h>

namespace GOAT_Animation
{
    //! Declares one agent-scoped variable, treating a name that is already declared as success since a level reload declares it again.
    GOAT::BlackboardKey DeclareAgentVariable(
        GOAT::IBlackboardSystem& blackboard, const AZ::Name& name, GOAT::BlackboardType type, AZStd::any defaultValue);

    inline AZ::Name SignalNameVariable()
    {
        return AZ_NAME_LITERAL("anim_signal");
    }

    inline AZ::Name SignalSerialVariable()
    {
        return AZ_NAME_LITERAL("anim_signal_serial");
    }
} // namespace GOAT_Animation

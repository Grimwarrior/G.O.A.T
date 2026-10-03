#pragma once

#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace GOAT_Animation
{
    //! One agent Bool a window must be combined with before it counts as ready.
    struct Requirement final
    {
        AZStd::string m_name;
        //! True when the Bool must be false, written with a leading "!".
        bool m_negated = false;
    };

    //! Reads requirements written as "name" or "!name", skipping blank ones and trimming spaces.
    AZStd::vector<Requirement> ParseRequirements(AZStd::span<const AZStd::string> texts);

    //! True when every requirement holds. A value of nothing, meaning the variable is missing or not a Bool, never holds.
    //! The values line up with the requirements one for one. No requirements at all holds.
    bool AllRequirementsHold(AZStd::span<const Requirement> requirements, AZStd::span<const AZStd::optional<bool>> values);
} // namespace GOAT_Animation

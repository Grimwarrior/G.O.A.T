#pragma once

#include <GOAT/Interfaces/IActionState.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace GOAT_Animation
{
    //! Pauses the branch until an animation signal variable is true, then succeeds. A timeout turns a signal that never comes into a failure.
    class WaitSignalAction final
        : public GOAT::IActionState
    {
    public:
        AZ_CLASS_ALLOCATOR(WaitSignalAction, AZ::SystemAllocator);

        AZ::Name GetName() const override;
        void Begin(const GOAT::ActionContext& context) override;
        GOAT::ActionResult Step(const GOAT::ActionContext& context, float deltaTime) override;
    };
} // namespace GOAT_Animation

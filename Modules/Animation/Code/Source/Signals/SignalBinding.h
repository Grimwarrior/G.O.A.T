#pragma once

#include <GOAT_Animation/GOAT_AnimationTypeIds.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace GOAT_Animation
{
    //! Ties one animation signal to one agent blackboard variable.
    struct SignalBinding final
    {
        AZ_TYPE_INFO(SignalBinding, SignalBindingTypeId);

        static void Reflect(AZ::ReflectContext* context);

        //! The motion event parameter this binding listens for, as in "combo".
        AZStd::string m_signal;
        //! The agent-scoped Bool it writes, as in "window_combo". The module declares it.
        AZStd::string m_key;
        //! True for a ranged event, which holds the key true from its start to its end; false for a one-shot event, which pulses it.
        bool m_window = true;
        //! A window that never gets its end event closes itself after this many seconds. Zero never closes it.
        float m_maxSeconds = 2.0f;
        //! Also wake the agent when the key changes, for an action waiting on a signal.
        bool m_wakeAgent = true;
        //! An optional second Bool, true only while the window is open and every required variable holds, so one guard can watch the whole condition. The module declares it.
        AZStd::string m_readyKey;
        //! Agent Bools that must also be true for the ready variable, as in "target_in_reach". A leading "!" means false, as in "!stunned".
        AZStd::vector<AZStd::string> m_requires;
    };
} // namespace GOAT_Animation

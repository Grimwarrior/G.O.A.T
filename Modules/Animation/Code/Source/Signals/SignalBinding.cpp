#include <Signals/SignalBinding.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GOAT_Animation
{
    void SignalBinding::Reflect(AZ::ReflectContext* context)
    {
        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext == nullptr)
        {
            return;
        }

        serializeContext->Class<SignalBinding>()
            ->Version(1)
            ->Field("Signal", &SignalBinding::m_signal)
            ->Field("Key", &SignalBinding::m_key)
            ->Field("Window", &SignalBinding::m_window)
            ->Field("MaxSeconds", &SignalBinding::m_maxSeconds)
            ->Field("WakeAgent", &SignalBinding::m_wakeAgent)
            ->Field("ReadyKey", &SignalBinding::m_readyKey)
            ->Field("Requires", &SignalBinding::m_requires);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext == nullptr)
        {
            return;
        }

        editContext->Class<SignalBinding>("Signal Binding", "Ties one animation signal to one agent blackboard variable")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_signal, "Signal",
                "The motion event parameter to listen for, as in \"combo\"")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_key, "Variable",
                "The agent Bool this writes, as in \"window_combo\". Declared automatically")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_window, "Window",
                "On for a ranged event, which holds the variable true from its start to its end. Off for a one-shot event, which pulses it")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_maxSeconds, "Max seconds",
                "A window that never gets its end event closes itself after this long. Zero never closes it")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " s")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_wakeAgent, "Wake agent",
                "Also wake the agent when the variable changes, for an action waiting on a signal")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_readyKey, "Ready variable",
                "Optional. A second Bool, true only while the window is open and every required variable holds, so one guard can watch the whole condition. Declared automatically")
            ->DataElement(AZ::Edit::UIHandlers::Default, &SignalBinding::m_requires, "Also require",
                "Agent Bools that must also be true for the ready variable, as in \"target_in_reach\". A leading ! means false, as in \"!stunned\"");
    }
} // namespace GOAT_Animation

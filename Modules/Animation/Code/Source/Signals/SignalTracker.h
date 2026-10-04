#pragma once

#include <Signals/SignalBinding.h>

#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>

namespace GOAT_Animation
{
    //! A pulse stays true this long, so an agent that ticks a few times a second still sees it.
    inline constexpr float PulseHoldSeconds = 0.25f;

    //! An animation event already copied out of EMotionFX, so it can cross from whichever thread raised it.
    struct SignalEvent final
    {
        AZStd::fixed_string<63> m_parameter;
        //! True for a one-shot event or the start of a ranged one; false for the end of a ranged one.
        bool m_start = true;
        //! How much of the final pose the playing motion contributes.
        float m_weight = 1.0f;
    };

    //! What each binding's variable should hold, worked out from the events seen. It touches no engine system and no blackboard.
    class SignalTracker final
    {
    public:
        //! Starts over with these bindings. A start event from a motion below the weight is ignored; an end event is always honoured.
        void Configure(AZStd::span<const SignalBinding> bindings, float minWeight);

        //! Ages what is open or pulsing by deltaTime. Call before the events of the same tick, so one raised now is seen at least once.
        void Advance(float deltaTime);

        void OnEvent(const SignalEvent& event);

        //! Closes every window and ends every pulse.
        void CloseAll();

        size_t GetBindingCount() const { return m_states.size(); }

        //! True while any window is open or any pulse is still running, which is when ticking can change something.
        bool IsActive() const;

        //! What the binding's variable should hold now.
        bool Desired(size_t index) const;

        //! The parameter of the latest accepted start event, and how many there have been.
        const AZStd::string& GetLastSignal() const { return m_lastSignal; }
        AZ::s64 GetSerial() const { return m_serial; }

    private:
        struct State final
        {
            AZStd::string m_signal;
            bool m_window = true;
            float m_maxSeconds = 0.0f;
            bool m_open = false;
            float m_openFor = 0.0f;
            float m_pulseLeft = 0.0f;
        };

        AZStd::vector<State> m_states;
        float m_minWeight = 0.0f;
        AZStd::string m_lastSignal;
        AZ::s64 m_serial = 0;
    };
} // namespace GOAT_Animation

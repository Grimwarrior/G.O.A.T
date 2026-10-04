#include <Signals/SignalTracker.h>

#include <AzCore/std/algorithm.h>

namespace GOAT_Animation
{
    void SignalTracker::Configure(AZStd::span<const SignalBinding> bindings, float minWeight)
    {
        m_states.clear();
        m_states.reserve(bindings.size());
        for (const SignalBinding& binding : bindings)
        {
            State state;
            state.m_signal = binding.m_signal;
            state.m_window = binding.m_window;
            state.m_maxSeconds = binding.m_maxSeconds;
            m_states.push_back(AZStd::move(state));
        }

        m_minWeight = minWeight;
        m_lastSignal.clear();
        m_serial = 0;
    }

    void SignalTracker::Advance(float deltaTime)
    {
        for (State& state : m_states)
        {
            if (state.m_window)
            {
                if (!state.m_open)
                {
                    continue;
                }

                state.m_openFor += deltaTime;
                if (state.m_maxSeconds > 0.0f && state.m_openFor >= state.m_maxSeconds)
                {
                    state.m_open = false;
                }
            }
            else
            {
                state.m_pulseLeft = AZStd::max(state.m_pulseLeft - deltaTime, 0.0f);
            }
        }
    }

    void SignalTracker::OnEvent(const SignalEvent& event)
    {
        // A fading motion's start events do not count, but its end events must still close a window it opened.
        if (event.m_start && event.m_weight < m_minWeight)
        {
            return;
        }

        if (event.m_start)
        {
            m_lastSignal = event.m_parameter.c_str();
            ++m_serial;
        }

        for (State& state : m_states)
        {
            if (state.m_signal != event.m_parameter.c_str())
            {
                continue;
            }

            if (state.m_window)
            {
                state.m_open = event.m_start;
                state.m_openFor = 0.0f;
            }
            else if (event.m_start)
            {
                state.m_pulseLeft = PulseHoldSeconds;
            }
        }
    }

    void SignalTracker::CloseAll()
    {
        for (State& state : m_states)
        {
            state.m_open = false;
            state.m_openFor = 0.0f;
            state.m_pulseLeft = 0.0f;
        }
    }

    bool SignalTracker::IsActive() const
    {
        for (const State& state : m_states)
        {
            if (state.m_window ? state.m_open : state.m_pulseLeft > 0.0f)
            {
                return true;
            }
        }
        return false;
    }

    bool SignalTracker::Desired(size_t index) const
    {
        AZ_Assert(index < m_states.size(), "A binding index must address a configured binding");
        if (index >= m_states.size())
        {
            return false;
        }

        const State& state = m_states[index];
        return state.m_window ? state.m_open : state.m_pulseLeft > 0.0f;
    }
} // namespace GOAT_Animation

#include <Signals/SignalTracker.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Animation
{
    //! One window binding on "combo" and one pulse binding on "hit", with a minimum weight of one half.
    class SignalTrackerFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            SignalBinding window;
            window.m_signal = "combo";
            window.m_key = "window_combo";
            window.m_window = true;
            window.m_maxSeconds = 2.0f;

            SignalBinding pulse;
            pulse.m_signal = "hit";
            pulse.m_key = "pulse_hit";
            pulse.m_window = false;

            m_bindings = { window, pulse };
            m_tracker.Configure(m_bindings, 0.5f);
        }

        static SignalEvent Event(const char* parameter, bool start = true, float weight = 1.0f)
        {
            SignalEvent event;
            event.m_parameter = parameter;
            event.m_start = start;
            event.m_weight = weight;
            return event;
        }

        AZStd::vector<SignalBinding> m_bindings;
        SignalTracker m_tracker;
    };

    TEST_F(SignalTrackerFixture, NothingIsUpToBeginWith)
    {
        EXPECT_EQ(m_tracker.GetBindingCount(), 2u);
        EXPECT_FALSE(m_tracker.Desired(0));
        EXPECT_FALSE(m_tracker.Desired(1));
        EXPECT_EQ(m_tracker.GetSerial(), 0);
    }

    TEST_F(SignalTrackerFixture, AWindow_IsUpFromItsStartToItsEnd)
    {
        m_tracker.OnEvent(Event("combo", true));
        EXPECT_TRUE(m_tracker.Desired(0));

        m_tracker.Advance(0.5f);
        EXPECT_TRUE(m_tracker.Desired(0));

        m_tracker.OnEvent(Event("combo", false));
        EXPECT_FALSE(m_tracker.Desired(0));
    }

    //! An end event that never comes must not leave the window open for good.
    TEST_F(SignalTrackerFixture, AWindowWithNoEnd_ClosesItselfAfterItsMaxSeconds)
    {
        m_tracker.OnEvent(Event("combo", true));

        m_tracker.Advance(1.9f);
        EXPECT_TRUE(m_tracker.Desired(0));

        m_tracker.Advance(0.2f);
        EXPECT_FALSE(m_tracker.Desired(0));
    }

    TEST_F(SignalTrackerFixture, AMaxSecondsOfZero_NeverClosesAWindow)
    {
        m_bindings[0].m_maxSeconds = 0.0f;
        m_tracker.Configure(m_bindings, 0.5f);

        m_tracker.OnEvent(Event("combo", true));
        m_tracker.Advance(1000.0f);
        EXPECT_TRUE(m_tracker.Desired(0));
    }

    TEST_F(SignalTrackerFixture, ARestartedWindow_StartsItsTimeoutOver)
    {
        m_tracker.OnEvent(Event("combo", true));
        m_tracker.Advance(1.5f);
        m_tracker.OnEvent(Event("combo", true));
        m_tracker.Advance(1.5f);
        EXPECT_TRUE(m_tracker.Desired(0));
    }

    TEST_F(SignalTrackerFixture, APulse_HoldsForAMomentThenDrops)
    {
        m_tracker.OnEvent(Event("hit", true));
        EXPECT_TRUE(m_tracker.Desired(1));

        m_tracker.Advance(PulseHoldSeconds * 0.5f);
        EXPECT_TRUE(m_tracker.Desired(1));

        m_tracker.Advance(PulseHoldSeconds);
        EXPECT_FALSE(m_tracker.Desired(1));
    }

    //! Ageing runs before the events of a tick, so a pulse raised now survives a long frame.
    TEST_F(SignalTrackerFixture, APulseRaisedAfterAgeing_IsSeenEvenOnALongFrame)
    {
        m_tracker.Advance(5.0f);
        m_tracker.OnEvent(Event("hit", true));
        EXPECT_TRUE(m_tracker.Desired(1));
    }

    TEST_F(SignalTrackerFixture, APulseIgnoresEndEvents)
    {
        m_tracker.OnEvent(Event("hit", false));
        EXPECT_FALSE(m_tracker.Desired(1));
    }

    TEST_F(SignalTrackerFixture, ASignalNoBindingNamesChangesNothing)
    {
        m_tracker.OnEvent(Event("other", true));
        EXPECT_FALSE(m_tracker.Desired(0));
        EXPECT_FALSE(m_tracker.Desired(1));
    }

    //! A clip that is fading out must not open a window the agent then waits on.
    TEST_F(SignalTrackerFixture, AStartFromAFadingMotion_IsIgnored)
    {
        m_tracker.OnEvent(Event("combo", true, 0.2f));
        EXPECT_FALSE(m_tracker.Desired(0));
        EXPECT_EQ(m_tracker.GetSerial(), 0);

        m_tracker.OnEvent(Event("combo", true, 0.5f));
        EXPECT_TRUE(m_tracker.Desired(0));
    }

    //! The same fade must not stop an end event closing a window that was opened while the motion was strong.
    TEST_F(SignalTrackerFixture, AnEndFromAFadingMotion_StillClosesTheWindow)
    {
        m_tracker.OnEvent(Event("combo", true, 1.0f));
        m_tracker.OnEvent(Event("combo", false, 0.1f));
        EXPECT_FALSE(m_tracker.Desired(0));
    }

    TEST_F(SignalTrackerFixture, EveryAcceptedStart_BumpsTheSerialAndNamesTheSignal)
    {
        m_tracker.OnEvent(Event("combo", true));
        EXPECT_EQ(m_tracker.GetSerial(), 1);
        EXPECT_STREQ(m_tracker.GetLastSignal().c_str(), "combo");

        // The same signal twice still changes the serial, which is what lets a guard notice the repeat.
        m_tracker.OnEvent(Event("combo", true));
        EXPECT_EQ(m_tracker.GetSerial(), 2);

        m_tracker.OnEvent(Event("anything", true));
        EXPECT_EQ(m_tracker.GetSerial(), 3);
        EXPECT_STREQ(m_tracker.GetLastSignal().c_str(), "anything");
    }

    TEST_F(SignalTrackerFixture, AnEnd_DoesNotBumpTheSerial)
    {
        m_tracker.OnEvent(Event("combo", true));
        m_tracker.OnEvent(Event("combo", false));
        EXPECT_EQ(m_tracker.GetSerial(), 1);
    }

    TEST_F(SignalTrackerFixture, CloseAll_EndsWindowsAndPulses)
    {
        m_tracker.OnEvent(Event("combo", true));
        m_tracker.OnEvent(Event("hit", true));

        m_tracker.CloseAll();
        EXPECT_FALSE(m_tracker.Desired(0));
        EXPECT_FALSE(m_tracker.Desired(1));
    }

    TEST_F(SignalTrackerFixture, Configure_StartsOver)
    {
        m_tracker.OnEvent(Event("combo", true));
        m_tracker.Configure(m_bindings, 0.5f);

        EXPECT_FALSE(m_tracker.Desired(0));
        EXPECT_EQ(m_tracker.GetSerial(), 0);
        EXPECT_TRUE(m_tracker.GetLastSignal().empty());
    }

    TEST_F(SignalTrackerFixture, TwoBindingsOnOneSignal_BothFollowIt)
    {
        SignalBinding second;
        second.m_signal = "combo";
        second.m_key = "also_combo";
        m_bindings.push_back(second);
        m_tracker.Configure(m_bindings, 0.5f);

        m_tracker.OnEvent(Event("combo", true));
        EXPECT_TRUE(m_tracker.Desired(0));
        EXPECT_TRUE(m_tracker.Desired(2));
    }

    //! The component skips its tick while nothing is up, so this is what has to stay true until the last thing ends.
    TEST_F(SignalTrackerFixture, IsActive_IsTrueOnlyWhileAWindowIsOpenOrAPulseIsRunning)
    {
        EXPECT_FALSE(m_tracker.IsActive());

        m_tracker.OnEvent(Event("combo", true));
        EXPECT_TRUE(m_tracker.IsActive());
        m_tracker.OnEvent(Event("combo", false));
        EXPECT_FALSE(m_tracker.IsActive());

        m_tracker.OnEvent(Event("hit", true));
        EXPECT_TRUE(m_tracker.IsActive());
        m_tracker.Advance(PulseHoldSeconds + 0.01f);
        EXPECT_FALSE(m_tracker.IsActive());
    }
} // namespace GOAT_Animation

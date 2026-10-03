#include <Sensing/AwarenessMeter.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Perception
{
    //! Default profile: states begin at 5, 35 and 100, the meter drains 10 a second, and the three states last 10, 16 and 24 seconds unstimulated.
    class AwarenessMeterFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        PerceptionProfileAsset m_profile;
        AwarenessMeter m_meter;
    };

    TEST_F(AwarenessMeterFixture, StartsUnaware)
    {
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 0.0f);
    }

    TEST_F(AwarenessMeterFixture, AStimulusFillsTheMeterAndRaisesTheState)
    {
        EXPECT_FALSE(m_meter.Advance(m_profile, 0.05f, 60.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);

        // Six units in all, which is past the first edge at five.
        EXPECT_TRUE(m_meter.Advance(m_profile, 0.05f, 60.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Suspicious);
        EXPECT_NEAR(m_meter.GetValue(), 6.0f, 1.0e-4f);
    }

    TEST_F(AwarenessMeterFixture, TheMeterStopsAtTheCeiling)
    {
        m_meter.Advance(m_profile, 10.0f, 60.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Engaged);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), m_profile.m_engagedAt);
    }

    TEST_F(AwarenessMeterFixture, ASoundCanJumpPastAState)
    {
        EXPECT_TRUE(m_meter.Add(m_profile, 40.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Searching);
    }

    TEST_F(AwarenessMeterFixture, BelowTheFirstEdge_TheMeterDrains)
    {
        m_meter.Add(m_profile, 3.0f);
        m_meter.Advance(m_profile, 0.2f, 0.0f);
        EXPECT_NEAR(m_meter.GetValue(), 1.0f, 1.0e-4f);

        m_meter.Advance(m_profile, 5.0f, 0.0f);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);
    }

    //! A state is held at its own edge until its forget time is up, however fast the meter drains.
    TEST_F(AwarenessMeterFixture, ASuspiciousAgent_StaysSuspiciousUntilItsForgetTime)
    {
        m_meter.Add(m_profile, 10.0f);
        ASSERT_EQ(m_meter.GetState(), AwarenessState::Suspicious);

        m_meter.Advance(m_profile, 5.0f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Suspicious);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), m_profile.m_suspiciousAt);

        EXPECT_TRUE(m_meter.Advance(m_profile, 5.1f, 0.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 0.0f);
    }

    TEST_F(AwarenessMeterFixture, AFreshStimulusRestartsTheForgetTime)
    {
        m_meter.Add(m_profile, 10.0f);
        m_meter.Advance(m_profile, 9.0f, 0.0f);
        m_meter.Add(m_profile, 1.0f);
        m_meter.Advance(m_profile, 9.0f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Suspicious);
    }

    //! Each step down starts its own forget time, so Engaged takes the sight memory, then Searching its own, then Suspicious.
    TEST_F(AwarenessMeterFixture, AnEngagedAgent_StepsDownOneStateAtATime)
    {
        m_meter.Add(m_profile, 100.0f);
        ASSERT_EQ(m_meter.GetState(), AwarenessState::Engaged);

        m_meter.Advance(m_profile, 23.9f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Engaged);

        EXPECT_TRUE(m_meter.Advance(m_profile, 0.2f, 0.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Searching);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), m_profile.m_searchingAt);

        m_meter.Advance(m_profile, 15.0f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Searching);

        EXPECT_TRUE(m_meter.Advance(m_profile, 1.1f, 0.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Suspicious);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), m_profile.m_suspiciousAt);

        EXPECT_TRUE(m_meter.Advance(m_profile, 10.1f, 0.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);
    }

    TEST_F(AwarenessMeterFixture, RaiseTo_LiftsToTheStartOfAState)
    {
        EXPECT_TRUE(m_meter.RaiseTo(m_profile, AwarenessState::Searching));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Searching);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), m_profile.m_searchingAt);
    }

    TEST_F(AwarenessMeterFixture, RaiseTo_NeverLowersAnything)
    {
        m_meter.Add(m_profile, 100.0f);
        EXPECT_FALSE(m_meter.RaiseTo(m_profile, AwarenessState::Suspicious));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Engaged);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), m_profile.m_engagedAt);
    }

    TEST_F(AwarenessMeterFixture, AnUnawareAgentNeverStepsDown)
    {
        EXPECT_FALSE(m_meter.Advance(m_profile, 1000.0f, 0.0f));
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);
    }

    TEST_F(AwarenessMeterFixture, Reset_ForgetsEverything)
    {
        m_meter.Add(m_profile, 100.0f);
        m_meter.Reset();
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 0.0f);
    }

    //! With continuous drain on, the forget time is only a hold, then the value falls at the drain rate and the state follows it.
    TEST_F(AwarenessMeterFixture, ContinuousDrain_HoldsThenDrainsSmoothly)
    {
        m_profile.m_continuousDrain = true;
        m_meter.Add(m_profile, 100.0f);
        ASSERT_EQ(m_meter.GetState(), AwarenessState::Engaged);

        // The hold is the Engaged forget time, 24 seconds, and the value does not move during it.
        m_meter.Advance(m_profile, 23.9f, 0.0f);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 100.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Engaged);

        // One second past the hold has drained one second's worth.
        m_meter.Advance(m_profile, 1.1f, 0.0f);
        EXPECT_NEAR(m_meter.GetValue(), 90.0f, 1.0e-3f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Searching);
    }

    TEST_F(AwarenessMeterFixture, ContinuousDrain_TheStateFallsWithTheValue)
    {
        m_profile.m_continuousDrain = true;
        m_meter.Add(m_profile, 100.0f);
        m_meter.Advance(m_profile, 24.0f, 0.0f);

        // 100 down to 35 takes 6.5 seconds at ten a second, 35 down to 5 another 3.
        m_meter.Advance(m_profile, 6.4f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Searching);
        m_meter.Advance(m_profile, 0.2f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Suspicious);

        m_meter.Advance(m_profile, 3.0f, 0.0f);
        EXPECT_EQ(m_meter.GetState(), AwarenessState::Unaware);

        m_meter.Advance(m_profile, 100.0f, 0.0f);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 0.0f);
    }

    //! A step that crosses the end of the hold drains only the part after it.
    TEST_F(AwarenessMeterFixture, ContinuousDrain_AStepAcrossTheEndOfTheHold_DrainsOnlyTheRest)
    {
        m_profile.m_continuousDrain = true;
        m_meter.Add(m_profile, 100.0f);
        m_meter.Advance(m_profile, 20.0f, 0.0f);
        m_meter.Advance(m_profile, 6.0f, 0.0f);
        EXPECT_NEAR(m_meter.GetValue(), 80.0f, 1.0e-3f);
    }

    TEST_F(AwarenessMeterFixture, ContinuousDrain_AFreshStimulusRestartsTheHold)
    {
        m_profile.m_continuousDrain = true;
        m_meter.Add(m_profile, 100.0f);
        m_meter.Advance(m_profile, 30.0f, 0.0f);
        ASSERT_LT(m_meter.GetValue(), 100.0f);

        m_meter.Add(m_profile, 100.0f);
        m_meter.Advance(m_profile, 23.0f, 0.0f);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 100.0f);
    }

    //! Below the first edge there is no hold to wait out, so it drains at once, as it does without the option.
    TEST_F(AwarenessMeterFixture, ContinuousDrain_AnUnawareAgentDrainsAtOnce)
    {
        m_profile.m_continuousDrain = true;
        m_meter.Add(m_profile, 3.0f);
        m_meter.Advance(m_profile, 0.2f, 0.0f);
        EXPECT_NEAR(m_meter.GetValue(), 1.0f, 1.0e-4f);
    }

    //! A short hold is the state's own forget time: a brief suspicion lets go sooner than a long chase.
    TEST_F(AwarenessMeterFixture, ContinuousDrain_TheHoldIsTheForgetTimeOfTheStateReached)
    {
        m_profile.m_continuousDrain = true;
        m_meter.Add(m_profile, 10.0f);
        ASSERT_EQ(m_meter.GetState(), AwarenessState::Suspicious);

        m_meter.Advance(m_profile, 9.9f, 0.0f);
        EXPECT_FLOAT_EQ(m_meter.GetValue(), 10.0f);

        m_meter.Advance(m_profile, 0.2f, 0.0f);
        EXPECT_NEAR(m_meter.GetValue(), 9.0f, 1.0e-3f);
    }

    TEST_F(AwarenessMeterFixture, StateForValue_ReadsTheEdges)
    {
        EXPECT_EQ(StateForValue(m_profile, 4.9f), AwarenessState::Unaware);
        EXPECT_EQ(StateForValue(m_profile, 5.0f), AwarenessState::Suspicious);
        EXPECT_EQ(StateForValue(m_profile, 35.0f), AwarenessState::Searching);
        EXPECT_EQ(StateForValue(m_profile, 100.0f), AwarenessState::Engaged);
    }
} // namespace GOAT_Perception

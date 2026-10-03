#include <Sensing/HearingModel.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Perception
{
    //! Runs against the default profile: hearing range 10 m, a hearing fill of 40, and walls that do not muffle.
    class HearingModelFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        PerceptionProfileAsset m_profile;
    };

    TEST_F(HearingModelFixture, ASoundWithinRange_IsHeard)
    {
        const HeardSound heard = EvaluateSound(m_profile, 5.0f, 1.0f, false);
        EXPECT_TRUE(heard.m_heard);
        EXPECT_FLOAT_EQ(heard.m_fill, 30.0f);
    }

    TEST_F(HearingModelFixture, ASoundPointBlank_AddsTheFullFill)
    {
        EXPECT_FLOAT_EQ(EvaluateSound(m_profile, 0.0f, 1.0f, false).m_fill, 40.0f);
    }

    TEST_F(HearingModelFixture, ASoundAtTheEdgeOfRange_AddsHalf)
    {
        const HeardSound heard = EvaluateSound(m_profile, 10.0f, 1.0f, false);
        EXPECT_TRUE(heard.m_heard);
        EXPECT_FLOAT_EQ(heard.m_fill, 20.0f);
    }

    TEST_F(HearingModelFixture, ASoundBeyondRange_IsNotHeard)
    {
        EXPECT_FALSE(EvaluateSound(m_profile, 10.1f, 1.0f, false).m_heard);
    }

    TEST_F(HearingModelFixture, LoudnessStretchesTheRange)
    {
        EXPECT_FALSE(EvaluateSound(m_profile, 15.0f, 1.0f, false).m_heard);
        EXPECT_TRUE(EvaluateSound(m_profile, 15.0f, 2.0f, false).m_heard);
    }

    TEST_F(HearingModelFixture, ABlockedPath_CostsTheWallCut)
    {
        m_profile.m_hearingWallCut = 4.0f;

        EXPECT_TRUE(EvaluateSound(m_profile, 5.0f, 1.0f, true).m_heard);
        EXPECT_FALSE(EvaluateSound(m_profile, 7.0f, 1.0f, true).m_heard);
        EXPECT_TRUE(EvaluateSound(m_profile, 7.0f, 1.0f, false).m_heard);
    }

    TEST_F(HearingModelFixture, AWallCutBiggerThanTheRange_MutesTheSound)
    {
        m_profile.m_hearingWallCut = 50.0f;
        EXPECT_FALSE(EvaluateSound(m_profile, 0.5f, 1.0f, true).m_heard);
    }

    TEST_F(HearingModelFixture, ASoundQuieterThanTheMinimum_IsNeverHeard)
    {
        m_profile.m_hearingMinLoudness = 0.5f;
        EXPECT_FALSE(EvaluateSound(m_profile, 1.0f, 0.3f, false).m_heard);
        EXPECT_TRUE(EvaluateSound(m_profile, 1.0f, 0.6f, false).m_heard);
    }

    TEST_F(HearingModelFixture, SilenceIsNeverHeard)
    {
        EXPECT_FALSE(EvaluateSound(m_profile, 0.0f, 0.0f, false).m_heard);
        EXPECT_FALSE(EvaluateSound(m_profile, 0.0f, -1.0f, false).m_heard);
    }
} // namespace GOAT_Perception

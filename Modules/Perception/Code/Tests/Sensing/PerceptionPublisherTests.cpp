#include <Sensing/PerceptionPublisher.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Perception
{
    using PerceptionPublisherFixture = UnitTest::LeakDetectionFixture;

    //! A tree condition reads a Bool, so each state a tree reacts to needs one.
    TEST_F(PerceptionPublisherFixture, StateFlags_AreAtLeastSuspiciousAtLeastSearchingAndExactlyEngaged)
    {
        const StateFlags unaware = FlagsFor(AwarenessState::Unaware);
        EXPECT_FALSE(unaware.m_suspicious);
        EXPECT_FALSE(unaware.m_searching);
        EXPECT_FALSE(unaware.m_engaged);

        const StateFlags suspicious = FlagsFor(AwarenessState::Suspicious);
        EXPECT_TRUE(suspicious.m_suspicious);
        EXPECT_FALSE(suspicious.m_searching);
        EXPECT_FALSE(suspicious.m_engaged);

        const StateFlags searching = FlagsFor(AwarenessState::Searching);
        EXPECT_TRUE(searching.m_suspicious);
        EXPECT_TRUE(searching.m_searching);
        EXPECT_FALSE(searching.m_engaged);

        const StateFlags engaged = FlagsFor(AwarenessState::Engaged);
        EXPECT_TRUE(engaged.m_suspicious);
        EXPECT_TRUE(engaged.m_searching);
        EXPECT_TRUE(engaged.m_engaged);
    }

    TEST_F(PerceptionPublisherFixture, LastKnown_IgnoresSmallMovesWhileTheTargetIsInSight)
    {
        const AZ::Vector3 published(0.0f, 10.0f, 0.0f);
        EXPECT_FALSE(ShouldPublishLastKnown(published, AZ::Vector3(0.0f, 10.4f, 0.0f), true, true));
        EXPECT_TRUE(ShouldPublishLastKnown(published, AZ::Vector3(0.0f, 10.6f, 0.0f), true, true));
    }

    //! The place the target was last seen has to land even if it barely moved.
    TEST_F(PerceptionPublisherFixture, LastKnown_IsWrittenWhenSightIsLost)
    {
        const AZ::Vector3 place(0.0f, 10.0f, 0.0f);
        EXPECT_TRUE(ShouldPublishLastKnown(place, place, true, false));
        EXPECT_FALSE(ShouldPublishLastKnown(place, place, false, false));
    }

    TEST_F(PerceptionPublisherFixture, Awareness_IsWrittenWhenTheStateChanges)
    {
        EXPECT_TRUE(ShouldPublishAwareness(20.0f, 20.5f, AwarenessState::Unaware, AwarenessState::Suspicious, 100.0f));
    }

    TEST_F(PerceptionPublisherFixture, Awareness_WaitsForATenthOfTheCeilingWithinAState)
    {
        EXPECT_FALSE(ShouldPublishAwareness(40.0f, 45.0f, AwarenessState::Searching, AwarenessState::Searching, 100.0f));
        EXPECT_TRUE(ShouldPublishAwareness(40.0f, 50.0f, AwarenessState::Searching, AwarenessState::Searching, 100.0f));
        EXPECT_TRUE(ShouldPublishAwareness(50.0f, 38.0f, AwarenessState::Searching, AwarenessState::Searching, 100.0f));
    }
} // namespace GOAT_Perception

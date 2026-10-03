#include <Sensing/SightGeometry.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Perception
{
    //! The eye sits at the origin facing +Y with Z up, looking through a 20 m, 90 degree by 60 degree cone.
    class SightGeometryFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        SightCone Cone() const { return SightCone{ 20.0f, 90.0f, 60.0f, 0.0f }; }

        EyePose Eye() const { return EyePose(); }
    };

    TEST_F(SightGeometryFixture, AheadAndInRange_IsInside)
    {
        EXPECT_TRUE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 10.0f, 0.0f)));
    }

    TEST_F(SightGeometryFixture, BeyondTheRange_IsOutside)
    {
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 25.0f, 0.0f)));
        EXPECT_TRUE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 19.9f, 0.0f)));
    }

    TEST_F(SightGeometryFixture, ARangeScaleShrinksTheCone)
    {
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 15.0f, 0.0f), 0.5f));
        EXPECT_TRUE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 8.0f, 0.0f), 0.5f));
    }

    //! The cone is 90 degrees wide, so half of it is 45 degrees either side of the facing.
    TEST_F(SightGeometryFixture, TheHorizontalAngleIsMeasuredFromTheFacing)
    {
        EXPECT_TRUE(InsideSightCone(Eye(), Cone(), AZ::Vector3(5.0f, 10.0f, 0.0f)));
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(10.0f, 5.0f, 0.0f)));
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(-10.0f, 5.0f, 0.0f)));
    }

    TEST_F(SightGeometryFixture, BehindTheAgent_IsOutsideUnlessTheConeIsAFullTurn)
    {
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, -5.0f, 0.0f)));

        SightCone fullTurn = Cone();
        fullTurn.m_yawDegrees = 360.0f;
        EXPECT_TRUE(InsideSightCone(Eye(), fullTurn, AZ::Vector3(0.0f, -5.0f, 0.0f)));
    }

    //! The cone is 60 degrees tall, so half of it is 30 degrees up or down.
    TEST_F(SightGeometryFixture, TheVerticalAngleIsMeasuredFromTheFacing)
    {
        EXPECT_TRUE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 10.0f, 5.0f)));
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 10.0f, 10.0f)));
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), AZ::Vector3(0.0f, 10.0f, -10.0f)));
    }

    //! Pulling the origin back means a target touching the agent's back is still ahead of it.
    TEST_F(SightGeometryFixture, ABackOffsetCoversATargetJustBehind)
    {
        const AZ::Vector3 justBehind(0.0f, -1.0f, 0.0f);
        EXPECT_FALSE(InsideSightCone(Eye(), Cone(), justBehind));

        SightCone offset = Cone();
        offset.m_backOffset = 2.0f;
        EXPECT_TRUE(InsideSightCone(Eye(), offset, justBehind));
    }

    TEST_F(SightGeometryFixture, ARangeOfZeroSeesNothing)
    {
        SightCone blind = Cone();
        blind.m_range = 0.0f;
        EXPECT_FALSE(InsideSightCone(Eye(), blind, AZ::Vector3(0.0f, 1.0f, 0.0f)));
    }

    //! Looking straight up has no horizontal direction, so only the vertical angle can rule a target out.
    TEST_F(SightGeometryFixture, LookingStraightUp_SeesWhatIsAbove)
    {
        EyePose up;
        up.m_forward = AZ::Vector3::CreateAxisZ();
        EXPECT_TRUE(InsideSightCone(up, Cone(), AZ::Vector3(0.0f, 0.0f, 10.0f)));
        EXPECT_FALSE(InsideSightCone(up, Cone(), AZ::Vector3(0.0f, 10.0f, 0.0f)));
    }

    TEST_F(SightGeometryFixture, ATargetOnTheEyeIsInside)
    {
        EXPECT_TRUE(InsideSightCone(Eye(), Cone(), AZ::Vector3::CreateZero()));
    }

    TEST_F(SightGeometryFixture, Closeness_FallsFromOneToAQuarterAtTheEdge)
    {
        EXPECT_FLOAT_EQ(SightCloseness(0.0f, 20.0f), 1.0f);
        EXPECT_FLOAT_EQ(SightCloseness(10.0f, 20.0f), 0.625f);
        EXPECT_FLOAT_EQ(SightCloseness(20.0f, 20.0f), 0.25f);
        EXPECT_FLOAT_EQ(SightCloseness(40.0f, 20.0f), 0.25f);
        EXPECT_FLOAT_EQ(SightCloseness(5.0f, 0.0f), 0.0f);
    }
} // namespace GOAT_Perception

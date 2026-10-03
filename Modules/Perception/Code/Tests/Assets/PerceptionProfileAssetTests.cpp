#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/limits.h>
#include <AzTest/AzTest.h>

namespace GOAT_Perception
{
    //! Reflects the asset into a bare serialize context with an edit context, since a wrong Asset Editor attribute only shows up when built.
    class PerceptionProfileAssetFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            m_context = AZStd::make_unique<AZ::SerializeContext>();
            m_context->CreateEditContext();
            AZ::Data::AssetData::Reflect(m_context.get());
            PerceptionProfileAsset::Reflect(m_context.get());
        }

        void TearDown() override
        {
            m_context->DestroyEditContext();
            m_context.reset();
            UnitTest::LeakDetectionFixture::TearDown();
        }

        //! Saves the asset the way the Asset Editor does and reads it back.
        AZStd::unique_ptr<PerceptionProfileAsset> RoundTrip(const PerceptionProfileAsset& source)
        {
            AZStd::vector<char> buffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
            EXPECT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_XML, &source, m_context.get()));

            return AZStd::unique_ptr<PerceptionProfileAsset>(
                AZ::Utils::LoadObjectFromBuffer<PerceptionProfileAsset>(buffer.data(), buffer.size(), m_context.get()));
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_context;
    };

    TEST_F(PerceptionProfileAssetFixture, Defaults_AreValid)
    {
        const PerceptionProfileAsset profile;
        const auto outcome = profile.Validate();
        EXPECT_TRUE(outcome.IsSuccess()) << outcome.GetError().c_str();
    }

    //! The staircase drain is what a profile saved before the option existed gets, so it stays the default.
    TEST_F(PerceptionProfileAssetFixture, ContinuousDrain_IsOffByDefault)
    {
        const PerceptionProfileAsset profile;
        EXPECT_FALSE(profile.m_continuousDrain);
    }

    TEST_F(PerceptionProfileAssetFixture, AssetEditor_ListsTheTypeUnderItsExtension)
    {
        EXPECT_STREQ(PerceptionProfileAsset::FileExtension, "prx");
        EXPECT_STREQ(PerceptionProfileAsset::AssetGroup, "GOAT");

        const AZ::SerializeContext::ClassData* data = m_context->FindClassData(azrtti_typeid<PerceptionProfileAsset>());
        ASSERT_NE(data, nullptr);
        EXPECT_NE(AZ::FindAttribute(AZ::Edit::Attributes::EnableForAssetEditor, data->m_attributes), nullptr);
    }

    TEST_F(PerceptionProfileAssetFixture, RoundTrip_KeepsEveryAuthoredValue)
    {
        PerceptionProfileAsset source;
        source.m_idleSight = SightCone{ 31.0f, 120.0f, 45.0f, 1.5f };
        source.m_alertSight = SightCone{ 12.0f, 200.0f, 70.0f, 0.5f };
        source.m_combatSight = SightCone{ 77.0f, 360.0f, 180.0f, 2.0f };
        source.m_darknessSightScale = 0.25f;
        source.m_sightBlockerMask = 0xA5u;
        source.m_hearingRange = 14.0f;
        source.m_hearingWallCut = 3.0f;
        source.m_hearingMinLoudness = 0.2f;
        source.m_proximityRange = 2.5f;
        source.m_suspiciousAt = 8.0f;
        source.m_searchingAt = 40.0f;
        source.m_engagedAt = 120.0f;
        source.m_drainPerSecond = 6.0f;
        source.m_continuousDrain = true;
        source.m_engageDistance = 9.0f;
        source.m_sightFillPerSecond = 45.0f;
        source.m_hearingFill = 25.0f;
        source.m_suspiciousForget = 11.0f;
        source.m_searchingForget = 17.0f;
        source.m_sightMemory = 25.0f;
        source.m_soundMemory = 12.0f;
        source.m_targetTags = { "player", "decoy" };
        source.m_damageAwareness = 60.0f;
        source.m_alertRadius = 30.0f;
        source.m_alertSquad = "wolves";
        source.m_alertReplyDelay = 0.5f;
        source.m_alertReplyJitter = 1.5f;
        source.m_senseInterval = 0.4f;

        const auto loaded = RoundTrip(source);
        ASSERT_NE(loaded, nullptr);

        EXPECT_FLOAT_EQ(loaded->m_idleSight.m_range, 31.0f);
        EXPECT_FLOAT_EQ(loaded->m_idleSight.m_yawDegrees, 120.0f);
        EXPECT_FLOAT_EQ(loaded->m_alertSight.m_pitchDegrees, 70.0f);
        EXPECT_FLOAT_EQ(loaded->m_combatSight.m_backOffset, 2.0f);
        EXPECT_FLOAT_EQ(loaded->m_darknessSightScale, 0.25f);
        EXPECT_EQ(loaded->m_sightBlockerMask, 0xA5u);
        EXPECT_FLOAT_EQ(loaded->m_hearingRange, 14.0f);
        EXPECT_FLOAT_EQ(loaded->m_hearingWallCut, 3.0f);
        EXPECT_FLOAT_EQ(loaded->m_hearingMinLoudness, 0.2f);
        EXPECT_FLOAT_EQ(loaded->m_proximityRange, 2.5f);
        EXPECT_FLOAT_EQ(loaded->m_suspiciousAt, 8.0f);
        EXPECT_FLOAT_EQ(loaded->m_searchingAt, 40.0f);
        EXPECT_FLOAT_EQ(loaded->m_engagedAt, 120.0f);
        EXPECT_FLOAT_EQ(loaded->m_drainPerSecond, 6.0f);
        EXPECT_TRUE(loaded->m_continuousDrain);
        EXPECT_FLOAT_EQ(loaded->m_engageDistance, 9.0f);
        EXPECT_FLOAT_EQ(loaded->m_sightFillPerSecond, 45.0f);
        EXPECT_FLOAT_EQ(loaded->m_hearingFill, 25.0f);
        EXPECT_FLOAT_EQ(loaded->m_suspiciousForget, 11.0f);
        EXPECT_FLOAT_EQ(loaded->m_searchingForget, 17.0f);
        EXPECT_FLOAT_EQ(loaded->m_sightMemory, 25.0f);
        EXPECT_FLOAT_EQ(loaded->m_soundMemory, 12.0f);
        ASSERT_EQ(loaded->m_targetTags.size(), 2u);
        EXPECT_EQ(loaded->m_targetTags[0], "player");
        EXPECT_EQ(loaded->m_targetTags[1], "decoy");
        EXPECT_FLOAT_EQ(loaded->m_damageAwareness, 60.0f);
        EXPECT_FLOAT_EQ(loaded->m_alertRadius, 30.0f);
        EXPECT_EQ(loaded->m_alertSquad, "wolves");
        EXPECT_FLOAT_EQ(loaded->m_alertReplyDelay, 0.5f);
        EXPECT_FLOAT_EQ(loaded->m_alertReplyJitter, 1.5f);
        EXPECT_FLOAT_EQ(loaded->m_senseInterval, 0.4f);
        EXPECT_TRUE(loaded->Validate().IsSuccess());
    }

    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesANegativeRange)
    {
        PerceptionProfileAsset hearing;
        hearing.m_hearingRange = -1.0f;
        EXPECT_FALSE(hearing.Validate().IsSuccess());

        PerceptionProfileAsset sight;
        sight.m_combatSight.m_range = -0.1f;
        EXPECT_FALSE(sight.Validate().IsSuccess());
    }

    //! A cone wider than a full turn is a typo, not a wider cone.
    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesAnImpossibleSightAngle)
    {
        PerceptionProfileAsset yaw;
        yaw.m_alertSight.m_yawDegrees = 361.0f;
        EXPECT_FALSE(yaw.Validate().IsSuccess());

        PerceptionProfileAsset pitch;
        pitch.m_idleSight.m_pitchDegrees = 181.0f;
        EXPECT_FALSE(pitch.Validate().IsSuccess());

        PerceptionProfileAsset fullTurn;
        fullTurn.m_idleSight.m_yawDegrees = 360.0f;
        fullTurn.m_idleSight.m_pitchDegrees = 180.0f;
        EXPECT_TRUE(fullTurn.Validate().IsSuccess());
    }

    //! The states are reached in order, so the edges between them have to rise.
    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesThresholdsOutOfOrder)
    {
        PerceptionProfileAsset first;
        first.m_suspiciousAt = 50.0f;
        first.m_searchingAt = 35.0f;
        const auto outcome = first.Validate();
        ASSERT_FALSE(outcome.IsSuccess());
        EXPECT_NE(outcome.GetError().find("thresholds"), AZStd::string::npos);

        PerceptionProfileAsset second;
        second.m_searchingAt = 150.0f;
        second.m_engagedAt = 100.0f;
        EXPECT_FALSE(second.Validate().IsSuccess());

        PerceptionProfileAsset allZero;
        allZero.m_suspiciousAt = 0.0f;
        allZero.m_searchingAt = 0.0f;
        allZero.m_engagedAt = 0.0f;
        EXPECT_FALSE(allZero.Validate().IsSuccess());
    }

    //! A zero interval would have the sensing pass spin without ever moving time on.
    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesASenseIntervalOfZero)
    {
        PerceptionProfileAsset profile;
        profile.m_senseInterval = 0.0f;
        EXPECT_FALSE(profile.Validate().IsSuccess());
    }

    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesANegativeFill)
    {
        PerceptionProfileAsset sight;
        sight.m_sightFillPerSecond = -1.0f;
        EXPECT_FALSE(sight.Validate().IsSuccess());

        PerceptionProfileAsset hearing;
        hearing.m_hearingFill = -1.0f;
        EXPECT_FALSE(hearing.Validate().IsSuccess());
    }

    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesNotANumber)
    {
        PerceptionProfileAsset notANumber;
        notANumber.m_hearingRange = AZStd::numeric_limits<float>::quiet_NaN();
        EXPECT_FALSE(notANumber.Validate().IsSuccess());

        PerceptionProfileAsset infinite;
        infinite.m_engagedAt = AZStd::numeric_limits<float>::infinity();
        EXPECT_FALSE(infinite.Validate().IsSuccess());
    }

    TEST_F(PerceptionProfileAssetFixture, Validate_RefusesADarknessScaleAboveOne)
    {
        PerceptionProfileAsset profile;
        profile.m_darknessSightScale = 1.5f;
        EXPECT_FALSE(profile.Validate().IsSuccess());

        profile.m_darknessSightScale = 0.0f;
        EXPECT_TRUE(profile.Validate().IsSuccess());
    }

    //! The first problem is reported, and it names the field that caused it.
    TEST_F(PerceptionProfileAssetFixture, Validate_NamesTheProblem)
    {
        PerceptionProfileAsset profile;
        profile.m_proximityRange = -2.0f;
        const auto outcome = profile.Validate();
        ASSERT_FALSE(outcome.IsSuccess());
        EXPECT_NE(outcome.GetError().find("Proximity Range"), AZStd::string::npos);
    }
} // namespace GOAT_Perception

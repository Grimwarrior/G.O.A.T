#include <Sensing/AgentSenses.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Perception
{
    //! A world with no physics: poses are never asked for here, and sight is either clear or not.
    class FakeWorld final
        : public IPerceptionWorld
    {
    public:
        bool GetPose(AZ::EntityId, EntityPose&) const override { return false; }

        bool HasLineOfSight(
            const AZ::Vector3&, const AZ::Vector3&, AZ::u32, AZ::EntityId, AZ::EntityId, AZ::EntityId* outBlocker) const override
        {
            if (!m_clear && outBlocker != nullptr)
            {
                *outBlocker = m_blocker;
            }
            return m_clear;
        }

        bool m_clear = true;
        AZ::EntityId m_blocker;
    };

    //! The agent stands at the origin facing +Y with the default profile, except that engaging at a distance is off so awareness is earned.
    class AgentSensesFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();
            m_profile.m_engageDistance = 0.0f;
            m_senses.SetOwner(m_owner);
        }

        SenseCandidate Candidate(AZ::u64 id, float x, float y, float z = 0.0f) const
        {
            SenseCandidate candidate;
            candidate.m_entity = AZ::EntityId(id);
            candidate.m_position = AZ::Vector3(x, y, z);
            return candidate;
        }

        void Update(AZStd::span<const SenseCandidate> candidates, float deltaTime)
        {
            m_senses.Update(m_profile, m_world, m_eye, candidates, deltaTime);
        }

        const AZ::EntityId m_owner = AZ::EntityId(1);
        PerceptionProfileAsset m_profile;
        FakeWorld m_world;
        EyePose m_eye;
        AgentSenses m_senses;
    };

    //! Ten metres ahead is half the 20 m range, so it fills at 60 x 0.625 = 37.5 a second.
    TEST_F(AgentSensesFixture, ASightedTarget_RaisesAwarenessAndIsReported)
    {
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.2f);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Suspicious);
        EXPECT_EQ(outputs.m_target, AZ::EntityId(2));
        EXPECT_TRUE(outputs.m_targetVisible);
        EXPECT_TRUE(outputs.m_lastKnown.IsClose(AZ::Vector3(0.0f, 10.0f, 0.0f)));
        EXPECT_NEAR(outputs.m_awareness, 7.5f, 1.0e-3f);
    }

    TEST_F(AgentSensesFixture, ATargetBehindAWall_IsNotSeen)
    {
        m_world.m_clear = false;
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 1.0f);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Unaware);
        EXPECT_FALSE(outputs.m_targetVisible);
        EXPECT_FALSE(outputs.m_target.IsValid());
    }

    //! A sensor that sees nothing has to be able to say what it was looking at and what was in the way.
    TEST_F(AgentSensesFixture, ATargetInTheConeButOutOfSight_IsRememberedWithWhatBlockedIt)
    {
        m_world.m_clear = false;
        m_world.m_blocker = AZ::EntityId(55);
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.2f);

        const SenseDebug& debug = m_senses.GetDebug();
        EXPECT_TRUE(debug.HasBlockedView());
        EXPECT_EQ(debug.m_blockedTarget, AZ::EntityId(2));
        EXPECT_EQ(debug.m_blocker, AZ::EntityId(55));
        EXPECT_NEAR(debug.m_blockedDistance, 10.0f, 1.0e-3f);
    }

    TEST_F(AgentSensesFixture, ATargetThatIsSeen_LeavesNothingBlocked)
    {
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.2f);
        EXPECT_FALSE(m_senses.GetDebug().HasBlockedView());
    }

    TEST_F(AgentSensesFixture, TheBlockedRecord_IsForTheLastPassOnly)
    {
        m_world.m_clear = false;
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.2f);
        ASSERT_TRUE(m_senses.GetDebug().HasBlockedView());

        m_world.m_clear = true;
        Update({ &ahead, 1 }, 0.2f);
        EXPECT_FALSE(m_senses.GetDebug().HasBlockedView());
    }

    TEST_F(AgentSensesFixture, ATargetOutsideTheCone_IsNotSeen)
    {
        const SenseCandidate behind = Candidate(2, 0.0f, -10.0f);
        Update({ &behind, 1 }, 1.0f);
        EXPECT_EQ(m_senses.GetState(), AwarenessState::Unaware);
    }

    //! Something at the agent's elbow is noticed whatever it faces and whatever stands between.
    TEST_F(AgentSensesFixture, ProximityNeedsNeitherTheConeNorALineOfSight)
    {
        m_world.m_clear = false;
        const SenseCandidate close = Candidate(2, 0.0f, -0.5f);
        Update({ &close, 1 }, 0.2f);

        EXPECT_EQ(m_senses.GetState(), AwarenessState::Suspicious);
        EXPECT_EQ(m_senses.GetOutputs().m_target, AZ::EntityId(2));
    }

    TEST_F(AgentSensesFixture, TheAgentNeverSensesItself)
    {
        const SenseCandidate self = Candidate(1, 0.0f, 0.5f);
        Update({ &self, 1 }, 5.0f);
        EXPECT_EQ(m_senses.GetState(), AwarenessState::Unaware);
    }

    TEST_F(AgentSensesFixture, TheNearestSightedTargetWins)
    {
        const SenseCandidate candidates[] = { Candidate(2, 0.0f, 15.0f), Candidate(3, 0.0f, 8.0f) };
        Update(candidates, 0.2f);
        EXPECT_EQ(m_senses.GetOutputs().m_target, AZ::EntityId(3));
    }

    TEST_F(AgentSensesFixture, ANearerTargetThatIsBlocked_DoesNotHideAFarOne)
    {
        // Only the nearer one is behind something, so the farther one is the one seen.
        class SelectiveWorld final
            : public IPerceptionWorld
        {
        public:
            bool GetPose(AZ::EntityId, EntityPose&) const override { return false; }
            bool HasLineOfSight(
                const AZ::Vector3&, const AZ::Vector3&, AZ::u32, AZ::EntityId, AZ::EntityId target, AZ::EntityId*) const override
            {
                return target != AZ::EntityId(3);
            }
        } world;

        const SenseCandidate candidates[] = { Candidate(2, 0.0f, 15.0f), Candidate(3, 0.0f, 8.0f) };
        m_senses.Update(m_profile, world, m_eye, candidates, 0.2f);
        EXPECT_EQ(m_senses.GetOutputs().m_target, AZ::EntityId(2));
    }

    TEST_F(AgentSensesFixture, ACloseVisibleTarget_SkipsStraightToEngaged)
    {
        m_profile.m_engageDistance = 15.0f;
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.05f);

        EXPECT_EQ(m_senses.GetState(), AwarenessState::Engaged);

        bool calledByAnother = true;
        EXPECT_TRUE(m_senses.TakeEngagedEdge(calledByAnother));
        EXPECT_FALSE(calledByAnother);
        EXPECT_FALSE(m_senses.TakeEngagedEdge(calledByAnother));
    }

    //! A target the agent merely glimpsed has not yet earned a reaction from a tree.
    TEST_F(AgentSensesFixture, AGlimpseBelowTheFirstEdge_IsNotReported)
    {
        m_profile.m_sightFillPerSecond = 10.0f;
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.1f);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Unaware);
        EXPECT_FALSE(outputs.m_target.IsValid());
        EXPECT_FALSE(outputs.m_targetVisible);
    }

    TEST_F(AgentSensesFixture, LosingSight_KeepsTheTargetAndItsLastKnownPlace)
    {
        m_profile.m_engageDistance = 15.0f;
        const SenseCandidate ahead = Candidate(2, 3.0f, 10.0f);
        Update({ &ahead, 1 }, 0.05f);
        ASSERT_EQ(m_senses.GetState(), AwarenessState::Engaged);

        Update({}, 5.0f);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Engaged);
        EXPECT_FALSE(outputs.m_targetVisible);
        EXPECT_EQ(outputs.m_target, AZ::EntityId(2));
        EXPECT_TRUE(outputs.m_lastKnown.IsClose(AZ::Vector3(3.0f, 10.0f, 0.0f)));
    }

    TEST_F(AgentSensesFixture, GivingUp_ClearsTheTarget)
    {
        m_profile.m_engageDistance = 15.0f;
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 0.05f);
        ASSERT_EQ(m_senses.GetState(), AwarenessState::Engaged);

        // Sight memory, then the searching and suspicious forget times, all with nothing in view.
        Update({}, 24.1f);
        Update({}, 16.1f);
        Update({}, 10.1f);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Unaware);
        EXPECT_FALSE(outputs.m_target.IsValid());
    }

    TEST_F(AgentSensesFixture, ASound_RaisesAwarenessAndIsRemembered)
    {
        NoiseEvent noise;
        noise.m_position = AZ::Vector3(0.0f, -5.0f, 0.0f);
        noise.m_loudness = 1.0f;
        noise.m_source = AZ::EntityId(2);
        m_senses.Hear(m_profile, m_world, m_eye, noise);

        SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Suspicious);
        EXPECT_TRUE(outputs.m_heard);
        EXPECT_TRUE(outputs.m_noisePosition.IsClose(noise.m_position));

        // A sound says where, not who, so it does not name a target.
        EXPECT_FALSE(outputs.m_target.IsValid());

        Update({}, m_profile.m_soundMemory + 1.0f);
        EXPECT_FALSE(m_senses.GetOutputs().m_heard);
    }

    TEST_F(AgentSensesFixture, ASoundOutOfRange_IsIgnored)
    {
        NoiseEvent noise;
        noise.m_position = AZ::Vector3(0.0f, 30.0f, 0.0f);
        m_senses.Hear(m_profile, m_world, m_eye, noise);

        EXPECT_EQ(m_senses.GetState(), AwarenessState::Unaware);
        EXPECT_FALSE(m_senses.GetOutputs().m_heard);
    }

    TEST_F(AgentSensesFixture, TheAgentIgnoresItsOwnNoise)
    {
        NoiseEvent noise;
        noise.m_position = AZ::Vector3(0.0f, 1.0f, 0.0f);
        noise.m_source = m_owner;
        m_senses.Hear(m_profile, m_world, m_eye, noise);
        EXPECT_EQ(m_senses.GetState(), AwarenessState::Unaware);
    }

    TEST_F(AgentSensesFixture, BeingHit_EngagesAndNamesTheAttacker)
    {
        const AZ::Vector3 where(4.0f, -6.0f, 0.0f);
        m_senses.ApplyDamage(m_profile, AZ::EntityId(7), &where);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Engaged);
        EXPECT_EQ(outputs.m_target, AZ::EntityId(7));
        EXPECT_TRUE(outputs.m_lastKnown.IsClose(where));

        bool calledByAnother = true;
        EXPECT_TRUE(m_senses.TakeEngagedEdge(calledByAnother));
        EXPECT_FALSE(calledByAnother);
    }

    TEST_F(AgentSensesFixture, BeingToldByAnAlly_EngagesWithoutCallingForHelpAgain)
    {
        const AZ::Vector3 where(1.0f, 2.0f, 3.0f);
        m_senses.ReceiveAlert(m_profile, AZ::EntityId(9), where);

        const SenseOutputs outputs = m_senses.GetOutputs();
        EXPECT_EQ(outputs.m_state, AwarenessState::Engaged);
        EXPECT_EQ(outputs.m_target, AZ::EntityId(9));

        bool calledByAnother = false;
        EXPECT_TRUE(m_senses.TakeEngagedEdge(calledByAnother));
        EXPECT_TRUE(calledByAnother);
    }

    TEST_F(AgentSensesFixture, AnAgentAlreadyEngaged_DoesNotRaiseTheEdgeAgain)
    {
        const AZ::Vector3 where = AZ::Vector3::CreateZero();
        m_senses.ApplyDamage(m_profile, AZ::EntityId(7), &where);

        bool calledByAnother = false;
        ASSERT_TRUE(m_senses.TakeEngagedEdge(calledByAnother));

        m_senses.ApplyDamage(m_profile, AZ::EntityId(7), &where);
        EXPECT_FALSE(m_senses.TakeEngagedEdge(calledByAnother));
    }

    TEST_F(AgentSensesFixture, ASightFillOfZero_MeansSightAddsNothing)
    {
        m_profile.m_sightFillPerSecond = 0.0f;
        const SenseCandidate ahead = Candidate(2, 0.0f, 10.0f);
        Update({ &ahead, 1 }, 10.0f);
        EXPECT_EQ(m_senses.GetState(), AwarenessState::Unaware);
    }
} // namespace GOAT_Perception

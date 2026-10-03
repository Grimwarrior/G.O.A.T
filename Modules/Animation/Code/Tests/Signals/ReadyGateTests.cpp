#include <Signals/ReadyGate.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace GOAT_Animation
{
    using ReadyGateFixture = UnitTest::LeakDetectionFixture;

    TEST_F(ReadyGateFixture, Parse_ReadsPlainAndNegatedNames)
    {
        const AZStd::vector<AZStd::string> texts = { "target_in_reach", "!stunned" };
        const AZStd::vector<Requirement> requirements = ParseRequirements(texts);

        ASSERT_EQ(requirements.size(), 2u);
        EXPECT_EQ(requirements[0].m_name, "target_in_reach");
        EXPECT_FALSE(requirements[0].m_negated);
        EXPECT_EQ(requirements[1].m_name, "stunned");
        EXPECT_TRUE(requirements[1].m_negated);
    }

    TEST_F(ReadyGateFixture, Parse_TrimsSpacesAroundTheNameAndTheBang)
    {
        const AZStd::vector<AZStd::string> texts = { "  target_in_reach\t", " ! stunned " };
        const AZStd::vector<Requirement> requirements = ParseRequirements(texts);

        ASSERT_EQ(requirements.size(), 2u);
        EXPECT_EQ(requirements[0].m_name, "target_in_reach");
        EXPECT_EQ(requirements[1].m_name, "stunned");
        EXPECT_TRUE(requirements[1].m_negated);
    }

    TEST_F(ReadyGateFixture, Parse_SkipsBlankEntries)
    {
        const AZStd::vector<AZStd::string> texts = { "", "   ", "!", " ! ", "alive" };
        const AZStd::vector<Requirement> requirements = ParseRequirements(texts);

        ASSERT_EQ(requirements.size(), 1u);
        EXPECT_EQ(requirements[0].m_name, "alive");
    }

    TEST_F(ReadyGateFixture, NoRequirements_AlwaysHold)
    {
        EXPECT_TRUE(AllRequirementsHold({}, {}));
    }

    TEST_F(ReadyGateFixture, EveryPlainRequirementMustBeTrue)
    {
        const AZStd::vector<Requirement> requirements = { { "a", false }, { "b", false } };

        const AZStd::vector<AZStd::optional<bool>> allTrue = { true, true };
        EXPECT_TRUE(AllRequirementsHold(requirements, allTrue));

        const AZStd::vector<AZStd::optional<bool>> oneFalse = { true, false };
        EXPECT_FALSE(AllRequirementsHold(requirements, oneFalse));
    }

    TEST_F(ReadyGateFixture, ANegatedRequirementMustBeFalse)
    {
        const AZStd::vector<Requirement> requirements = { { "stunned", true } };

        const AZStd::vector<AZStd::optional<bool>> clear = { false };
        EXPECT_TRUE(AllRequirementsHold(requirements, clear));

        const AZStd::vector<AZStd::optional<bool>> stunned = { true };
        EXPECT_FALSE(AllRequirementsHold(requirements, stunned));
    }

    //! A variable that does not exist yet is not evidence of anything, so it never lets the gate open, negated or not.
    TEST_F(ReadyGateFixture, AMissingVariable_NeverHolds)
    {
        const AZStd::vector<Requirement> plain = { { "a", false } };
        const AZStd::vector<Requirement> negated = { { "a", true } };
        const AZStd::vector<AZStd::optional<bool>> missing = { AZStd::nullopt };

        EXPECT_FALSE(AllRequirementsHold(plain, missing));
        EXPECT_FALSE(AllRequirementsHold(negated, missing));
    }
} // namespace GOAT_Animation

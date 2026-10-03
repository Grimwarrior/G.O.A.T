#include <Sensing/PerceptionKeys.h>

#include <AzCore/Name/NameDictionary.h>
#include <AzCore/std/any.h>

namespace GOAT_Perception
{
    namespace
    {
        //! Declares one variable, treating an already declared name as success since a level reload declares it again.
        GOAT::BlackboardKey DeclareOne(
            GOAT::IBlackboardSystem& blackboard, const AZ::Name& name, GOAT::BlackboardType type, AZStd::any defaultValue)
        {
            auto declared = blackboard.Declare(name, GOAT::BlackboardScope::Agent, type, AZStd::move(defaultValue));
            if (declared.IsSuccess())
            {
                return declared.GetValue();
            }

            const GOAT::BlackboardKey existing = blackboard.FindKey(name);
            AZ_Error("GOAT", existing.IsValid(), "Perception variable '%s' could not be declared: %s",
                name.GetCStr(), declared.GetError().c_str());
            return existing;
        }
    } // namespace

    bool PerceptionKeys::Declare(GOAT::IBlackboardSystem& blackboard)
    {
        m_state = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_state"), GOAT::BlackboardType::Int, AZStd::any(AZ::s64(0)));
        m_suspicious = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_suspicious"), GOAT::BlackboardType::Bool, AZStd::any(false));
        m_searching = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_searching"), GOAT::BlackboardType::Bool, AZStd::any(false));
        m_engaged = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_engaged"), GOAT::BlackboardType::Bool, AZStd::any(false));
        m_target = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_target"), GOAT::BlackboardType::EntityId, AZStd::any(AZ::EntityId{}));
        m_targetVisible = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_target_visible"), GOAT::BlackboardType::Bool, AZStd::any(false));
        m_lastKnown = DeclareOne(
            blackboard, AZ_NAME_LITERAL("perc_last_known"), GOAT::BlackboardType::Vector3, AZStd::any(AZ::Vector3::CreateZero()));
        m_heard = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_heard"), GOAT::BlackboardType::Bool, AZStd::any(false));
        m_noisePosition = DeclareOne(
            blackboard, AZ_NAME_LITERAL("perc_noise_pos"), GOAT::BlackboardType::Vector3, AZStd::any(AZ::Vector3::CreateZero()));
        m_awareness = DeclareOne(blackboard, AZ_NAME_LITERAL("perc_awareness"), GOAT::BlackboardType::Float, AZStd::any(0.0f));

        AZ_Assert(IsValid(), "Every perception blackboard variable must resolve to a key");
        return IsValid();
    }

    bool PerceptionKeys::IsValid() const
    {
        return m_state.IsValid() && m_suspicious.IsValid() && m_searching.IsValid() && m_engaged.IsValid() && m_target.IsValid() && m_targetVisible.IsValid() && m_lastKnown.IsValid() && m_heard.IsValid() &&
            m_noisePosition.IsValid() && m_awareness.IsValid();
    }
} // namespace GOAT_Perception

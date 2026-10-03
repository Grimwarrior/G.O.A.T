#include <Signals/ReadyGate.h>

#include <AzCore/Debug/Trace.h>

namespace GOAT_Animation
{
    namespace
    {
        constexpr const char* Blanks = " \t";

        AZStd::string Trimmed(const AZStd::string& text)
        {
            const size_t first = text.find_first_not_of(Blanks);
            if (first == AZStd::string::npos)
            {
                return AZStd::string();
            }
            const size_t last = text.find_last_not_of(Blanks);
            return text.substr(first, last - first + 1);
        }
    } // namespace

    AZStd::vector<Requirement> ParseRequirements(AZStd::span<const AZStd::string> texts)
    {
        AZStd::vector<Requirement> requirements;
        requirements.reserve(texts.size());

        for (const AZStd::string& text : texts)
        {
            AZStd::string name = Trimmed(text);
            const bool negated = !name.empty() && name[0] == '!';
            if (negated)
            {
                name = Trimmed(name.substr(1));
            }

            if (!name.empty())
            {
                Requirement requirement;
                requirement.m_name = AZStd::move(name);
                requirement.m_negated = negated;
                requirements.push_back(AZStd::move(requirement));
            }
        }
        return requirements;
    }

    bool AllRequirementsHold(AZStd::span<const Requirement> requirements, AZStd::span<const AZStd::optional<bool>> values)
    {
        AZ_Assert(requirements.size() == values.size(), "Every requirement needs exactly one value to be judged by");
        if (requirements.size() != values.size())
        {
            return false;
        }

        for (size_t i = 0; i < requirements.size(); ++i)
        {
            if (!values[i].has_value() || *values[i] == requirements[i].m_negated)
            {
                return false;
            }
        }
        return true;
    }
} // namespace GOAT_Animation

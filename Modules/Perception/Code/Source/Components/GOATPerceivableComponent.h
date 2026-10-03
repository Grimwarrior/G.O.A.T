#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace GOAT_Perception
{
    //! Makes this entity something GOAT agents can notice, such as a player or a decoy.
    class GOATPerceivableComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT_DECL(GOATPerceivableComponent);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        void Activate() override;
        void Deactivate() override;

    private:
        //! Labels a profile's target tags are matched against, as in "player".
        AZStd::vector<AZStd::string> m_tags;

        //! Where sensors aim, relative to this entity's origin. Chest height by default.
        AZ::Vector3 m_aimOffset = AZ::Vector3(0.0f, 0.0f, 1.0f);
    };
} // namespace GOAT_Perception

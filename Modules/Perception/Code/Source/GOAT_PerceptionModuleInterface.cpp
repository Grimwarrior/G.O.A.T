
#include "GOAT_PerceptionModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>

#include <Clients/GOAT_PerceptionSystemComponent.h>
#include <Components/GOATPerceptionComponent.h>
#include <Components/GOATPerceivableComponent.h>

namespace GOAT_Perception
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(GOAT_PerceptionModuleInterface,
        "GOAT_PerceptionModuleInterface", GOAT_PerceptionModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(GOAT_PerceptionModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(GOAT_PerceptionModuleInterface, AZ::SystemAllocator);

    GOAT_PerceptionModuleInterface::GOAT_PerceptionModuleInterface()
    {
        // Every component descriptor of this gem goes here, which is also what reflects it.
        m_descriptors.insert(m_descriptors.end(), {
            GOAT_PerceptionSystemComponent::CreateDescriptor(),
            GOATPerceptionComponent::CreateDescriptor(),
            GOATPerceivableComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList GOAT_PerceptionModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<GOAT_PerceptionSystemComponent>(),
        };
    }
} // namespace GOAT_Perception

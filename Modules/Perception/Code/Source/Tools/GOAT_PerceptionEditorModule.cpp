
#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>
#include <GOAT_PerceptionModuleInterface.h>
#include "GOAT_PerceptionEditorSystemComponent.h"

#include <Tools/GOAT_PerceptionBuilderComponent.h>

namespace GOAT_Perception
{
    class GOAT_PerceptionEditorModule
        : public GOAT_PerceptionModuleInterface
    {
    public:
        AZ_RTTI(GOAT_PerceptionEditorModule, GOAT_PerceptionEditorModuleTypeId, GOAT_PerceptionModuleInterface);
        AZ_CLASS_ALLOCATOR(GOAT_PerceptionEditorModule, AZ::SystemAllocator);

        GOAT_PerceptionEditorModule()
        {
            // Every editor-only component descriptor of this gem goes here, which is also what reflects it.
            m_descriptors.insert(m_descriptors.end(), {
                GOAT_PerceptionEditorSystemComponent::CreateDescriptor(),
                GOAT_PerceptionBuilderComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<GOAT_PerceptionEditorSystemComponent>(),
            };
        }
    };
}// namespace GOAT_Perception

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), GOAT_Perception::GOAT_PerceptionEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_GOAT_Perception_Editor, GOAT_Perception::GOAT_PerceptionEditorModule)
#endif

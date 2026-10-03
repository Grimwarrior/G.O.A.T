
#include <GOAT_Perception/GOAT_PerceptionTypeIds.h>
#include <GOAT_PerceptionModuleInterface.h>
#include "GOAT_PerceptionSystemComponent.h"

namespace GOAT_Perception
{
    class GOAT_PerceptionModule
        : public GOAT_PerceptionModuleInterface
    {
    public:
        AZ_RTTI(GOAT_PerceptionModule, GOAT_PerceptionModuleTypeId, GOAT_PerceptionModuleInterface);
        AZ_CLASS_ALLOCATOR(GOAT_PerceptionModule, AZ::SystemAllocator);
    };
}// namespace GOAT_Perception

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), GOAT_Perception::GOAT_PerceptionModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_GOAT_Perception, GOAT_Perception::GOAT_PerceptionModule)
#endif

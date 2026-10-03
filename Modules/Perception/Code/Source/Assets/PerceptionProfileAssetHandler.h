#pragma once

#include <GOAT_Perception/PerceptionProfileAsset.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace GOAT_Perception
{
    //! Handles .prx assets, adding the asset browser icon; everything else is the generic behaviour, including building into the cache.
    class PerceptionProfileAssetHandler final
        : public AzFramework::GenericAssetHandler<PerceptionProfileAsset>
    {
    public:
        AZ_CLASS_ALLOCATOR(PerceptionProfileAssetHandler, AZ::SystemAllocator);

        PerceptionProfileAssetHandler();

        //! Path of the icon shown for this asset type, relative to the asset cache.
        const char* GetBrowserIcon() const override;
    };
} // namespace GOAT_Perception

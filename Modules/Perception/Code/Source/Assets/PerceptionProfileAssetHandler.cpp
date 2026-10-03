#include <Assets/PerceptionProfileAssetHandler.h>

#include <AzCore/Debug/Trace.h>

namespace GOAT_Perception
{
    PerceptionProfileAssetHandler::PerceptionProfileAssetHandler()
        : AzFramework::GenericAssetHandler<PerceptionProfileAsset>(
              PerceptionProfileAsset::DisplayName, PerceptionProfileAsset::AssetGroup, PerceptionProfileAsset::FileExtension)
    {
        // Lets the Asset Processor build the source into the cache without a custom builder.
        SetAutoBuildAssetToCache(true);

        AZ_Assert(PerceptionProfileAsset::FileExtension != nullptr && PerceptionProfileAsset::FileExtension[0] != '\0',
            "A generic asset handler needs a file extension to claim");
    }

    const char* PerceptionProfileAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/GOAT/AssetBrowser/Perception.svg";
    }
} // namespace GOAT_Perception

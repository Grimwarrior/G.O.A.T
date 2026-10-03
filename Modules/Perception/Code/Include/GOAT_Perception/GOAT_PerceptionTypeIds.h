
#pragma once

namespace GOAT_Perception
{
    // System Component TypeIds
    inline constexpr const char* GOAT_PerceptionSystemComponentTypeId = "{016AE930-F8A1-4294-BA39-6B928B05B201}";
    inline constexpr const char* GOAT_PerceptionEditorSystemComponentTypeId = "{4E362285-F1AB-4668-925A-DF0C873927E7}";
    inline constexpr const char* GOAT_PerceptionBuilderComponentTypeId = "{488E2D59-0C24-4F26-B4D2-67CD95BF60B2}";

    // Module derived classes TypeIds
    inline constexpr const char* GOAT_PerceptionModuleInterfaceTypeId = "{A4C6571A-5A9E-40E0-9ADD-5C6A3F5C64B8}";
    inline constexpr const char* GOAT_PerceptionModuleTypeId = "{6ED0B75B-417B-4305-AEE3-0221266142A2}";
    // The Editor Module is mutually exclusive with the Client Module, so they share a TypeId
    inline constexpr const char* GOAT_PerceptionEditorModuleTypeId = GOAT_PerceptionModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* GOAT_PerceptionRequestsTypeId = "{864755C4-9AC1-4445-9F17-E38DD5805646}";

    // Component TypeIds
    inline constexpr const char* GOATPerceptionComponentTypeId = "{BEFF7738-6430-4116-9B21-B89EEDFE0A33}";
    inline constexpr const char* GOATPerceivableComponentTypeId = "{CE447E4E-C923-430B-B4B0-F704BFF81067}";

    // Asset TypeIds
    inline constexpr const char* SightConeTypeId = "{B7DD5A28-F484-42CA-AD43-73C94140C062}";
    inline constexpr const char* PerceptionProfileAssetTypeId = "{15762DFE-936F-4114-A271-D61E13D71143}";
} // namespace GOAT_Perception


#pragma once

namespace GOAT_Animation
{
    // System Component TypeIds
    inline constexpr const char* GOAT_AnimationSystemComponentTypeId = "{EECBC8F5-0861-43F7-A3D7-CF412190F542}";
    inline constexpr const char* GOAT_AnimationEditorSystemComponentTypeId = "{45BAD222-7A3F-45AC-AC00-0C43A90E85B2}";

    // Module derived classes TypeIds
    inline constexpr const char* GOAT_AnimationModuleInterfaceTypeId = "{238B58FE-AEDD-4379-A51C-0623D1162B15}";
    inline constexpr const char* GOAT_AnimationModuleTypeId = "{9549D466-9825-49A9-B7DF-C017537BC46C}";
    // The Editor Module is mutually exclusive with the Client Module, so they share a TypeId
    inline constexpr const char* GOAT_AnimationEditorModuleTypeId = GOAT_AnimationModuleTypeId;

    // Interface TypeIds

    // Component TypeIds
    inline constexpr const char* GOATAnimationSignalsComponentTypeId = "{174AA354-B0F8-4E05-9618-96F7E919DDC6}";

    // Data TypeIds
    inline constexpr const char* SignalBindingTypeId = "{66E81E71-8708-4AB4-B3FA-E99C1C65D0D5}";
} // namespace GOAT_Animation

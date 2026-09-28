#pragma once

#include <string_view>

namespace ascii
{

// ==================================================
// Large-scale landform
//
// This describes the dominant topographic character
// of the generated embark/world region.
//
// It is deliberately broad:
//
//     MountainRange
//
// does not mean every tile is mountainous.
// It means the large-scale terrain-generation model
// used for this world is mountain-dominated.
// ==================================================

enum class LandformType
{
    Unknown = 0,

    Plains,
    RollingHills,
    Highlands,
    MountainRange,
    RiverValley,
    Plateau
};

// ==================================================
// Landform display name
// ==================================================

[[nodiscard]]
constexpr std::string_view landformName(
    LandformType type
)
{
    switch (type)
    {
        case LandformType::Plains:
            return "Plains";

        case LandformType::RollingHills:
            return "Rolling Hills";

        case LandformType::Highlands:
            return "Highlands";

        case LandformType::MountainRange:
            return "Mountain Range";

        case LandformType::RiverValley:
            return "River Valley";

        case LandformType::Plateau:
            return "Plateau";

        case LandformType::Unknown:
        default:
            return "Unknown";
    }
}

// ==================================================
// Safe deserialization
//
// Never blindly trust integer values loaded from JSON.
// If an old/corrupt save contains something outside the
// known enum range, return Unknown.
// ==================================================

[[nodiscard]]
constexpr LandformType landformFromInt(
    int value
)
{
    switch (
        static_cast<LandformType>(
            value
        )
    )
    {
        case LandformType::Plains:
            return LandformType::Plains;

        case LandformType::RollingHills:
            return LandformType::RollingHills;

        case LandformType::Highlands:
            return LandformType::Highlands;

        case LandformType::MountainRange:
            return LandformType::MountainRange;

        case LandformType::RiverValley:
            return LandformType::RiverValley;

        case LandformType::Plateau:
            return LandformType::Plateau;

        case LandformType::Unknown:
        default:
            return LandformType::Unknown;
    }
}

// ==================================================
// Climate
//
// This is the world's broad climate classification.
//
// Detailed environmental values such as moisture and
// temperature are generated as continuous fields.
// This enum describes the overall regional climate.
// ==================================================

enum class ClimateType
{
    Unknown = 0,

    Temperate,
    WetTemperate,
    Boreal,
    Dry
};

// ==================================================
// Climate display name
// ==================================================

[[nodiscard]]
constexpr std::string_view climateName(
    ClimateType type
)
{
    switch (type)
    {
        case ClimateType::Temperate:
            return "Temperate";

        case ClimateType::WetTemperate:
            return "Wet Temperate";

        case ClimateType::Boreal:
            return "Boreal";

        case ClimateType::Dry:
            return "Dry";

        case ClimateType::Unknown:
        default:
            return "Unknown";
    }
}

// ==================================================
// Safe deserialization
// ==================================================

[[nodiscard]]
constexpr ClimateType climateFromInt(
    int value
)
{
    switch (
        static_cast<ClimateType>(
            value
        )
    )
    {
        case ClimateType::Temperate:
            return ClimateType::Temperate;

        case ClimateType::WetTemperate:
            return ClimateType::WetTemperate;

        case ClimateType::Boreal:
            return ClimateType::Boreal;

        case ClimateType::Dry:
            return ClimateType::Dry;

        case ClimateType::Unknown:
        default:
            return ClimateType::Unknown;
    }
}

}

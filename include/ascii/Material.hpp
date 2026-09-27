#pragma once

#include "Color.hpp"

#include <string_view>

namespace ascii
{

enum class MaterialType
{
    None,

    // Surface / soil
    Grass,
    Soil,
    Clay,
    Sand,

    // Sedimentary
    Limestone,
    Sandstone,

    // Igneous / metamorphic
    Granite,
    Basalt,
    Marble,
    Obsidian,

    // Fuel / minerals / ores
    Coal,
    IronOre,
    CopperOre,
    TinOre,
    SilverOre,
    GoldOre,
    Quartz,

    // Wood
    OakWood,
    PineWood
};

[[nodiscard]]
constexpr std::string_view materialName(
    MaterialType material
)
{
    switch (material)
    {
        case MaterialType::Grass:
            return "Grass";

        case MaterialType::Soil:
            return "Soil";

        case MaterialType::Clay:
            return "Clay";

        case MaterialType::Sand:
            return "Sand";

        case MaterialType::Limestone:
            return "Limestone";

        case MaterialType::Sandstone:
            return "Sandstone";

        case MaterialType::Granite:
            return "Granite";

        case MaterialType::Basalt:
            return "Basalt";

        case MaterialType::Marble:
            return "Marble";

        case MaterialType::Obsidian:
            return "Obsidian";

        case MaterialType::Coal:
            return "Coal";

        case MaterialType::IronOre:
            return "Iron Ore";

        case MaterialType::CopperOre:
            return "Copper Ore";

        case MaterialType::TinOre:
            return "Tin Ore";

        case MaterialType::SilverOre:
            return "Silver Ore";

        case MaterialType::GoldOre:
            return "Gold Ore";

        case MaterialType::Quartz:
            return "Quartz";

        case MaterialType::OakWood:
            return "Oak Wood";

        case MaterialType::PineWood:
            return "Pine Wood";

        case MaterialType::None:
        default:
            return "None";
    }
}

[[nodiscard]]
constexpr TerminalColor materialColor(
    MaterialType material
)
{
    switch (material)
    {
        case MaterialType::Grass:
            return TerminalColor::BrightGreen;

        case MaterialType::Soil:
            return TerminalColor::Yellow;

        case MaterialType::Clay:
            return TerminalColor::Red;

        case MaterialType::Sand:
            return TerminalColor::BrightYellow;

        case MaterialType::Limestone:
            return TerminalColor::White;

        case MaterialType::Sandstone:
            return TerminalColor::Yellow;

        case MaterialType::Granite:
            return TerminalColor::BrightBlack;

        case MaterialType::Basalt:
            return TerminalColor::BrightBlack;

        case MaterialType::Marble:
            return TerminalColor::BrightWhite;

        case MaterialType::Obsidian:
            return TerminalColor::Magenta;

        case MaterialType::Coal:
            return TerminalColor::BrightBlack;

        case MaterialType::IronOre:
            return TerminalColor::BrightRed;

        case MaterialType::CopperOre:
            return TerminalColor::BrightYellow;

        case MaterialType::TinOre:
            return TerminalColor::BrightCyan;

        case MaterialType::SilverOre:
            return TerminalColor::BrightWhite;

        case MaterialType::GoldOre:
            return TerminalColor::BrightYellow;

        case MaterialType::Quartz:
            return TerminalColor::BrightMagenta;

        case MaterialType::OakWood:
            return TerminalColor::Yellow;

        case MaterialType::PineWood:
            return TerminalColor::Green;

        case MaterialType::None:
        default:
            return TerminalColor::Default;
    }
}

[[nodiscard]]
constexpr bool isSoilMaterial(
    MaterialType material
)
{
    return
        material == MaterialType::Soil
        ||
        material == MaterialType::Clay
        ||
        material == MaterialType::Sand;
}

[[nodiscard]]
constexpr bool isOreMaterial(
    MaterialType material
)
{
    return
        material == MaterialType::Coal
        ||
        material == MaterialType::IronOre
        ||
        material == MaterialType::CopperOre
        ||
        material == MaterialType::TinOre
        ||
        material == MaterialType::SilverOre
        ||
        material == MaterialType::GoldOre
        ||
        material == MaterialType::Quartz;
}

[[nodiscard]]
constexpr bool isWoodMaterial(
    MaterialType material
)
{
    return
        material == MaterialType::OakWood
        ||
        material == MaterialType::PineWood;
}

[[nodiscard]]
constexpr bool isStoneMaterial(
    MaterialType material
)
{
    return
        material == MaterialType::Limestone
        ||
        material == MaterialType::Sandstone
        ||
        material == MaterialType::Granite
        ||
        material == MaterialType::Basalt
        ||
        material == MaterialType::Marble
        ||
        material == MaterialType::Obsidian;
}

}

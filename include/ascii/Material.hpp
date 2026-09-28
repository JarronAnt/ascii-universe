#pragma once

#include "Color.hpp"

#include <string_view>

namespace ascii
{

enum class MaterialType
{
    None,

    // Existing values.
    //
    // IMPORTANT:
    // Keep these in their original order because the
    // current version-2 save format serializes
    // MaterialType numerically.
    Grass,
    Soil,
    Clay,
    Sand,

    Limestone,
    Sandstone,

    Granite,
    Basalt,
    Marble,
    Obsidian,

    Coal,
    IronOre,
    CopperOre,
    TinOre,
    SilverOre,
    GoldOre,
    Quartz,

    OakWood,
    PineWood,

    // ==================================================
    // Phase 5.5 additions
    //
    // These are deliberately APPENDED rather than
    // inserted above so old version-2 save material
    // IDs remain valid.
    // ==================================================

    // Additional unconsolidated sediments / soils.
    Silt,
    Loam,
    Peat,

    // Sedimentary rocks.
    Shale,
    Dolomite,

    // Igneous rocks.
    Diorite,
    Gabbro,

    // Metamorphic rocks.
    Slate,
    Schist,
    Gneiss
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

        case MaterialType::Silt:
            return "Silt";

        case MaterialType::Loam:
            return "Loam";

        case MaterialType::Peat:
            return "Peat";

        case MaterialType::Shale:
            return "Shale";

        case MaterialType::Dolomite:
            return "Dolomite";

        case MaterialType::Diorite:
            return "Diorite";

        case MaterialType::Gabbro:
            return "Gabbro";

        case MaterialType::Slate:
            return "Slate";

        case MaterialType::Schist:
            return "Schist";

        case MaterialType::Gneiss:
            return "Gneiss";

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
        // ==================================================
        // Surface / soil
        // ==================================================

        case MaterialType::Grass:
            return TerminalColor::BrightGreen;

        case MaterialType::Soil:
            return TerminalColor::Yellow;

        case MaterialType::Clay:
            return TerminalColor::Red;

        case MaterialType::Sand:
            return TerminalColor::BrightYellow;

        case MaterialType::Silt:
            return TerminalColor::Yellow;

        case MaterialType::Loam:
            return TerminalColor::BrightYellow;

        case MaterialType::Peat:
            return TerminalColor::BrightBlack;

        // ==================================================
        // Sedimentary
        // ==================================================

        case MaterialType::Limestone:
            return TerminalColor::White;

        case MaterialType::Sandstone:
            return TerminalColor::Yellow;

        case MaterialType::Shale:
            return TerminalColor::BrightBlack;

        case MaterialType::Dolomite:
            return TerminalColor::BrightWhite;

        // ==================================================
        // Igneous
        // ==================================================

        case MaterialType::Granite:
            return TerminalColor::BrightBlack;

        case MaterialType::Basalt:
            return TerminalColor::BrightBlack;

        case MaterialType::Obsidian:
            return TerminalColor::Magenta;

        case MaterialType::Diorite:
            return TerminalColor::White;

        case MaterialType::Gabbro:
            return TerminalColor::BrightBlack;

        // ==================================================
        // Metamorphic
        // ==================================================

        case MaterialType::Marble:
            return TerminalColor::BrightWhite;

        case MaterialType::Slate:
            return TerminalColor::Blue;

        case MaterialType::Schist:
            return TerminalColor::Magenta;

        case MaterialType::Gneiss:
            return TerminalColor::BrightWhite;

        // ==================================================
        // Ores / minerals
        // ==================================================

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

        // ==================================================
        // Wood
        // ==================================================

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
        material == MaterialType::Sand
        ||
        material == MaterialType::Silt
        ||
        material == MaterialType::Loam
        ||
        material == MaterialType::Peat;
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
        material == MaterialType::Obsidian
        ||
        material == MaterialType::Shale
        ||
        material == MaterialType::Dolomite
        ||
        material == MaterialType::Diorite
        ||
        material == MaterialType::Gabbro
        ||
        material == MaterialType::Slate
        ||
        material == MaterialType::Schist
        ||
        material == MaterialType::Gneiss;
}

}

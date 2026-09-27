#pragma once

#include "Material.hpp"

#include <cstdint>

namespace ascii
{

enum class TileShape
{
    Open,

    Floor,
    Wall,

    Ramp,

    UpStair,
    DownStair,
    UpDownStair
};

enum class TileFeature
{
    None,
    Tree
};

enum class LiquidType
{
    None,
    Water
};

struct Liquid
{
    LiquidType type{
        LiquidType::None
    };

    // 0 = dry
    // 7 = full/deep
    std::uint8_t depth{0};
};

struct Tile
{
    TileShape shape{
        TileShape::Open
    };

    // Geological material making up this tile.
    MaterialType material{
        MaterialType::None
    };

    // Surface feature such as a tree.
    TileFeature feature{
        TileFeature::None
    };

    MaterialType featureMaterial{
        MaterialType::None
    };

    Liquid liquid{};

    [[nodiscard]]
    bool walkable() const
    {
        const bool passableShape =
            shape == TileShape::Floor
            ||
            shape == TileShape::Ramp
            ||
            shape == TileShape::UpStair
            ||
            shape == TileShape::DownStair
            ||
            shape == TileShape::UpDownStair;

        return
            passableShape
            &&
            feature != TileFeature::Tree;
    }

    [[nodiscard]]
    bool solid() const
    {
        return
            shape == TileShape::Wall;
    }

    [[nodiscard]]
    bool hasUpConnection() const
    {
        return
            shape == TileShape::UpStair
            ||
            shape == TileShape::UpDownStair;
    }

    [[nodiscard]]
    bool hasDownConnection() const
    {
        return
            shape == TileShape::DownStair
            ||
            shape == TileShape::UpDownStair;
    }

    [[nodiscard]]
    bool canHoldLiquid() const
    {
        return
            shape != TileShape::Wall
            &&
            feature != TileFeature::Tree;
    }

    [[nodiscard]]
    bool supportsHorizontalLiquid() const
    {
        return walkable();
    }
};

}

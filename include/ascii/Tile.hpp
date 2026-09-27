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

    // Dwarf Fortress-style liquid depth.
    //
    // 0 = dry
    // 1-3 = shallow / walkable
    // 4-7 = deep / currently blocks goblin movement
    std::uint8_t depth{0};
};

struct Tile
{
    // ==================================================
    // Geometry
    // ==================================================

    TileShape shape{
        TileShape::Open
    };

    // ==================================================
    // Geology
    // ==================================================

    MaterialType material{
        MaterialType::None
    };

    // ==================================================
    // Surface features
    // ==================================================

    TileFeature feature{
        TileFeature::None
    };

    MaterialType featureMaterial{
        MaterialType::None
    };

    // ==================================================
    // Liquids
    // ==================================================

    Liquid liquid{};

    // ==================================================
    // Geometry helpers
    // ==================================================

    [[nodiscard]]
    bool baseWalkable() const
    {
        return
            shape == TileShape::Floor
            ||
            shape == TileShape::Ramp
            ||
            shape == TileShape::UpStair
            ||
            shape == TileShape::DownStair
            ||
            shape == TileShape::UpDownStair;
    }

    [[nodiscard]]
    bool walkable() const
    {
        // Trees physically occupy the tile.
        if (
            feature ==
            TileFeature::Tree
        )
        {
            return false;
        }

        // Deep water currently blocks normal
        // goblin movement.
        //
        // Later this can be replaced with:
        //
        // CanSwim
        // swimming skill
        // creature size
        // flow strength
        // drowning
        //
        if (
            liquid.type ==
                LiquidType::Water
            &&
            liquid.depth > 3
        )
        {
            return false;
        }

        return baseWalkable();
    }

    [[nodiscard]]
    bool solid() const
    {
        return
            shape ==
            TileShape::Wall;
    }

    // ==================================================
    // Vertical navigation
    // ==================================================

    [[nodiscard]]
    bool hasUpConnection() const
    {
        return
            shape ==
                TileShape::UpStair
            ||
            shape ==
                TileShape::UpDownStair;
    }

    [[nodiscard]]
    bool hasDownConnection() const
    {
        return
            shape ==
                TileShape::DownStair
            ||
            shape ==
                TileShape::UpDownStair;
    }

    // ==================================================
    // Liquid helpers
    // ==================================================

    [[nodiscard]]
    bool canHoldLiquid() const
    {
        // A solid wall cannot contain water.
        if (
            shape ==
            TileShape::Wall
        )
        {
            return false;
        }

        // For now trees prevent the tile itself from
        // accepting water.
        //
        // We can make trees coexist with shallow water
        // later when vegetation gets more sophisticated.
        if (
            feature ==
            TileFeature::Tree
        )
        {
            return false;
        }

        return true;
    }

    [[nodiscard]]
    bool supportsHorizontalLiquid() const
    {
        // IMPORTANT:
        //
        // Do NOT call walkable() here.
        //
        // A tile with water depth 7 is not walkable
        // by a normal goblin, but water still needs
        // to be able to flow horizontally through it.
        return
            baseWalkable()
            &&
            feature !=
                TileFeature::Tree;
    }
};

}

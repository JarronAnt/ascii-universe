#pragma once

#include "Material.hpp"

#include <entt/entt.hpp>

#include <cstdint>
#include <optional>

namespace ascii
{

enum class ItemType
{
    Stone,
    Ore,
    Soil,
    Log
};

struct Item
{
    ItemType type{
        ItemType::Stone
    };

    MaterialType material{
        MaterialType::Granite
    };

    std::uint32_t weight{1};
};

struct Carriable
{
};

enum class ItemLocation
{
    OnGround,
    Carried,
    Stockpiled
};

struct ItemState
{
    ItemLocation location{
        ItemLocation::OnGround
    };

    entt::entity carrier{
        entt::null
    };

    entt::entity stockpile{
        entt::null
    };
};

struct CarryingItem
{
    entt::entity item{
        entt::null
    };
};

[[nodiscard]]
inline std::optional<ItemType>
itemTypeForMaterial(
    MaterialType material
)
{
    if (
        isSoilMaterial(material)
    )
    {
        return ItemType::Soil;
    }

    if (
        isOreMaterial(material)
    )
    {
        return ItemType::Ore;
    }

    if (
        isStoneMaterial(material)
    )
    {
        return ItemType::Stone;
    }

    if (
        isWoodMaterial(material)
    )
    {
        return ItemType::Log;
    }

    return std::nullopt;
}

}

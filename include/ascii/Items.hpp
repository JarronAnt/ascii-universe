#pragma once

#include <entt/entt.hpp>

#include <cstdint>

namespace ascii
{

enum class ItemType
{
    Stone
};

struct Item
{
    ItemType type{
        ItemType::Stone
    };

    std::uint32_t weight{1};
};

// Marker component.
//
// Only entities with Carriable can generate
// hauling jobs.
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

// Added to a goblin while it is physically
// carrying an item.
struct CarryingItem
{
    entt::entity item{
        entt::null
    };
};

}

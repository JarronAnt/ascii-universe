#pragma once

#include "Items.hpp"
#include "Position.hpp"

#include <entt/entt.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace ascii
{

struct StockpileBounds
{
    Position min{};
    Position max{};

    [[nodiscard]]
    bool contains(
        Position position
    ) const
    {
        return
            position.x >= min.x
            &&
            position.y >= min.y
            &&
            position.z >= min.z
            &&
            position.x <= max.x
            &&
            position.y <= max.y
            &&
            position.z <= max.z;
    }
};

struct Stockpile
{
    StockpileBounds bounds{};

    std::vector<ItemType>
        accepts;

    std::vector<entt::entity>
        currentItems;

    std::vector<Position>
        reservedCells;

    std::optional<std::size_t>
        maxItems;

    [[nodiscard]]
    bool acceptsItem(
        ItemType type
    ) const
    {
        return
            std::find(
                accepts.begin(),
                accepts.end(),
                type
            )
            !=
            accepts.end();
    }

    [[nodiscard]]
    bool full() const
    {
        if (!maxItems)
        {
            return false;
        }

        return
            currentItems.size()
            +
            reservedCells.size()
            >=
            *maxItems;
    }
};

}

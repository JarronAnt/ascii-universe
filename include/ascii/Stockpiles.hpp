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
    Position topLeft{};
    Position bottomRight{};

    [[nodiscard]]
    bool contains(
        Position position
    ) const
    {
        return
            position.x >= topLeft.x &&
            position.y >= topLeft.y &&
            position.x <= bottomRight.x &&
            position.y <= bottomRight.y;
    }
};

struct Stockpile
{
    StockpileBounds bounds{};

    // Item filters.
    std::vector<ItemType> accepts;

    // Items currently stored here.
    std::vector<entt::entity>
        currentItems;

    // Cells promised to haul jobs but which
    // don't contain their item yet.
    std::vector<Position>
        reservedCells;

    // Empty optional means unlimited except for
    // available physical cells.
    std::optional<std::size_t>
        maxItems;

    [[nodiscard]]
    bool acceptsItem(
        ItemType type
    ) const
    {
        return std::find(
            accepts.begin(),
            accepts.end(),
            type
        ) != accepts.end();
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

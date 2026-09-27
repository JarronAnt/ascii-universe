#pragma once

#include "ascii/GameMap.hpp"
#include "ascii/Items.hpp"
#include "ascii/Pathfinder.hpp"
#include "ascii/Position.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <optional>

namespace ascii::systems
{

struct StockpileDestination
{
    entt::entity stockpile{
        entt::null
    };

    Position position{};

    std::size_t pathLength{};
};

[[nodiscard]]
std::optional<StockpileDestination>
findStockpileDestination(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    ItemType itemType,
    Position itemPosition
);

void reserveStockpileCell(
    entt::registry& registry,
    entt::entity stockpile,
    Position position
);

void releaseStockpileCell(
    entt::registry& registry,
    entt::entity stockpile,
    Position position
);

}

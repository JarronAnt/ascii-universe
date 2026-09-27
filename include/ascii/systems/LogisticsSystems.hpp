#pragma once

#include "ascii/Events.hpp"
#include "ascii/GameMap.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Pathfinder.hpp"

#include <entt/entt.hpp>

namespace ascii::systems
{

void generateHaulJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
);

void assignHaulJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
);

void executeHauling(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard,
    EventQueue<ItemPickupEvent>&
        itemPickupEvents,
    EventQueue<ItemDropEvent>&
        itemDropEvents
);

void processItemSpawns(
    entt::registry& registry,
    EventQueue<ItemSpawnEvent>&
        events
);

void processItemPickups(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard,
    EventQueue<ItemPickupEvent>&
        events
);

void processItemDrops(
    entt::registry& registry,
    JobBoard& jobBoard,
    EventQueue<ItemDropEvent>&
        events
);

}

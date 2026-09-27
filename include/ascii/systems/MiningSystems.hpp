#pragma once

#include "ascii/Events.hpp"
#include "ascii/GameMap.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Pathfinder.hpp"

#include <entt/entt.hpp>

namespace ascii::systems
{

void assignMiningJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
);

void executeMining(
    entt::registry& registry,
    GameMap& map,
    JobBoard& jobBoard,
    EventQueue<ItemSpawnEvent>&
        itemSpawnEvents
);

}

#pragma once

#include "ascii/GameMap.hpp"
#include "ascii/Jobs.hpp"

#include <entt/entt.hpp>

namespace ascii::systems
{

void deduplicateDesignations(
    entt::registry& registry
);

void createDesignationJobs(
    entt::registry& registry,
    GameMap& map,
    JobBoard& jobBoard
);

}

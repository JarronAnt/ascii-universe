#pragma once

#include "ascii/GameMap.hpp"

#include <entt/entt.hpp>

namespace ascii::systems
{

void updateMovement(
    entt::registry& registry,
    const GameMap& map
);

}

#include "ascii/systems/MovementSystem.hpp"

#include "ascii/Components.hpp"
#include "ascii/Position.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <vector>

namespace ascii::systems
{

void updateMovement(
    entt::registry& registry,
    const GameMap& map
)
{
    auto view =
        registry.view<
            Position,
            MovementPath
        >();

    std::vector<entt::entity>
        entities;

    for (
        auto entity :
        view
    )
    {
        entities.push_back(
            entity
        );
    }

    sortEntities(
        entities
    );

    for (
        auto entity :
        entities
    )
    {
        auto& position =
            registry.get<
                Position
            >(entity);

        auto& path =
            registry.get<
                MovementPath
            >(entity);

        if (
            path.finished()
        )
        {
            continue;
        }

        const Position next =
            path.nodes[
                path.nextStep
            ];

        if (
            !map.inBounds(next)
            ||
            !map.at(
                next
            ).walkable()
        )
        {
            path.nextStep =
                path.nodes.size();

            continue;
        }

        position =
            next;

        ++path.nextStep;
    }
}

}

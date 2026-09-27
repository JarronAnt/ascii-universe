#include "ascii/systems/DesignationSystems.hpp"

#include "ascii/Components.hpp"
#include "ascii/Designations.hpp"
#include "ascii/Position.hpp"
#include "ascii/Tile.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <set>
#include <utility>
#include <vector>

namespace ascii::systems
{

void deduplicateDesignations(
    entt::registry& registry
)
{
    auto view =
        registry.view<
            MineDesignation,
            Position,
            DesignationLifecycle
        >();

    std::vector<entt::entity>
        entities;

    for (auto entity : view)
    {
        const auto& lifecycle =
            view.get<
                DesignationLifecycle
            >(entity);

        if (
            lifecycle.state ==
            DesignationState::Active
        )
        {
            entities.push_back(
                entity
            );
        }
    }

    sortEntities(
        entities
    );

    std::set<
        std::pair<int, int>
    > seen;

    for (auto entity : entities)
    {
        const auto& position =
            registry.get<
                Position
            >(entity);

        const auto key =
            std::make_pair(
                position.x,
                position.y
            );

        const auto [
            iterator,
            inserted
        ] = seen.insert(key);

        (void)iterator;

        if (inserted)
        {
            continue;
        }

        auto& lifecycle =
            registry.get<
                DesignationLifecycle
            >(entity);

        lifecycle.state =
            DesignationState::Ignored;

        hideDesignation(
            registry,
            entity
        );
    }
}

void createDesignationJobs(
    entt::registry& registry,
    GameMap& map,
    JobBoard& jobBoard
)
{
    auto view =
        registry.view<
            MineDesignation,
            Position,
            DesignationLifecycle
        >();

    std::vector<entt::entity>
        entities;

    for (auto entity : view)
    {
        const auto& lifecycle =
            view.get<
                DesignationLifecycle
            >(entity);

        if (
            lifecycle.state ==
            DesignationState::Active
        )
        {
            entities.push_back(
                entity
            );
        }
    }

    sortEntities(
        entities
    );

    for (auto entity : entities)
    {
        auto& lifecycle =
            registry.get<
                DesignationLifecycle
            >(entity);

        const Position position =
            registry.get<
                Position
            >(entity);

        if (
            !map.inBounds(
                position.x,
                position.y
            )
            ||
            map.at(
                position.x,
                position.y
            ).type !=
                TileType::Wall
        )
        {
            lifecycle.state =
                DesignationState::Ignored;

            hideDesignation(
                registry,
                entity
            );

            continue;
        }

        jobBoard.add(
            JobType::Mine,
            position,
            entity
        );

        lifecycle.state =
            DesignationState::Consumed;
    }
}

}

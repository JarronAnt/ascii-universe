#include "ascii/systems/DesignationSystems.hpp"

#include "ascii/Designations.hpp"
#include "ascii/Position.hpp"
#include "ascii/Tile.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <set>
#include <tuple>
#include <vector>

namespace ascii::systems
{

namespace
{

bool validDesignation(
    const GameMap& map,
    DesignationType type,
    Position position
)
{
    if (
        !map.inBounds(
            position
        )
    )
    {
        return false;
    }

    const Tile& tile =
        map.at(
            position
        );

    switch (type)
    {
        case DesignationType::Mine:
            return
                tile.shape ==
                    TileShape::Wall;

        case DesignationType::DigDown:
        {
            if (
                !tile.walkable()
                ||
                position.z <= 0
            )
            {
                return false;
            }

            const Position below{
                position.x,
                position.y,
                position.z - 1
            };

            return
                map.at(
                    below
                ).shape ==
                    TileShape::Wall;
        }

        case DesignationType::DigUp:
        {
            if (
                !tile.walkable()
                ||
                position.z >=
                    map.depth() - 1
            )
            {
                return false;
            }

            const Position above{
                position.x,
                position.y,
                position.z + 1
            };

            return
                map.at(
                    above
                ).shape ==
                    TileShape::Wall;
        }

        case DesignationType::FellTree:
            return
                tile.feature ==
                    TileFeature::Tree;
    }

    return false;
}

JobType jobTypeForDesignation(
    DesignationType type
)
{
    switch (type)
    {
        case DesignationType::Mine:
            return JobType::Mine;

        case DesignationType::DigDown:
            return JobType::DigDown;

        case DesignationType::DigUp:
            return JobType::DigUp;

        case DesignationType::FellTree:
            return JobType::FellTree;
    }

    return JobType::Mine;
}

}

void deduplicateDesignations(
    entt::registry& registry
)
{
    auto view =
        registry.view<
            Designation,
            Position,
            DesignationLifecycle
        >();

    std::vector<entt::entity>
        entities;

    for (
        auto entity :
        view
    )
    {
        if (
            view.get<
                DesignationLifecycle
            >(entity).state ==
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
        std::tuple<
            int,
            int,
            int,
            int
        >
    > seen;

    for (
        auto entity :
        entities
    )
    {
        const auto& designation =
            registry.get<
                Designation
            >(entity);

        const auto& position =
            registry.get<
                Position
            >(entity);

        const auto key =
            std::make_tuple(
                static_cast<int>(
                    designation.type
                ),
                position.x,
                position.y,
                position.z
            );

        if (
            !seen.insert(
                key
            ).second
        )
        {
            registry.get<
                DesignationLifecycle
            >(entity).state =
                DesignationState::Ignored;

            hideDesignation(
                registry,
                entity
            );
        }
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
            Designation,
            Position,
            DesignationLifecycle
        >();

    std::vector<entt::entity>
        entities;

    for (
        auto entity :
        view
    )
    {
        if (
            view.get<
                DesignationLifecycle
            >(entity).state ==
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

    for (
        auto entity :
        entities
    )
    {
        auto& lifecycle =
            registry.get<
                DesignationLifecycle
            >(entity);

        const auto& designation =
            registry.get<
                Designation
            >(entity);

        const Position position =
            registry.get<
                Position
            >(entity);

        if (
            !validDesignation(
                map,
                designation.type,
                position
            )
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
            jobTypeForDesignation(
                designation.type
            ),
            position,
            entity
        );

        lifecycle.state =
            DesignationState::Consumed;
    }
}

}

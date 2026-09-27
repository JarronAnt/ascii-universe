#pragma once

#include "ascii/Components.hpp"
#include "ascii/GameMap.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Pathfinder.hpp"
#include "ascii/Position.hpp"

#include <entt/entt.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace ascii::systems
{

inline void sortEntities(
    std::vector<entt::entity>&
        entities
)
{
    std::sort(
        entities.begin(),
        entities.end(),
        [](
            entt::entity first,
            entt::entity second
        )
        {
            return
                entt::to_integral(
                    first
                )
                <
                entt::to_integral(
                    second
                );
        }
    );
}

inline bool isAdjacentSameLevel(
    Position first,
    Position second
)
{
    if (
        first.z !=
        second.z
    )
    {
        return false;
    }

    const int dx =
        std::abs(
            first.x -
            second.x
        );

    const int dy =
        std::abs(
            first.y -
            second.y
        );

    return
        dx + dy == 1;
}

struct WorkApproach
{
    Position workPosition{};

    std::vector<Position>
        path;
};

inline std::optional<WorkApproach>
findAdjacentApproach(
    const GameMap& map,
    const Pathfinder& pathfinder,
    Position worker,
    Position target
)
{
    constexpr std::array<
        Position,
        4
    > directions{
        Position{0, -1, 0},
        Position{1, 0, 0},
        Position{0, 1, 0},
        Position{-1, 0, 0}
    };

    std::optional<
        WorkApproach
    > best;

    for (
        const auto direction :
        directions
    )
    {
        const Position candidate{
            target.x +
                direction.x,

            target.y +
                direction.y,

            target.z
        };

        if (
            !map.inBounds(
                candidate
            )
            ||
            !map.at(
                candidate
            ).walkable()
        )
        {
            continue;
        }

        auto path =
            pathfinder.findPath(
                map,
                worker,
                candidate
            );

        if (!path)
        {
            continue;
        }

        if (
            !best
            ||
            path->size() <
                best->path.size()
        )
        {
            best =
                WorkApproach{
                    candidate,
                    std::move(
                        *path
                    )
                };
        }
    }

    return best;
}

inline void setMovementPath(
    entt::registry& registry,
    entt::entity worker,
    std::vector<Position> path
)
{
    if (
        registry.all_of<
            MovementPath
        >(worker)
    )
    {
        registry.remove<
            MovementPath
        >(worker);
    }

    auto& movement =
        registry.emplace<
            MovementPath
        >(worker);

    movement.nodes =
        std::move(path);

    movement.nextStep =
        0;
}

inline void clearWorkerJob(
    entt::registry& registry,
    entt::entity worker
)
{
    if (
        !registry.valid(
            worker
        )
    )
    {
        return;
    }

    if (
        registry.all_of<
            MovementPath
        >(worker)
    )
    {
        registry.remove<
            MovementPath
        >(worker);
    }

    if (
        registry.all_of<
            AssignedJob
        >(worker)
    )
    {
        registry.remove<
            AssignedJob
        >(worker);
    }
}

inline void hideDesignation(
    entt::registry& registry,
    entt::entity entity
)
{
    if (
        entity ==
            entt::null
        ||
        !registry.valid(
            entity
        )
    )
    {
        return;
    }

    if (
        registry.all_of<
            Glyph
        >(entity)
    )
    {
        registry.remove<
            Glyph
        >(entity);
    }
}

inline void releaseJob(
    Job& job
)
{
    job.state =
        JobState::Available;

    job.worker =
        entt::null;
}

}

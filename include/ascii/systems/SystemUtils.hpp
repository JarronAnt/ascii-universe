#pragma once

#include "ascii/Components.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Position.hpp"

#include <entt/entt.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace ascii::systems
{

inline void sortEntities(
    std::vector<entt::entity>& entities
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
                entt::to_integral(first)
                <
                entt::to_integral(second);
        }
    );
}

inline bool isAdjacent(
    Position first,
    Position second
)
{
    int dx =
        first.x - second.x;

    int dy =
        first.y - second.y;

    if (dx < 0)
    {
        dx = -dx;
    }

    if (dy < 0)
    {
        dy = -dy;
    }

    return
        (dx + dy) == 1;
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
        !registry.valid(worker)
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
        entity == entt::null
        ||
        !registry.valid(entity)
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

#include "ascii/systems/MiningSystems.hpp"

#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Position.hpp"
#include "ascii/Tile.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace ascii::systems
{

namespace
{

struct MiningApproach
{
    Position workPosition{};

    std::vector<Position>
        path;
};

std::optional<MiningApproach>
findMiningApproach(
    const GameMap& map,
    const Pathfinder& pathfinder,
    Position worker,
    Position wall
)
{
    constexpr std::array<
        Position,
        4
    > directions{
        Position{0, -1},
        Position{1, 0},
        Position{0, 1},
        Position{-1, 0}
    };

    std::optional<
        MiningApproach
    > best;

    for (
        const Position direction :
        directions
    )
    {
        const Position candidate{
            wall.x +
                direction.x,

            wall.y +
                direction.y
        };

        if (
            !map.inBounds(
                candidate.x,
                candidate.y
            )
        )
        {
            continue;
        }

        if (
            !map.at(
                candidate.x,
                candidate.y
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
            MiningApproach approach;

            approach.workPosition =
                candidate;

            approach.path =
                std::move(*path);

            best =
                std::move(approach);
        }
    }

    return best;
}

}

void assignMiningJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
)
{
    auto view =
        registry.view<
            Goblin,
            Miner,
            Position
        >();

    std::vector<entt::entity>
        miners;

    for (auto entity : view)
    {
        miners.push_back(
            entity
        );
    }

    sortEntities(
        miners
    );

    for (auto miner : miners)
    {
        if (
            registry.all_of<
                AssignedJob
            >(miner)
        )
        {
            continue;
        }

        const Position start =
            registry.get<
                Position
            >(miner);

        for (
            auto& job :
            jobBoard.jobs()
        )
        {
            if (
                job.state !=
                    JobState::Available
                ||
                job.type !=
                    JobType::Mine
            )
            {
                continue;
            }

            auto approach =
                findMiningApproach(
                    map,
                    pathfinder,
                    start,
                    job.target
                );

            if (!approach)
            {
                continue;
            }

            job.state =
                JobState::Assigned;

            job.worker =
                miner;

            job.workPosition =
                approach->
                    workPosition;

            auto& assigned =
                registry.emplace<
                    AssignedJob
                >(miner);

            assigned.id =
                job.id;

            setMovementPath(
                registry,
                miner,
                std::move(
                    approach->path
                )
            );

            break;
        }
    }
}

void executeMining(
    entt::registry& registry,
    GameMap& map,
    JobBoard& jobBoard,
    EventQueue<ItemSpawnEvent>&
        itemSpawnEvents
)
{
    auto view =
        registry.view<
            Goblin,
            Miner,
            Position,
            AssignedJob
        >();

    std::vector<entt::entity>
        workers;

    for (auto entity : view)
    {
        workers.push_back(
            entity
        );
    }

    sortEntities(
        workers
    );

    std::vector<entt::entity>
        workersToClear;

    for (auto worker : workers)
    {
        const auto& assigned =
            registry.get<
                AssignedJob
            >(worker);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (job == nullptr)
        {
            workersToClear.push_back(
                worker
            );

            continue;
        }

        // This worker might also be a Hauler.
        if (
            job->type !=
                JobType::Mine
        )
        {
            continue;
        }

        if (
            job->state !=
                JobState::Assigned
            ||
            job->worker !=
                worker
        )
        {
            workersToClear.push_back(
                worker
            );

            continue;
        }

        if (
            registry.all_of<
                MovementPath
            >(worker)
        )
        {
            const auto& movement =
                registry.get<
                    MovementPath
                >(worker);

            if (
                !movement.finished()
            )
            {
                continue;
            }
        }

        const Position workerPosition =
            registry.get<
                Position
            >(worker);

        if (
            workerPosition !=
                job->workPosition
        )
        {
            releaseJob(
                *job
            );

            workersToClear.push_back(
                worker
            );

            continue;
        }

        if (
            !isAdjacent(
                workerPosition,
                job->target
            )
        )
        {
            releaseJob(
                *job
            );

            workersToClear.push_back(
                worker
            );

            continue;
        }

        if (
            !map.inBounds(
                job->target.x,
                job->target.y
            )
        )
        {
            job->state =
                JobState::Cancelled;

            job->worker =
                entt::null;

            hideDesignation(
                registry,
                job->
                    sourceDesignation
            );

            workersToClear.push_back(
                worker
            );

            continue;
        }

        Tile& targetTile =
            map.at(
                job->target.x,
                job->target.y
            );

        if (
            targetTile.type !=
                TileType::Wall
        )
        {
            job->state =
                JobState::Complete;

            job->worker =
                entt::null;

            hideDesignation(
                registry,
                job->
                    sourceDesignation
            );

            workersToClear.push_back(
                worker
            );

            continue;
        }

        // ==========================================
        // Mining
        // ==========================================

        targetTile.type =
            TileType::Floor;

        itemSpawnEvents.emit(
            ItemSpawnEvent{
                ItemType::Stone,
                job->target,
                ItemSource::Mining
            }
        );

        job->state =
            JobState::Complete;

        job->worker =
            entt::null;

        hideDesignation(
            registry,
            job->
                sourceDesignation
        );

        workersToClear.push_back(
            worker
        );
    }

    for (
        auto worker :
        workersToClear
    )
    {
        clearWorkerJob(
            registry,
            worker
        );
    }
}

}

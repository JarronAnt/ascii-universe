#include "ascii/systems/FellingSystems.hpp"

#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Tile.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <utility>
#include <vector>

namespace ascii::systems
{

void assignFellingJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
)
{
    auto view =
        registry.view<
            Goblin,
            Woodcutter,
            Position
        >();

    std::vector<entt::entity>
        workers;

    for (
        auto entity :
        view
    )
    {
        workers.push_back(
            entity
        );
    }

    sortEntities(
        workers
    );

    for (
        auto worker :
        workers
    )
    {
        if (
            registry.all_of<
                AssignedJob
            >(worker)
        )
        {
            continue;
        }

        const Position start =
            registry.get<
                Position
            >(worker);

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
                    JobType::FellTree
            )
            {
                continue;
            }

            auto approach =
                findAdjacentApproach(
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
                worker;

            job.workPosition =
                approach->
                    workPosition;

            auto& assigned =
                registry.emplace<
                    AssignedJob
                >(worker);

            assigned.id =
                job.id;

            setMovementPath(
                registry,
                worker,
                std::move(
                    approach->path
                )
            );

            break;
        }
    }
}

void executeFelling(
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
            Woodcutter,
            Position,
            AssignedJob
        >();

    std::vector<entt::entity>
        workers;

    for (
        auto entity :
        view
    )
    {
        workers.push_back(
            entity
        );
    }

    sortEntities(
        workers
    );

    std::vector<entt::entity>
        clear;

    for (
        auto worker :
        workers
    )
    {
        const auto assigned =
            registry.get<
                AssignedJob
            >(worker);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (
            job == nullptr
            ||
            job->type !=
                JobType::FellTree
        )
        {
            continue;
        }

        if (
            registry.all_of<
                MovementPath
            >(worker)
            &&
            !registry.get<
                MovementPath
            >(worker).finished()
        )
        {
            continue;
        }

        const Position position =
            registry.get<
                Position
            >(worker);

        bool success =
            false;

        if (
            position ==
                job->workPosition
            &&
            isAdjacentSameLevel(
                position,
                job->target
            )
            &&
            map.inBounds(
                job->target
            )
        )
        {
            Tile& tile =
                map.at(
                    job->target
                );

            if (
                tile.feature ==
                    TileFeature::Tree
            )
            {
                const MaterialType wood =
                    tile.featureMaterial;

                tile.feature =
                    TileFeature::None;

                tile.featureMaterial =
                    MaterialType::None;

                itemSpawnEvents.emit(
                    ItemSpawnEvent{
                        ItemType::Log,
                        wood,
                        job->target,
                        ItemSource::Felling
                    }
                );

                success =
                    true;
            }
        }

        job->state =
            success
            ?
            JobState::Complete
            :
            JobState::Cancelled;

        job->worker =
            entt::null;

        hideDesignation(
            registry,
            job->
                sourceDesignation
        );

        clear.push_back(
            worker
        );
    }

    for (
        auto worker :
        clear
    )
    {
        clearWorkerJob(
            registry,
            worker
        );
    }
}

}

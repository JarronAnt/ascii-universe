#include "ascii/systems/HaulingSystems.hpp"

#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Position.hpp"
#include "ascii/systems/StockpileSystems.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <utility>
#include <vector>

namespace ascii::systems
{

namespace
{

bool hasUnfinishedHaulJob(
    const JobBoard& jobBoard,
    entt::entity item
)
{
    for (
        const auto& job :
        jobBoard.jobs()
    )
    {
        if (
            job.type ==
                JobType::Haul &&
            job.item ==
                item &&
            (
                job.state ==
                    JobState::Available ||
                job.state ==
                    JobState::Assigned
            )
        )
        {
            return true;
        }
    }

    return false;
}

void cancelHaulJob(
    entt::registry& registry,
    Job& job
)
{
    releaseStockpileCell(
        registry,
        job.destinationStockpile,
        job.destination
    );

    job.state =
        JobState::Cancelled;

    job.worker =
        entt::null;
}

}

void generateHaulJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
)
{
    auto view =
        registry.view<
            Item,
            Carriable,
            ItemState,
            Position
        >();

    std::vector<entt::entity>
        items;

    for (auto entity : view)
    {
        items.push_back(entity);
    }

    sortEntities(items);

    for (
        auto itemEntity :
        items
    )
    {
        const auto& state =
            registry.get<
                ItemState
            >(itemEntity);

        if (
            state.location !=
            ItemLocation::OnGround
        )
        {
            continue;
        }

        if (
            hasUnfinishedHaulJob(
                jobBoard,
                itemEntity
            )
        )
        {
            continue;
        }

        const auto& item =
            registry.get<
                Item
            >(itemEntity);

        const Position position =
            registry.get<
                Position
            >(itemEntity);

        auto destination =
            findStockpileDestination(
                registry,
                map,
                pathfinder,
                item.type,
                position
            );

        if (!destination)
        {
            continue;
        }

        reserveStockpileCell(
            registry,
            destination->stockpile,
            destination->position
        );

        jobBoard.addHaul(
            itemEntity,
            destination->stockpile,
            destination->position
        );
    }
}

void assignHaulJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
)
{
    auto view =
        registry.view<
            Goblin,
            Hauler,
            Position
        >();

    std::vector<entt::entity>
        haulers;

    for (auto entity : view)
    {
        haulers.push_back(entity);
    }

    sortEntities(haulers);

    for (auto hauler : haulers)
    {
        if (
            registry.all_of<
                AssignedJob
            >(hauler)
        )
        {
            continue;
        }

        const Position workerPosition =
            registry.get<
                Position
            >(hauler);

        for (auto& job : jobBoard.jobs())
        {
            if (
                job.state !=
                    JobState::Available ||
                job.type !=
                    JobType::Haul
            )
            {
                continue;
            }

            if (
                job.item == entt::null ||
                !registry.valid(
                    job.item
                ) ||
                !registry.all_of<
                    Item,
                    ItemState,
                    Position
                >(job.item)
            )
            {
                cancelHaulJob(
                    registry,
                    job
                );

                continue;
            }

            const auto& state =
                registry.get<
                    ItemState
                >(job.item);

            if (
                state.location !=
                ItemLocation::OnGround
            )
            {
                cancelHaulJob(
                    registry,
                    job
                );

                continue;
            }

            const Position itemPosition =
                registry.get<
                    Position
                >(job.item);

            auto path =
                pathfinder.findPath(
                    map,
                    workerPosition,
                    itemPosition
                );

            if (!path)
            {
                continue;
            }

            job.state =
                JobState::Assigned;

            job.worker =
                hauler;

            job.haulStage =
                HaulStage::ToItem;

            registry.emplace<
                AssignedJob
            >(hauler).id =
                job.id;

            setMovementPath(
                registry,
                hauler,
                std::move(*path)
            );

            break;
        }
    }
}

void executeHauling(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard,
    EventQueue<ItemPickupEvent>&
        itemPickupEvents,
    EventQueue<ItemDropEvent>&
        itemDropEvents
)
{
    auto view =
        registry.view<
            Goblin,
            Hauler,
            Position,
            AssignedJob
        >();

    std::vector<entt::entity>
        workers;

    for (auto entity : view)
    {
        workers.push_back(entity);
    }

    sortEntities(workers);

    for (auto worker : workers)
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
            job == nullptr ||
            job->type !=
                JobType::Haul ||
            job->state !=
                JobState::Assigned ||
            job->worker !=
                worker
        )
        {
            continue;
        }

        const Position workerPosition =
            registry.get<
                Position
            >(worker);

        // ==========================================
        // Walk to item
        // ==========================================

        if (
            job->haulStage ==
            HaulStage::ToItem
        )
        {
            if (
                !registry.valid(
                    job->item
                ) ||
                !registry.all_of<
                    ItemState,
                    Position
                >(job->item)
            )
            {
                cancelHaulJob(
                    registry,
                    *job
                );

                clearWorkerJob(
                    registry,
                    worker
                );

                continue;
            }

            const auto& state =
                registry.get<
                    ItemState
                >(job->item);

            if (
                state.location !=
                ItemLocation::OnGround
            )
            {
                cancelHaulJob(
                    registry,
                    *job
                );

                clearWorkerJob(
                    registry,
                    worker
                );

                continue;
            }

            if (
                registry.all_of<
                    MovementPath
                >(worker) &&
                !registry.get<
                    MovementPath
                >(worker).finished()
            )
            {
                continue;
            }

            const Position itemPosition =
                registry.get<
                    Position
                >(job->item);

            if (
                workerPosition !=
                itemPosition
            )
            {
                releaseJob(*job);

                clearWorkerJob(
                    registry,
                    worker
                );

                continue;
            }

            itemPickupEvents.emit(
                ItemPickupEvent{
                    job->item,
                    worker
                }
            );

            continue;
        }

        // ==========================================
        // Carry to stockpile
        // ==========================================

        if (
            !registry.valid(
                job->item
            ) ||
            !registry.all_of<
                ItemState
            >(job->item)
        )
        {
            cancelHaulJob(
                registry,
                *job
            );

            clearWorkerJob(
                registry,
                worker
            );

            continue;
        }

        const auto& state =
            registry.get<
                ItemState
            >(job->item);

        if (
            state.location !=
                ItemLocation::Carried ||
            state.carrier !=
                worker
        )
        {
            continue;
        }

        if (
            workerPosition ==
            job->destination
        )
        {
            itemDropEvents.emit(
                ItemDropEvent{
                    job->item,
                    worker,
                    job->destinationStockpile,
                    job->destination
                }
            );

            continue;
        }

        bool needsPath =
            true;

        if (
            registry.all_of<
                MovementPath
            >(worker)
        )
        {
            needsPath =
                registry.get<
                    MovementPath
                >(worker).finished();
        }

        if (!needsPath)
        {
            continue;
        }

        auto path =
            pathfinder.findPath(
                map,
                workerPosition,
                job->destination
            );

        if (path)
        {
            setMovementPath(
                registry,
                worker,
                std::move(*path)
            );
        }
    }
}

}

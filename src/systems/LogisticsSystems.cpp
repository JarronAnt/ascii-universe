#include "ascii/systems/LogisticsSystems.hpp"

#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Position.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace ascii::systems
{

namespace
{

struct StockpileDestination
{
    entt::entity stockpile{
        entt::null
    };

    Position position{};

    std::size_t pathLength{};
};

bool cellReserved(
    const Stockpile& stockpile,
    Position position
)
{
    return
        std::find(
            stockpile.reservedCells.begin(),
            stockpile.reservedCells.end(),
            position
        )
        !=
        stockpile.reservedCells.end();
}

bool cellOccupied(
    const entt::registry& registry,
    const Stockpile& stockpile,
    Position position
)
{
    for (
        auto item :
        stockpile.currentItems
    )
    {
        if (
            !registry.valid(item)
            ||
            !registry.all_of<
                Position
            >(item)
        )
        {
            continue;
        }

        if (
            registry.get<
                Position
            >(item)
            ==
            position
        )
        {
            return true;
        }
    }

    return false;
}

void releaseReservation(
    entt::registry& registry,
    entt::entity stockpileEntity,
    Position position
)
{
    if (
        stockpileEntity ==
            entt::null
        ||
        !registry.valid(
            stockpileEntity
        )
        ||
        !registry.all_of<
            Stockpile
        >(
            stockpileEntity
        )
    )
    {
        return;
    }

    auto& stockpile =
        registry.get<
            Stockpile
        >(
            stockpileEntity
        );

    const auto iterator =
        std::find(
            stockpile.
                reservedCells.begin(),
            stockpile.
                reservedCells.end(),
            position
        );

    if (
        iterator !=
        stockpile.
            reservedCells.end()
    )
    {
        stockpile.
            reservedCells.erase(
                iterator
            );
    }
}

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
                JobType::Haul
            &&
            job.item ==
                item
            &&
            (
                job.state ==
                    JobState::Available
                ||
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

std::optional<
    StockpileDestination
>
findStockpileDestination(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    ItemType itemType,
    Position itemPosition
)
{
    auto view =
        registry.view<
            Stockpile
        >();

    std::vector<entt::entity>
        stockpiles;

    for (auto entity : view)
    {
        stockpiles.push_back(
            entity
        );
    }

    sortEntities(
        stockpiles
    );

    std::optional<
        StockpileDestination
    > best;

    for (
        auto stockpileEntity :
        stockpiles
    )
    {
        auto& stockpile =
            registry.get<
                Stockpile
            >(
                stockpileEntity
            );

        if (
            !stockpile.acceptsItem(
                itemType
            )
            ||
            stockpile.full()
        )
        {
            continue;
        }

        for (
            int y =
                stockpile.bounds.
                    topLeft.y;
            y <=
                stockpile.bounds.
                    bottomRight.y;
            ++y
        )
        {
            for (
                int x =
                    stockpile.bounds.
                        topLeft.x;
                x <=
                    stockpile.bounds.
                        bottomRight.x;
                ++x
            )
            {
                const Position candidate{
                    x,
                    y
                };

                if (
                    !map.inBounds(
                        x,
                        y
                    )
                    ||
                    !map.at(
                        x,
                        y
                    ).walkable()
                )
                {
                    continue;
                }

                if (
                    cellOccupied(
                        registry,
                        stockpile,
                        candidate
                    )
                    ||
                    cellReserved(
                        stockpile,
                        candidate
                    )
                )
                {
                    continue;
                }

                auto path =
                    pathfinder.findPath(
                        map,
                        itemPosition,
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
                        best->
                            pathLength
                )
                {
                    best =
                        StockpileDestination{
                            stockpileEntity,
                            candidate,
                            path->size()
                        };
                }
            }
        }
    }

    return best;
}

void cancelHaulJob(
    entt::registry& registry,
    Job& job
)
{
    releaseReservation(
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

void processItemSpawns(
    entt::registry& registry,
    EventQueue<ItemSpawnEvent>&
        events
)
{
    const auto queued =
        events.take();

    for (
        const auto& event :
        queued
    )
    {
        const auto entity =
            registry.create();

        auto& item =
            registry.emplace<
                Item
            >(entity);

        item.type =
            event.itemType;

        item.weight =
            1;

        registry.emplace<
            Carriable
        >(entity);

        auto& state =
            registry.emplace<
                ItemState
            >(entity);

        state.location =
            ItemLocation::OnGround;

        state.carrier =
            entt::null;

        state.stockpile =
            entt::null;

        auto& position =
            registry.emplace<
                Position
            >(entity);

        position =
            event.position;

        auto& glyph =
            registry.emplace<
                Glyph
            >(entity);

        switch (
            event.itemType
        )
        {
            case ItemType::Stone:
                glyph.character =
                    's';

                glyph.color =
                    TerminalColor::
                        BrightWhite;

                break;
        }
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
        items.push_back(
            entity
        );
    }

    sortEntities(
        items
    );

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

        const Position itemPosition =
            registry.get<
                Position
            >(itemEntity);

        auto destination =
            findStockpileDestination(
                registry,
                map,
                pathfinder,
                item.type,
                itemPosition
            );

        if (!destination)
        {
            continue;
        }

        auto& stockpile =
            registry.get<
                Stockpile
            >(
                destination->
                    stockpile
            );

        // Reserve before creating the job.
        stockpile.
            reservedCells.
            push_back(
                destination->
                    position
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
        haulers.push_back(
            entity
        );
    }

    sortEntities(
        haulers
    );

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
                    JobType::Haul
            )
            {
                continue;
            }

            if (
                job.item ==
                    entt::null
                ||
                !registry.valid(
                    job.item
                )
                ||
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

            auto& assigned =
                registry.emplace<
                    AssignedJob
                >(hauler);

            assigned.id =
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
        workers.push_back(
            entity
        );
    }

    sortEntities(
        workers
    );

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

        if (
            job == nullptr
            ||
            job->type !=
                JobType::Haul
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
            continue;
        }

        const Position workerPosition =
            registry.get<
                Position
            >(worker);

        // ==========================================
        // Go to item
        // ==========================================

        if (
            job->haulStage ==
                HaulStage::ToItem
        )
        {
            if (
                !registry.valid(
                    job->item
                )
                ||
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

            const Position itemPosition =
                registry.get<
                    Position
                >(job->item);

            if (
                workerPosition !=
                    itemPosition
            )
            {
                releaseJob(
                    *job
                );

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
        // Go to stockpile
        // ==========================================

        if (
            job->haulStage ==
                HaulStage::ToStockpile
        )
        {
            if (
                !registry.valid(
                    job->item
                )
                ||
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
                    ItemLocation::Carried
                ||
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
                        job->
                            destinationStockpile,
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
                const auto& movement =
                    registry.get<
                        MovementPath
                    >(worker);

                needsPath =
                    movement.finished();
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

            if (!path)
            {
                // World may change again later.
                continue;
            }

            setMovementPath(
                registry,
                worker,
                std::move(*path)
            );
        }
    }
}

void processItemPickups(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard,
    EventQueue<ItemPickupEvent>&
        events
)
{
    const auto queued =
        events.take();

    for (
        const auto& event :
        queued
    )
    {
        if (
            !registry.valid(
                event.item
            )
            ||
            !registry.valid(
                event.agent
            )
            ||
            !registry.all_of<
                ItemState,
                Position
            >(event.item)
            ||
            !registry.all_of<
                AssignedJob,
                Position
            >(event.agent)
        )
        {
            continue;
        }

        auto& state =
            registry.get<
                ItemState
            >(event.item);

        if (
            state.location !=
                ItemLocation::OnGround
        )
        {
            continue;
        }

        const Position itemPosition =
            registry.get<
                Position
            >(event.item);

        const Position agentPosition =
            registry.get<
                Position
            >(event.agent);

        if (
            itemPosition !=
                agentPosition
        )
        {
            continue;
        }

        const auto& assigned =
            registry.get<
                AssignedJob
            >(event.agent);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (
            job == nullptr
            ||
            job->type !=
                JobType::Haul
            ||
            job->item !=
                event.item
        )
        {
            continue;
        }

        state.location =
            ItemLocation::Carried;

        state.carrier =
            event.agent;

        state.stockpile =
            entt::null;

        registry.remove<
            Position
        >(event.item);

        if (
            registry.all_of<
                CarryingItem
            >(event.agent)
        )
        {
            registry.get<
                CarryingItem
            >(event.agent).item =
                event.item;
        }
        else
        {
            auto& carrying =
                registry.emplace<
                    CarryingItem
                >(event.agent);

            carrying.item =
                event.item;
        }

        job->haulStage =
            HaulStage::ToStockpile;

        auto path =
            pathfinder.findPath(
                map,
                agentPosition,
                job->destination
            );

        if (path)
        {
            setMovementPath(
                registry,
                event.agent,
                std::move(*path)
            );
        }
        else if (
            registry.all_of<
                MovementPath
            >(event.agent)
        )
        {
            registry.remove<
                MovementPath
            >(event.agent);
        }
    }
}

void processItemDrops(
    entt::registry& registry,
    JobBoard& jobBoard,
    EventQueue<ItemDropEvent>&
        events
)
{
    const auto queued =
        events.take();

    for (
        const auto& event :
        queued
    )
    {
        if (
            !registry.valid(
                event.item
            )
            ||
            !registry.valid(
                event.agent
            )
            ||
            !registry.valid(
                event.stockpile
            )
            ||
            !registry.all_of<
                ItemState
            >(event.item)
            ||
            !registry.all_of<
                Stockpile
            >(event.stockpile)
            ||
            !registry.all_of<
                AssignedJob
            >(event.agent)
        )
        {
            continue;
        }

        auto& state =
            registry.get<
                ItemState
            >(event.item);

        if (
            state.location !=
                ItemLocation::Carried
            ||
            state.carrier !=
                event.agent
        )
        {
            continue;
        }

        auto& stockpile =
            registry.get<
                Stockpile
            >(event.stockpile);

        if (
            !stockpile.bounds.contains(
                event.destination
            )
        )
        {
            continue;
        }

        const auto& assigned =
            registry.get<
                AssignedJob
            >(event.agent);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (
            job == nullptr
            ||
            job->type !=
                JobType::Haul
            ||
            job->item !=
                event.item
        )
        {
            continue;
        }

        if (
            registry.all_of<
                Position
            >(event.item)
        )
        {
            registry.get<
                Position
            >(event.item) =
                event.destination;
        }
        else
        {
            auto& position =
                registry.emplace<
                    Position
                >(event.item);

            position =
                event.destination;
        }

        state.location =
            ItemLocation::Stockpiled;

        state.carrier =
            entt::null;

        state.stockpile =
            event.stockpile;

        if (
            std::find(
                stockpile.
                    currentItems.begin(),
                stockpile.
                    currentItems.end(),
                event.item
            )
            ==
            stockpile.
                currentItems.end()
        )
        {
            stockpile.
                currentItems.
                push_back(
                    event.item
                );
        }

        releaseReservation(
            registry,
            event.stockpile,
            event.destination
        );

        job->state =
            JobState::Complete;

        job->worker =
            entt::null;

        if (
            registry.all_of<
                CarryingItem
            >(event.agent)
        )
        {
            registry.remove<
                CarryingItem
            >(event.agent);
        }

        clearWorkerJob(
            registry,
            event.agent
        );
    }
}

}

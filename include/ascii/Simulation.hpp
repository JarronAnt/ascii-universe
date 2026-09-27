#pragma once

#include "Components.hpp"
#include "Designations.hpp"
#include "Events.hpp"
#include "GameMap.hpp"
#include "Items.hpp"
#include "Jobs.hpp"
#include "Pathfinder.hpp"
#include "Random.hpp"
#include "Stockpiles.hpp"

#include <entt/entt.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace ascii
{

struct SimulationTime
{
    std::uint64_t tick{0};
};

class Simulation
{
public:
    static constexpr std::uint32_t TickRate =
        10;

    static constexpr std::chrono::nanoseconds
        FixedStep{
            100'000'000
        };

    Simulation(
        int width,
        int height,
        std::uint64_t seed
    )
        : map_(width, height),
          random_(seed)
    {
    }

    // ==================================================
    // Real time -> fixed simulation time
    // ==================================================

    int advance(
        std::chrono::nanoseconds elapsed
    )
    {
        if (
            elapsed <=
            std::chrono::nanoseconds::zero()
        )
        {
            return 0;
        }

        accumulator_ += elapsed;

        int ticksExecuted = 0;

        while (
            accumulator_ >= FixedStep
        )
        {
            step();

            accumulator_ -= FixedStep;

            ++ticksExecuted;
        }

        return ticksExecuted;
    }

    // ==================================================
    // One deterministic simulation tick
    // ==================================================

    void step()
    {
        // ----------------------------------------------
        // Player intent -> jobs
        // ----------------------------------------------

        designationDedupSystem();

        designationToJobsSystem();

        // Existing loose items may need hauling.
        haulingJobGenerationSystem();

        // ----------------------------------------------
        // Assign work
        // ----------------------------------------------

        jobAssignmentSystem();

        // ----------------------------------------------
        // Movement
        // ----------------------------------------------

        movementSystem();

        // ----------------------------------------------
        // Work execution
        // ----------------------------------------------

        miningSystem();

        haulingSystem();

        // ----------------------------------------------
        // Events
        // ----------------------------------------------

        itemSpawnSystem();

        itemPickupSystem();

        itemDropSystem();

        // Mining may have created new items this tick.
        //
        // Generate their haul jobs immediately.
        haulingJobGenerationSystem();

        ++time_.tick;
    }

    // ==================================================
    // Player commands
    // ==================================================

    entt::entity designateMine(
        Position position
    )
    {
        if (
            !map_.inBounds(
                position.x,
                position.y
            )
        )
        {
            return entt::null;
        }

        if (
            map_.at(
                position.x,
                position.y
            ).type != TileType::Wall
        )
        {
            return entt::null;
        }

        const auto entity =
            registry_.create();

        registry_.emplace<
            MineDesignation
        >(
            entity
        );

        registry_.emplace<
            DesignationLifecycle
        >(
            entity
        );

        auto& entityPosition =
            registry_.emplace<
                Position
            >(
                entity
            );

        entityPosition =
            position;

        auto& glyph =
            registry_.emplace<
                Glyph
            >(
                entity
            );

        glyph.character =
            'X';

        return entity;
    }

    entt::entity createStockpile(
        Position topLeft,
        Position bottomRight,
        std::vector<ItemType> accepts
    )
    {
        if (
            topLeft.x >
                bottomRight.x
            ||
            topLeft.y >
                bottomRight.y
        )
        {
            return entt::null;
        }

        if (
            !map_.inBounds(
                topLeft.x,
                topLeft.y
            )
            ||
            !map_.inBounds(
                bottomRight.x,
                bottomRight.y
            )
        )
        {
            return entt::null;
        }

        const auto entity =
            registry_.create();

        auto& stockpile =
            registry_.emplace<
                Stockpile
            >(
                entity
            );

        stockpile.bounds.topLeft =
            topLeft;

        stockpile.bounds.bottomRight =
            bottomRight;

        stockpile.accepts =
            std::move(accepts);

        return entity;
    }

    // ==================================================
    // Public state access
    // ==================================================

    GameMap& map()
    {
        return map_;
    }

    const GameMap& map() const
    {
        return map_;
    }

    entt::registry& registry()
    {
        return registry_;
    }

    const entt::registry& registry() const
    {
        return registry_;
    }

    Random& random()
    {
        return random_;
    }

    const Random& random() const
    {
        return random_;
    }

    JobBoard& jobBoard()
    {
        return jobBoard_;
    }

    const JobBoard& jobBoard() const
    {
        return jobBoard_;
    }

    [[nodiscard]]
    const SimulationTime& time() const
    {
        return time_;
    }

    [[nodiscard]]
    bool hasOutstandingWork()
    {
        if (
            jobBoard_.hasUnfinished()
        )
        {
            return true;
        }

        if (
            !itemSpawnEvents_.empty()
            ||
            !itemPickupEvents_.empty()
            ||
            !itemDropEvents_.empty()
        )
        {
            return true;
        }

        auto designationView =
            registry_.view<
                MineDesignation,
                DesignationLifecycle
            >();

        for (
            auto entity :
            designationView
        )
        {
            const auto& lifecycle =
                designationView.get<
                    DesignationLifecycle
                >(
                    entity
                );

            if (
                lifecycle.state ==
                    DesignationState::Active
            )
            {
                return true;
            }
        }

        return false;
    }

private:
    // ==================================================
    // DESIGNATIONS
    // ==================================================

    void designationDedupSystem()
    {
        auto view =
            registry_.view<
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
                >(
                    entity
                );

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
                registry_.get<
                    Position
                >(
                    entity
                );

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

            if (!inserted)
            {
                auto& lifecycle =
                    registry_.get<
                        DesignationLifecycle
                    >(
                        entity
                    );

                lifecycle.state =
                    DesignationState::Ignored;

                hideDesignation(
                    entity
                );
            }
        }
    }

    void designationToJobsSystem()
    {
        auto view =
            registry_.view<
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
                >(
                    entity
                );

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
                registry_.get<
                    DesignationLifecycle
                >(
                    entity
                );

            const Position position =
                registry_.get<
                    Position
                >(
                    entity
                );

            if (
                !map_.inBounds(
                    position.x,
                    position.y
                )
                ||
                map_.at(
                    position.x,
                    position.y
                ).type !=
                    TileType::Wall
            )
            {
                lifecycle.state =
                    DesignationState::Ignored;

                hideDesignation(
                    entity
                );

                continue;
            }

            jobBoard_.add(
                JobType::Mine,
                position,
                entity
            );

            lifecycle.state =
                DesignationState::Consumed;
        }
    }

    // ==================================================
    // JOB ASSIGNMENT
    // ==================================================

    void jobAssignmentSystem()
    {
        miningJobAssignmentSystem();

        haulingJobAssignmentSystem();
    }

    void miningJobAssignmentSystem()
    {
        auto view =
            registry_.view<
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
                registry_.all_of<
                    AssignedJob
                >(
                    miner
                )
            )
            {
                continue;
            }

            const Position start =
                registry_.get<
                    Position
                >(
                    miner
                );

            for (
                auto& job :
                jobBoard_.jobs()
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
                    registry_.emplace<
                        AssignedJob
                    >(
                        miner
                    );

                assigned.id =
                    job.id;

                setMovementPath(
                    miner,
                    std::move(
                        approach->path
                    )
                );

                break;
            }
        }
    }

    void haulingJobAssignmentSystem()
    {
        auto view =
            registry_.view<
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
                registry_.all_of<
                    AssignedJob
                >(
                    hauler
                )
            )
            {
                continue;
            }

            const Position workerPosition =
                registry_.get<
                    Position
                >(
                    hauler
                );

            for (
                auto& job :
                jobBoard_.jobs()
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
                    !registry_.valid(
                        job.item
                    )
                    ||
                    !registry_.all_of<
                        Item,
                        ItemState,
                        Position
                    >(
                        job.item
                    )
                )
                {
                    cancelHaulJob(
                        job
                    );

                    continue;
                }

                const auto& state =
                    registry_.get<
                        ItemState
                    >(
                        job.item
                    );

                if (
                    state.location !=
                        ItemLocation::OnGround
                )
                {
                    cancelHaulJob(
                        job
                    );

                    continue;
                }

                const Position itemPosition =
                    registry_.get<
                        Position
                    >(
                        job.item
                    );

                auto path =
                    pathfinder_.findPath(
                        map_,
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
                    registry_.emplace<
                        AssignedJob
                    >(
                        hauler
                    );

                assigned.id =
                    job.id;

                setMovementPath(
                    hauler,
                    std::move(*path)
                );

                break;
            }
        }
    }

    // ==================================================
    // MOVEMENT
    // ==================================================

    void movementSystem()
    {
        auto view =
            registry_.view<
                Position,
                MovementPath
            >();

        std::vector<entt::entity>
            entities;

        for (auto entity : view)
        {
            entities.push_back(
                entity
            );
        }

        sortEntities(
            entities
        );

        for (auto entity : entities)
        {
            auto& position =
                registry_.get<
                    Position
                >(
                    entity
                );

            auto& path =
                registry_.get<
                    MovementPath
                >(
                    entity
                );

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
                !map_.inBounds(
                    next.x,
                    next.y
                )
                ||
                !map_.at(
                    next.x,
                    next.y
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

    // ==================================================
    // MINING
    // ==================================================

    void miningSystem()
    {
        auto view =
            registry_.view<
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
                registry_.get<
                    AssignedJob
                >(
                    worker
                );

            Job* job =
                jobBoard_.find(
                    assigned.id
                );

            if (job == nullptr)
            {
                workersToClear.push_back(
                    worker
                );

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
                job->type !=
                    JobType::Mine
            )
            {
                continue;
            }

            if (
                registry_.all_of<
                    MovementPath
                >(
                    worker
                )
            )
            {
                const auto& movement =
                    registry_.get<
                        MovementPath
                    >(
                        worker
                    );

                if (
                    !movement.finished()
                )
                {
                    continue;
                }
            }

            const Position workerPosition =
                registry_.get<
                    Position
                >(
                    worker
                );

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
                !map_.inBounds(
                    job->target.x,
                    job->target.y
                )
            )
            {
                job->state =
                    JobState::Cancelled;

                hideDesignation(
                    job->
                        sourceDesignation
                );

                workersToClear.push_back(
                    worker
                );

                continue;
            }

            Tile& targetTile =
                map_.at(
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

                hideDesignation(
                    job->
                        sourceDesignation
                );

                workersToClear.push_back(
                    worker
                );

                continue;
            }

            // ------------------------------------------
            // Mine wall
            // ------------------------------------------

            targetTile.type =
                TileType::Floor;

            // Do not create the stone directly here.
            //
            // Mining emits an event and the item
            // subsystem handles entity creation.
            itemSpawnEvents_.emit(
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
                worker
            );
        }
    }

    // ==================================================
    // ITEM SPAWNING
    // ==================================================

    void itemSpawnSystem()
    {
        const auto events =
            itemSpawnEvents_.take();

        for (
            const auto& event :
            events
        )
        {
            const auto entity =
                registry_.create();

            auto& item =
                registry_.emplace<
                    Item
                >(
                    entity
                );

            item.type =
                event.itemType;

            item.weight =
                1;

            registry_.emplace<
                Carriable
            >(
                entity
            );

            auto& state =
                registry_.emplace<
                    ItemState
                >(
                    entity
                );

            state.location =
                ItemLocation::OnGround;

            state.carrier =
                entt::null;

            state.stockpile =
                entt::null;

            auto& position =
                registry_.emplace<
                    Position
                >(
                    entity
                );

            position =
                event.position;

            auto& glyph =
                registry_.emplace<
                    Glyph
                >(
                    entity
                );

            switch (
                event.itemType
            )
            {
                case ItemType::Stone:
                    glyph.character =
                        's';
                    break;
            }
        }
    }

    // ==================================================
    // HAUL JOB GENERATION
    // ==================================================

    void haulingJobGenerationSystem()
    {
        auto view =
            registry_.view<
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

        for (auto itemEntity : items)
        {
            const auto& state =
                registry_.get<
                    ItemState
                >(
                    itemEntity
                );

            if (
                state.location !=
                    ItemLocation::OnGround
            )
            {
                continue;
            }

            if (
                hasUnfinishedHaulJob(
                    itemEntity
                )
            )
            {
                continue;
            }

            const auto& item =
                registry_.get<
                    Item
                >(
                    itemEntity
                );

            const Position itemPosition =
                registry_.get<
                    Position
                >(
                    itemEntity
                );

            auto destination =
                findStockpileDestination(
                    item.type,
                    itemPosition
                );

            if (!destination)
            {
                continue;
            }

            auto& stockpile =
                registry_.get<
                    Stockpile
                >(
                    destination->
                        stockpile
                );

            // Reserve the destination immediately so
            // another stone can't choose it.
            stockpile.reservedCells.push_back(
                destination->position
            );

            jobBoard_.addHaul(
                itemEntity,
                destination->stockpile,
                destination->position
            );
        }
    }

    // ==================================================
    // HAULING
    // ==================================================

    void haulingSystem()
    {
        auto view =
            registry_.view<
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
                registry_.get<
                    AssignedJob
                >(
                    worker
                );

            Job* job =
                jobBoard_.find(
                    assigned.id
                );

            if (
                job == nullptr
                ||
                job->type !=
                    JobType::Haul
                ||
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
                registry_.get<
                    Position
                >(
                    worker
                );

            // ==========================================
            // Stage 1: move to the item
            // ==========================================

            if (
                job->haulStage ==
                    HaulStage::ToItem
            )
            {
                if (
                    !registry_.valid(
                        job->item
                    )
                    ||
                    !registry_.all_of<
                        ItemState,
                        Position
                    >(
                        job->item
                    )
                )
                {
                    cancelHaulJob(
                        *job
                    );

                    clearWorkerJob(
                        worker
                    );

                    continue;
                }

                const auto& state =
                    registry_.get<
                        ItemState
                    >(
                        job->item
                    );

                if (
                    state.location !=
                        ItemLocation::OnGround
                )
                {
                    cancelHaulJob(
                        *job
                    );

                    clearWorkerJob(
                        worker
                    );

                    continue;
                }

                if (
                    registry_.all_of<
                        MovementPath
                    >(
                        worker
                    )
                )
                {
                    const auto& movement =
                        registry_.get<
                            MovementPath
                        >(
                            worker
                        );

                    if (
                        !movement.finished()
                    )
                    {
                        continue;
                    }
                }

                const Position itemPosition =
                    registry_.get<
                        Position
                    >(
                        job->item
                    );

                if (
                    workerPosition !=
                        itemPosition
                )
                {
                    // Path ended without reaching item.
                    //
                    // Make the job available again.
                    releaseJob(
                        *job
                    );

                    clearWorkerJob(
                        worker
                    );

                    continue;
                }

                itemPickupEvents_.emit(
                    ItemPickupEvent{
                        job->item,
                        worker
                    }
                );

                continue;
            }

            // ==========================================
            // Stage 2: carry item to stockpile
            // ==========================================

            if (
                job->haulStage ==
                    HaulStage::ToStockpile
            )
            {
                if (
                    !registry_.valid(
                        job->item
                    )
                    ||
                    !registry_.all_of<
                        ItemState
                    >(
                        job->item
                    )
                )
                {
                    cancelHaulJob(
                        *job
                    );

                    clearWorkerJob(
                        worker
                    );

                    continue;
                }

                const auto& state =
                    registry_.get<
                        ItemState
                    >(
                        job->item
                    );

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
                    itemDropEvents_.emit(
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
                    registry_.all_of<
                        MovementPath
                    >(
                        worker
                    )
                )
                {
                    const auto& movement =
                        registry_.get<
                            MovementPath
                        >(
                            worker
                        );

                    needsPath =
                        movement.finished();
                }

                if (!needsPath)
                {
                    continue;
                }

                auto path =
                    pathfinder_.findPath(
                        map_,
                        workerPosition,
                        job->destination
                    );

                if (!path)
                {
                    // Destination is currently
                    // unreachable.
                    //
                    // Keep the item carried and retry
                    // on future ticks.
                    continue;
                }

                setMovementPath(
                    worker,
                    std::move(*path)
                );
            }
        }
    }

    // ==================================================
    // ITEM PICKUP
    // ==================================================

    void itemPickupSystem()
    {
        const auto events =
            itemPickupEvents_.take();

        for (
            const auto& event :
            events
        )
        {
            if (
                !registry_.valid(
                    event.item
                )
                ||
                !registry_.valid(
                    event.agent
                )
                ||
                !registry_.all_of<
                    ItemState,
                    Position
                >(
                    event.item
                )
                ||
                !registry_.all_of<
                    AssignedJob,
                    Position
                >(
                    event.agent
                )
            )
            {
                continue;
            }

            auto& state =
                registry_.get<
                    ItemState
                >(
                    event.item
                );

            if (
                state.location !=
                    ItemLocation::OnGround
            )
            {
                continue;
            }

            const Position itemPosition =
                registry_.get<
                    Position
                >(
                    event.item
                );

            const Position agentPosition =
                registry_.get<
                    Position
                >(
                    event.agent
                );

            if (
                itemPosition !=
                    agentPosition
            )
            {
                continue;
            }

            const auto& assigned =
                registry_.get<
                    AssignedJob
                >(
                    event.agent
                );

            Job* job =
                jobBoard_.find(
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

            // Item is no longer physically sitting
            // on the ground.
            registry_.remove<
                Position
            >(
                event.item
            );

            if (
                registry_.all_of<
                    CarryingItem
                >(
                    event.agent
                )
            )
            {
                auto& carrying =
                    registry_.get<
                        CarryingItem
                    >(
                        event.agent
                    );

                carrying.item =
                    event.item;
            }
            else
            {
                auto& carrying =
                    registry_.emplace<
                        CarryingItem
                    >(
                        event.agent
                    );

                carrying.item =
                    event.item;
            }

            job->haulStage =
                HaulStage::ToStockpile;

            // Replace the route-to-item with a route
            // to the reserved stockpile tile.
            auto path =
                pathfinder_.findPath(
                    map_,
                    agentPosition,
                    job->destination
                );

            if (path)
            {
                setMovementPath(
                    event.agent,
                    std::move(*path)
                );
            }
            else
            {
                if (
                    registry_.all_of<
                        MovementPath
                    >(
                        event.agent
                    )
                )
                {
                    registry_.remove<
                        MovementPath
                    >(
                        event.agent
                    );
                }
            }
        }
    }

    // ==================================================
    // ITEM DROP
    // ==================================================

    void itemDropSystem()
    {
        const auto events =
            itemDropEvents_.take();

        for (
            const auto& event :
            events
        )
        {
            if (
                !registry_.valid(
                    event.item
                )
                ||
                !registry_.valid(
                    event.agent
                )
                ||
                !registry_.valid(
                    event.stockpile
                )
                ||
                !registry_.all_of<
                    ItemState
                >(
                    event.item
                )
                ||
                !registry_.all_of<
                    Stockpile
                >(
                    event.stockpile
                )
                ||
                !registry_.all_of<
                    AssignedJob
                >(
                    event.agent
                )
            )
            {
                continue;
            }

            auto& state =
                registry_.get<
                    ItemState
                >(
                    event.item
                );

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
                registry_.get<
                    Stockpile
                >(
                    event.stockpile
                );

            if (
                !stockpile.bounds.contains(
                    event.destination
                )
            )
            {
                continue;
            }

            const auto& assigned =
                registry_.get<
                    AssignedJob
                >(
                    event.agent
                );

            Job* job =
                jobBoard_.find(
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

            // ------------------------------------------
            // Put item into the world again
            // ------------------------------------------

            if (
                registry_.all_of<
                    Position
                >(
                    event.item
                )
            )
            {
                registry_.get<
                    Position
                >(
                    event.item
                ) = event.destination;
            }
            else
            {
                auto& position =
                    registry_.emplace<
                        Position
                    >(
                        event.item
                    );

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
                    stockpile.currentItems.begin(),
                    stockpile.currentItems.end(),
                    event.item
                )
                ==
                stockpile.currentItems.end()
            )
            {
                stockpile.currentItems.push_back(
                    event.item
                );
            }

            releaseStockpileReservation(
                event.stockpile,
                event.destination
            );

            job->state =
                JobState::Complete;

            job->worker =
                entt::null;

            if (
                registry_.all_of<
                    CarryingItem
                >(
                    event.agent
                )
            )
            {
                registry_.remove<
                    CarryingItem
                >(
                    event.agent
                );
            }

            clearWorkerJob(
                event.agent
            );
        }
    }

    // ==================================================
    // MINING PATHFINDING
    // ==================================================

    struct MiningApproach
    {
        Position workPosition{};

        std::vector<Position>
            path;
    };

    [[nodiscard]]
    std::optional<MiningApproach>
    findMiningApproach(
        Position worker,
        Position wall
    ) const
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
                !map_.inBounds(
                    candidate.x,
                    candidate.y
                )
            )
            {
                continue;
            }

            if (
                !map_.at(
                    candidate.x,
                    candidate.y
                ).walkable()
            )
            {
                continue;
            }

            auto path =
                pathfinder_.findPath(
                    map_,
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
                    std::move(
                        approach
                    );
            }
        }

        return best;
    }

    // ==================================================
    // STOCKPILE SEARCH
    // ==================================================

    struct StockpileDestination
    {
        entt::entity stockpile{
            entt::null
        };

        Position position{};

        std::size_t pathLength{};
    };

    [[nodiscard]]
    std::optional<StockpileDestination>
    findStockpileDestination(
        ItemType itemType,
        Position itemPosition
    )
    {
        auto view =
            registry_.view<
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
                registry_.get<
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
                    stockpile.bounds.topLeft.y;
                y <=
                    stockpile.bounds.bottomRight.y;
                ++y
            )
            {
                for (
                    int x =
                        stockpile.bounds.topLeft.x;
                    x <=
                        stockpile.bounds.bottomRight.x;
                    ++x
                )
                {
                    const Position candidate{
                        x,
                        y
                    };

                    if (
                        !map_.inBounds(
                            x,
                            y
                        )
                        ||
                        !map_.at(
                            x,
                            y
                        ).walkable()
                    )
                    {
                        continue;
                    }

                    if (
                        stockpileCellOccupied(
                            stockpile,
                            candidate
                        )
                        ||
                        stockpileCellReserved(
                            stockpile,
                            candidate
                        )
                    )
                    {
                        continue;
                    }

                    // Ensure the destination is
                    // actually reachable.
                    auto path =
                        pathfinder_.findPath(
                            map_,
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
                            best->pathLength
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

    // ==================================================
    // HELPERS
    // ==================================================

    [[nodiscard]]
    bool hasUnfinishedHaulJob(
        entt::entity item
    ) const
    {
        for (
            const auto& job :
            jobBoard_.jobs()
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

    [[nodiscard]]
    bool stockpileCellReserved(
        const Stockpile& stockpile,
        Position position
    ) const
    {
        return std::find(
            stockpile.reservedCells.begin(),
            stockpile.reservedCells.end(),
            position
        ) !=
        stockpile.reservedCells.end();
    }

    [[nodiscard]]
    bool stockpileCellOccupied(
        const Stockpile& stockpile,
        Position position
    ) const
    {
        for (
            auto item :
            stockpile.currentItems
        )
        {
            if (
                !registry_.valid(
                    item
                )
                ||
                !registry_.all_of<
                    Position
                >(
                    item
                )
            )
            {
                continue;
            }

            if (
                registry_.get<
                    Position
                >(
                    item
                )
                ==
                position
            )
            {
                return true;
            }
        }

        return false;
    }

    void releaseStockpileReservation(
        entt::entity stockpileEntity,
        Position position
    )
    {
        if (
            stockpileEntity ==
                entt::null
            ||
            !registry_.valid(
                stockpileEntity
            )
            ||
            !registry_.all_of<
                Stockpile
            >(
                stockpileEntity
            )
        )
        {
            return;
        }

        auto& stockpile =
            registry_.get<
                Stockpile
            >(
                stockpileEntity
            );

        auto iterator =
            std::find(
                stockpile.reservedCells.begin(),
                stockpile.reservedCells.end(),
                position
            );

        if (
            iterator !=
                stockpile.reservedCells.end()
        )
        {
            stockpile.reservedCells.erase(
                iterator
            );
        }
    }

    void setMovementPath(
        entt::entity worker,
        std::vector<Position> path
    )
    {
        if (
            registry_.all_of<
                MovementPath
            >(
                worker
            )
        )
        {
            registry_.remove<
                MovementPath
            >(
                worker
            );
        }

        auto& movement =
            registry_.emplace<
                MovementPath
            >(
                worker
            );

        movement.nodes =
            std::move(path);

        movement.nextStep =
            0;
    }

    void clearWorkerJob(
        entt::entity worker
    )
    {
        if (
            !registry_.valid(
                worker
            )
        )
        {
            return;
        }

        if (
            registry_.all_of<
                MovementPath
            >(
                worker
            )
        )
        {
            registry_.remove<
                MovementPath
            >(
                worker
            );
        }

        if (
            registry_.all_of<
                AssignedJob
            >(
                worker
            )
        )
        {
            registry_.remove<
                AssignedJob
            >(
                worker
            );
        }
    }

    void cancelHaulJob(
        Job& job
    )
    {
        releaseStockpileReservation(
            job.destinationStockpile,
            job.destination
        );

        job.state =
            JobState::Cancelled;

        job.worker =
            entt::null;
    }

    static void releaseJob(
        Job& job
    )
    {
        job.state =
            JobState::Available;

        job.worker =
            entt::null;
    }

    static bool isAdjacent(
        Position first,
        Position second
    )
    {
        int dx =
            first.x -
            second.x;

        int dy =
            first.y -
            second.y;

        if (dx < 0)
        {
            dx = -dx;
        }

        if (dy < 0)
        {
            dy = -dy;
        }

        return
            dx + dy == 1;
    }

    static void sortEntities(
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

    void hideDesignation(
        entt::entity entity
    )
    {
        if (
            entity ==
                entt::null
            ||
            !registry_.valid(
                entity
            )
        )
        {
            return;
        }

        if (
            registry_.all_of<
                Glyph
            >(
                entity
            )
        )
        {
            registry_.remove<
                Glyph
            >(
                entity
            );
        }
    }

    // ==================================================
    // Simulation resources
    // ==================================================

    GameMap map_;

    entt::registry registry_;

    Random random_;

    Pathfinder pathfinder_;

    JobBoard jobBoard_;

    EventQueue<ItemSpawnEvent>
        itemSpawnEvents_;

    EventQueue<ItemPickupEvent>
        itemPickupEvents_;

    EventQueue<ItemDropEvent>
        itemDropEvents_;

    SimulationTime time_;

    std::chrono::nanoseconds
        accumulator_{0};
};

}

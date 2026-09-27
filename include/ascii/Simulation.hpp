#pragma once

#include "Components.hpp"
#include "Designations.hpp"
#include "GameMap.hpp"
#include "Jobs.hpp"
#include "Pathfinder.hpp"
#include "Random.hpp"

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
    // --------------------------------------------------
    // Fixed timestep
    // --------------------------------------------------

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

    // --------------------------------------------------
    // Real time -> fixed simulation ticks
    // --------------------------------------------------

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

    // --------------------------------------------------
    // One deterministic simulation tick
    // --------------------------------------------------

    void step()
    {
        // Ordering matters.
        //
        // Designations must be deduplicated BEFORE
        // they become jobs.

        designationDedupSystem();

        designationToJobsSystem();

        // Idle workers claim jobs and receive paths.
        jobAssignmentSystem();

        // Workers travel along their assigned path.
        movementSystem();

        // Workers perform work after reaching
        // the appropriate work position.
        miningSystem();

        ++time_.tick;
    }

    // --------------------------------------------------
    // Player commands
    // --------------------------------------------------

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

        glyph.character = 'X';

        return entity;
    }

    // --------------------------------------------------
    // Public state access
    // --------------------------------------------------

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

    // Useful for our CLI demo.
    //
    // Returns true while either:
    //
    //     active designations exist
    // or
    //     unfinished jobs exist
    bool hasOutstandingWork()
    {
        if (
            jobBoard_.hasUnfinished()
        )
        {
            return true;
        }

        auto view =
            registry_.view<
                MineDesignation,
                DesignationLifecycle
            >();

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
                return true;
            }
        }

        return false;
    }

private:
    // ==================================================
    // Designation deduplication
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

        sortEntities(entities);

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
                    DesignationState::
                        Ignored;

                hideDesignation(
                    entity
                );
            }
        }
    }

    // ==================================================
    // Designation -> Job
    // ==================================================

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

        sortEntities(entities);

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

            // It might have become invalid before this
            // system executes.
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
                    DesignationState::
                        Ignored;

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
                DesignationState::
                    Consumed;
        }
    }

    // ==================================================
    // Profession-aware job assignment
    // ==================================================

    void jobAssignmentSystem()
    {
        // Only goblins with the Miner component
        // are even considered for Mine jobs.
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

        sortEntities(miners);

        for (auto miner : miners)
        {
            // Already working.
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
                )
                {
                    continue;
                }

                if (
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

                // Job isn't currently reachable
                // from this miner.
                if (!approach)
                {
                    continue;
                }

                // Claim job.
                job.state =
                    JobState::Assigned;

                job.worker =
                    miner;

                job.workPosition =
                    approach->
                        workPosition;

                // Add AssignedJob component.
                auto& assigned =
                    registry_.emplace<
                        AssignedJob
                    >(
                        miner
                    );

                assigned.id =
                    job.id;

                // Ensure stale movement data isn't
                // hanging around.
                if (
                    registry_.all_of<
                        MovementPath
                    >(
                        miner
                    )
                )
                {
                    registry_.remove<
                        MovementPath
                    >(
                        miner
                    );
                }

                auto& movement =
                    registry_.emplace<
                        MovementPath
                    >(
                        miner
                    );

                movement.nodes =
                    std::move(
                        approach->path
                    );

                movement.nextStep =
                    0;

                // One job per worker.
                break;
            }
        }
    }

    // ==================================================
    // Movement
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

        sortEntities(entities);

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

            if (path.finished())
            {
                continue;
            }

            const Position next =
                path.nodes[
                    path.nextStep
                ];

            // A wall or some other obstacle may have
            // appeared after path calculation.
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
                // Force the path into the finished
                // state.
                //
                // miningSystem() will notice that the
                // worker didn't reach workPosition and
                // release the job for another attempt.
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
    // Mining execution
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

        sortEntities(workers);

        // Removing AssignedJob while iterating the
        // view could invalidate the view.
        //
        // Instead, collect workers and clean them up
        // after processing.
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

            // Worker still walking.
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

                if (!movement.finished())
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

            // Path may have become invalid after
            // assignment.
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

            // Miner must stand immediately beside
            // the target wall.
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

            // The world may have changed since job
            // creation.
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

            // Someone/something may already have
            // removed this wall.
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
            // ACTUAL MINING
            // ------------------------------------------

            targetTile.type =
                TileType::Floor;

            job->state =
                JobState::Complete;

            // Remove the X now that the wall has
            // actually been mined.
            hideDesignation(
                job->
                    sourceDesignation
            );

            workersToClear.push_back(
                worker
            );
        }

        // ----------------------------------------------
        // Worker cleanup
        // ----------------------------------------------

        for (
            auto worker :
            workersToClear
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
    }

    // ==================================================
    // Mining pathfinding helper
    // ==================================================

    struct MiningApproach
    {
        Position workPosition{};

        std::vector<Position> path;
    };

    [[nodiscard]]
    std::optional<MiningApproach>
    findMiningApproach(
        Position worker,
        Position wall
    ) const
    {
        // Deterministic order:
        //
        // north
        // east
        // south
        // west
        //
        // If two paths have the same length,
        // the first one encountered wins.
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
                    std::move(approach);
            }
        }

        return best;
    }

    // ==================================================
    // Helpers
    // ==================================================

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
            (dx + dy) == 1;
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

    static void releaseJob(
        Job& job
    )
    {
        job.state =
            JobState::Available;

        job.worker =
            entt::null;
    }

    // ==================================================
    // Simulation resources
    // ==================================================

    GameMap map_;

    entt::registry registry_;

    Random random_;

    Pathfinder pathfinder_;

    JobBoard jobBoard_;

    SimulationTime time_;

    std::chrono::nanoseconds
        accumulator_{0};
};

}

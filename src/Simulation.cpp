#include "ascii/Simulation.hpp"

#include "ascii/Components.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"

#include "ascii/systems/DesignationSystems.hpp"
#include "ascii/systems/ExcavationSystems.hpp"
#include "ascii/systems/FellingSystems.hpp"
#include "ascii/systems/HaulingSystems.hpp"
#include "ascii/systems/ItemSystems.hpp"
#include "ascii/systems/MovementSystem.hpp"
#include "ascii/systems/SystemUtils.hpp"
#include "ascii/systems/WaterSystem.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace ascii
{

namespace
{

bool stockpileBoundsOverlap(
    const StockpileBounds& first,
    const StockpileBounds& second
)
{
    return !(
        first.max.x < second.min.x
        ||
        first.min.x > second.max.x
        ||
        first.max.y < second.min.y
        ||
        first.min.y > second.max.y
        ||
        first.max.z < second.min.z
        ||
        first.min.z > second.max.z
    );
}

}

Simulation::Simulation(
    int width,
    int height,
    int depth,
    std::uint64_t seed
)
    : map_(
        width,
        height,
        depth
      ),
      random_(seed),
      worldSeed_(seed)
{
}

int Simulation::advance(
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
        accumulator_ >=
        FixedStep
    )
    {
        step();

        accumulator_ -=
            FixedStep;

        ++ticksExecuted;
    }

    return ticksExecuted;
}

void Simulation::step()
{
    systems::deduplicateDesignations(
        registry_
    );

    systems::createDesignationJobs(
        registry_,
        map_,
        jobBoard_
    );

    systems::generateHaulJobs(
        registry_,
        map_,
        pathfinder_,
        jobBoard_
    );

    systems::assignExcavationJobs(
        registry_,
        map_,
        pathfinder_,
        jobBoard_
    );

    systems::assignFellingJobs(
        registry_,
        map_,
        pathfinder_,
        jobBoard_
    );

    systems::assignHaulJobs(
        registry_,
        map_,
        pathfinder_,
        jobBoard_
    );

    systems::updateMovement(
        registry_,
        map_
    );

    systems::executeExcavation(
        registry_,
        map_,
        jobBoard_,
        itemSpawnEvents_
    );

    systems::executeFelling(
        registry_,
        map_,
        jobBoard_,
        itemSpawnEvents_
    );

    systems::executeHauling(
        registry_,
        map_,
        pathfinder_,
        jobBoard_,
        itemPickupEvents_,
        itemDropEvents_
    );

    systems::processItemSpawns(
        registry_,
        itemSpawnEvents_
    );

    systems::processItemPickups(
        registry_,
        map_,
        pathfinder_,
        jobBoard_,
        itemPickupEvents_
    );

    systems::processItemDrops(
        registry_,
        jobBoard_,
        itemDropEvents_
    );

    systems::generateHaulJobs(
        registry_,
        map_,
        pathfinder_,
        jobBoard_
    );

    if (
        time_.tick % 2 == 0
    )
    {
        systems::updateWater(
            map_
        );
    }

    ++time_.tick;
}

bool Simulation::hasOutstandingDesignationAt(
    Position position
) const
{
    auto view =
        registry_.view<
            Designation,
            Position,
            DesignationLifecycle
        >();

    for (auto entity : view)
    {
        if (
            view.get<
                Position
            >(entity)
            !=
            position
        )
        {
            continue;
        }

        const auto state =
            view.get<
                DesignationLifecycle
            >(entity).state;

        if (
            state ==
            DesignationState::Active
        )
        {
            return true;
        }

        if (
            state ==
            DesignationState::Consumed
        )
        {
            for (
                const Job& job :
                jobBoard_.jobs()
            )
            {
                if (
                    job.sourceDesignation ==
                    entity
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
        }
    }

    return false;
}

entt::entity
Simulation::createDesignation(
    Position position,
    DesignationType type,
    char character,
    TerminalColor color
)
{
    if (
        !map_.inBounds(position)
    )
    {
        return entt::null;
    }

    if (
        hasOutstandingDesignationAt(
            position
        )
    )
    {
        return entt::null;
    }

    const auto entity =
        registry_.create();

    registry_.emplace<
        Designation
    >(entity).type =
        type;

    registry_.emplace<
        DesignationLifecycle
    >(entity);

    registry_.emplace<
        Position
    >(entity) =
        position;

    auto& glyph =
        registry_.emplace<
            Glyph
        >(entity);

    glyph.character =
        character;

    glyph.color =
        color;

    return entity;
}

entt::entity
Simulation::designateMine(
    Position position
)
{
    if (
        !map_.inBounds(position)
        ||
        map_.at(position).shape !=
            TileShape::Wall
    )
    {
        return entt::null;
    }

    return createDesignation(
        position,
        DesignationType::Mine,
        'X',
        TerminalColor::BrightRed
    );
}

entt::entity
Simulation::designateDigDown(
    Position position
)
{
    if (
        !map_.inBounds(position)
        ||
        !map_.at(position).walkable()
        ||
        position.z <= 0
    )
    {
        return entt::null;
    }

    const Position below{
        position.x,
        position.y,
        position.z - 1
    };

    if (
        map_.at(below).shape !=
        TileShape::Wall
    )
    {
        return entt::null;
    }

    return createDesignation(
        position,
        DesignationType::DigDown,
        'v',
        TerminalColor::BrightMagenta
    );
}

entt::entity
Simulation::designateDigUp(
    Position position
)
{
    if (
        !map_.inBounds(position)
        ||
        !map_.at(position).walkable()
        ||
        position.z >=
            map_.depth() - 1
    )
    {
        return entt::null;
    }

    const Position above{
        position.x,
        position.y,
        position.z + 1
    };

    if (
        map_.at(above).shape !=
        TileShape::Wall
    )
    {
        return entt::null;
    }

    return createDesignation(
        position,
        DesignationType::DigUp,
        '^',
        TerminalColor::BrightMagenta
    );
}

entt::entity
Simulation::designateFellTree(
    Position position
)
{
    if (
        !map_.inBounds(position)
        ||
        map_.at(position).feature !=
            TileFeature::Tree
    )
    {
        return entt::null;
    }

    return createDesignation(
        position,
        DesignationType::FellTree,
        'F',
        TerminalColor::BrightYellow
    );
}

bool Simulation::cancelDesignationAt(
    Position position
)
{
    auto view =
        registry_.view<
            Designation,
            Position,
            DesignationLifecycle
        >();

    std::vector<entt::entity>
        matches;

    for (auto entity : view)
    {
        if (
            view.get<
                Position
            >(entity)
            ==
            position
        )
        {
            matches.push_back(
                entity
            );
        }
    }

    std::sort(
        matches.begin(),
        matches.end(),
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

    bool cancelled = false;

    for (auto entity : matches)
    {
        auto& lifecycle =
            registry_.get<
                DesignationLifecycle
            >(entity);

        if (
            lifecycle.state ==
                DesignationState::Ignored
            ||
            lifecycle.state ==
                DesignationState::Cancelled
        )
        {
            continue;
        }

        bool hasCancelableJob =
            lifecycle.state ==
            DesignationState::Active;

        for (Job& job : jobBoard_.jobs())
        {
            if (
                job.sourceDesignation !=
                entity
            )
            {
                continue;
            }

            if (
                job.state !=
                    JobState::Available
                &&
                job.state !=
                    JobState::Assigned
            )
            {
                continue;
            }

            hasCancelableJob =
                true;

            const entt::entity worker =
                job.worker;

            job.state =
                JobState::Cancelled;

            job.worker =
                entt::null;

            if (
                worker !=
                    entt::null
                &&
                registry_.valid(
                    worker
                )
            )
            {
                systems::clearWorkerJob(
                    registry_,
                    worker
                );
            }
        }

        if (!hasCancelableJob)
        {
            continue;
        }

        lifecycle.state =
            DesignationState::Cancelled;

        systems::hideDesignation(
            registry_,
            entity
        );

        cancelled =
            true;
    }

    return cancelled;
}

entt::entity
Simulation::createStockpile(
    Position min,
    Position max,
    std::vector<ItemType> accepts
)
{
    if (
        accepts.empty()
        ||
        min.x > max.x
        ||
        min.y > max.y
        ||
        min.z > max.z
        ||
        !map_.inBounds(min)
        ||
        !map_.inBounds(max)
    )
    {
        return entt::null;
    }

    const StockpileBounds requested{
        min,
        max
    };

    auto existing =
        registry_.view<
            Stockpile
        >();

    for (auto entity : existing)
    {
        if (
            stockpileBoundsOverlap(
                requested,
                existing.get<
                    Stockpile
                >(entity).bounds
            )
        )
        {
            return entt::null;
        }
    }

    for (
        int z = min.z;
        z <= max.z;
        ++z
    )
    {
        for (
            int y = min.y;
            y <= max.y;
            ++y
        )
        {
            for (
                int x = min.x;
                x <= max.x;
                ++x
            )
            {
                const Tile& tile =
                    map_.at(
                        x,
                        y,
                        z
                    );

                if (
                    !tile.baseWalkable()
                    ||
                    tile.feature !=
                        TileFeature::None
                    ||
                    tile.liquid.depth > 0
                )
                {
                    return entt::null;
                }
            }
        }
    }

    const auto entity =
        registry_.create();

    auto& stockpile =
        registry_.emplace<
            Stockpile
        >(entity);

    stockpile.bounds =
        requested;

    stockpile.accepts =
        std::move(accepts);

    return entity;
}

bool Simulation::configureStockpileAt(
    Position position,
    std::vector<ItemType> accepts
)
{
    if (accepts.empty())
    {
        return false;
    }

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

    std::sort(
        stockpiles.begin(),
        stockpiles.end(),
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

    for (auto entity : stockpiles)
    {
        auto& stockpile =
            registry_.get<
                Stockpile
            >(entity);

        if (
            stockpile.bounds.contains(
                position
            )
        )
        {
            stockpile.accepts =
                std::move(accepts);

            return true;
        }
    }

    return false;
}

std::uint64_t
Simulation::worldSeed() const
{
    return worldSeed_;
}

void Simulation::restoreRuntimeState(
    std::uint64_t tick,
    std::uint64_t rngState
)
{
    time_.tick =
        tick;

    random_.setState(
        rngState
    );

    accumulator_ =
        std::chrono::nanoseconds{0};
}

GameMap&
Simulation::map()
{
    return map_;
}

const GameMap&
Simulation::map() const
{
    return map_;
}

entt::registry&
Simulation::registry()
{
    return registry_;
}

const entt::registry&
Simulation::registry() const
{
    return registry_;
}

Random&
Simulation::random()
{
    return random_;
}

const Random&
Simulation::random() const
{
    return random_;
}

JobBoard&
Simulation::jobBoard()
{
    return jobBoard_;
}

const JobBoard&
Simulation::jobBoard() const
{
    return jobBoard_;
}

const SimulationTime&
Simulation::time() const
{
    return time_;
}

bool Simulation::hasOutstandingWork()
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

    auto view =
        registry_.view<
            Designation,
            DesignationLifecycle
        >();

    for (auto entity : view)
    {
        if (
            view.get<
                DesignationLifecycle
            >(entity).state ==
            DesignationState::Active
        )
        {
            return true;
        }
    }

    return false;
}

}

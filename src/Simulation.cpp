#include "ascii/Simulation.hpp"

#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Designations.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"

#include "ascii/systems/DesignationSystems.hpp"
#include "ascii/systems/LogisticsSystems.hpp"
#include "ascii/systems/MiningSystems.hpp"
#include "ascii/systems/MovementSystem.hpp"

#include <utility>

namespace ascii
{

Simulation::Simulation(
    int width,
    int height,
    std::uint64_t seed
)
    : map_(
        width,
        height
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
        std::chrono::
            nanoseconds::zero()
    )
    {
        return 0;
    }

    accumulator_ +=
        elapsed;

    int ticksExecuted =
        0;

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
    // ==================================================
    // Player intent
    // ==================================================

    systems::
        deduplicateDesignations(
            registry_
        );

    systems::
        createDesignationJobs(
            registry_,
            map_,
            jobBoard_
        );

    systems::
        generateHaulJobs(
            registry_,
            map_,
            pathfinder_,
            jobBoard_
        );

    // ==================================================
    // Job assignment
    // ==================================================

    systems::
        assignMiningJobs(
            registry_,
            map_,
            pathfinder_,
            jobBoard_
        );

    systems::
        assignHaulJobs(
            registry_,
            map_,
            pathfinder_,
            jobBoard_
        );

    // ==================================================
    // Movement
    // ==================================================

    systems::
        updateMovement(
            registry_,
            map_
        );

    // ==================================================
    // Work
    // ==================================================

    systems::
        executeMining(
            registry_,
            map_,
            jobBoard_,
            itemSpawnEvents_
        );

    systems::
        executeHauling(
            registry_,
            map_,
            pathfinder_,
            jobBoard_,
            itemPickupEvents_,
            itemDropEvents_
        );

    // ==================================================
    // Events
    // ==================================================

    systems::
        processItemSpawns(
            registry_,
            itemSpawnEvents_
        );

    systems::
        processItemPickups(
            registry_,
            map_,
            pathfinder_,
            jobBoard_,
            itemPickupEvents_
        );

    systems::
        processItemDrops(
            registry_,
            jobBoard_,
            itemDropEvents_
        );

    systems::
        generateHaulJobs(
            registry_,
            map_,
            pathfinder_,
            jobBoard_
        );

    ++time_.tick;
}

entt::entity
Simulation::designateMine(
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
        ).type !=
            TileType::Wall
    )
    {
        return entt::null;
    }

    const auto entity =
        registry_.create();

    registry_.emplace<
        MineDesignation
    >(entity);

    registry_.emplace<
        DesignationLifecycle
    >(entity);

    auto& entityPosition =
        registry_.emplace<
            Position
        >(entity);

    entityPosition =
        position;

    auto& glyph =
        registry_.emplace<
            Glyph
        >(entity);

    glyph.character =
        'X';

    glyph.color =
        TerminalColor::BrightRed;

    return entity;
}

entt::entity
Simulation::createStockpile(
    Position topLeft,
    Position bottomRight,
    std::vector<ItemType>
        accepts
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
        >(entity);

    stockpile.bounds.topLeft =
        topLeft;

    stockpile.bounds.bottomRight =
        bottomRight;

    stockpile.accepts =
        std::move(accepts);

    return entity;
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

GameMap& Simulation::map()
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

Random& Simulation::random()
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
        jobBoard_.
            hasUnfinished()
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
            MineDesignation,
            DesignationLifecycle
        >();

    for (auto entity : view)
    {
        const auto& lifecycle =
            view.get<
                DesignationLifecycle
            >(entity);

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

}

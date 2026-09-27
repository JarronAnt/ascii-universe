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
#include "ascii/systems/WaterSystem.hpp"

#include <utility>

namespace ascii
{

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
    std::chrono::nanoseconds
        elapsed
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

    systems::
        assignExcavationJobs(
            registry_,
            map_,
            pathfinder_,
            jobBoard_
        );

    systems::
        assignFellingJobs(
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

    systems::
        updateMovement(
            registry_,
            map_
        );

    systems::
        executeExcavation(
            registry_,
            map_,
            jobBoard_,
            itemSpawnEvents_
        );

    systems::
        executeFelling(
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

    // Water doesn't need to update as frequently
    // as worker AI.
    if (
        time_.tick % 2 == 0
    )
    {
        systems::
            updateWater(
                map_
            );
    }

    ++time_.tick;
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
        !map_.inBounds(position) ||
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
        !map_.inBounds(position) ||
        !map_.at(position).walkable() ||
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
        !map_.inBounds(position) ||
        !map_.at(position).walkable() ||
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
        !map_.inBounds(position) ||
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

entt::entity
Simulation::createStockpile(
    Position min,
    Position max,
    std::vector<ItemType>
        accepts
)
{
    if (
        min.x > max.x ||
        min.y > max.y ||
        min.z > max.z ||
        !map_.inBounds(min) ||
        !map_.inBounds(max)
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

    stockpile.bounds.min =
        min;

    stockpile.bounds.max =
        max;

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
        !itemSpawnEvents_.empty() ||
        !itemPickupEvents_.empty() ||
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

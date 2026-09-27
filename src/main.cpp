#include "ascii/Components.hpp"
#include "ascii/Pathfinder.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/TerminalRenderer.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>

using namespace ascii;

int main()
{
    // --------------------------------------------------
    // World seed
    // --------------------------------------------------

    constexpr std::uint64_t WorldSeed =
        123456789ULL;

    // --------------------------------------------------
    // Simulation
    // --------------------------------------------------

    Simulation simulation{
        40,
        20,
        WorldSeed
    };

    GameMap& map =
        simulation.map();

    entt::registry& registry =
        simulation.registry();

    // --------------------------------------------------
    // Outer walls
    // --------------------------------------------------

    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            if (
                x == 0 ||
                y == 0 ||
                x == map.width() - 1 ||
                y == map.height() - 1
            )
            {
                map.at(x, y).type =
                    TileType::Wall;
            }
        }
    }

    // --------------------------------------------------
    // Add an internal wall.
    //
    // The goblin MUST use A* to find the gap.
    // --------------------------------------------------

    constexpr int WallX = 20;
    constexpr int GapY = 10;

    for (
        int y = 1;
        y < map.height() - 1;
        ++y
    )
    {
        if (y == GapY)
        {
            continue;
        }

        map.at(
            WallX,
            y
        ).type = TileType::Wall;
    }

    // --------------------------------------------------
    // Deterministic start/goal
    // --------------------------------------------------
    //
    // These LOOK random, but the same WorldSeed always
    // gives the same result.

    const Position start{
        2,simulation.random().integer(2,map.height() - 3)
    };

    const Position goal{map.width() - 3,simulation.random().integer(2, map.height() - 3)
    };

    // --------------------------------------------------
    // Create goblin
    // --------------------------------------------------

    const auto goblin =
        registry.create();

    registry.emplace<Goblin>(
        goblin
    );

    registry.emplace<Name>(
        goblin,
        "Uru"
    );

    registry.emplace<Position>(
        goblin,
        start.x,
        start.y
    );

    registry.emplace<Glyph>(
        goblin,
        'g'
    );

    // --------------------------------------------------
    // Pathfinding
    // --------------------------------------------------

    Pathfinder pathfinder;

    auto path =
        pathfinder.findPath(
            map,
            start,
            goal
        );

    if (!path)
    {
        std::cerr
            << "No path found from ("
            << start.x
            << ", "
            << start.y
            << ") to ("
            << goal.x
            << ", "
            << goal.y
            << ")\n";

        return 1;
    }

    std::cout
        << "Path contains "
        << path->size()
        << " steps.\n";

    auto& movement =
        registry.emplace<MovementPath>(
            goblin
        );

    movement.nodes = std::move(*path);
    movement.nextStep = 0; 
    // --------------------------------------------------
    // Renderer
    // --------------------------------------------------

    TerminalRenderer renderer;

    renderer.render(
        map,
        registry
    );

    std::cout
        << "\nSeed: "
        << WorldSeed
        << '\n';

    std::cout
        << "Start: ("
        << start.x
        << ", "
        << start.y
        << ")\n";

    std::cout
        << "Goal:  ("
        << goal.x
        << ", "
        << goal.y
        << ")\n";

    // --------------------------------------------------
    // Fixed timestep game loop
    // --------------------------------------------------

    using Clock =
        std::chrono::steady_clock;

    auto previousTime =
        Clock::now();

    bool running = true;

    while (running)
    {
        const auto currentTime =
            Clock::now();

        const auto elapsed =
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(
                currentTime -
                previousTime
            );

        previousTime =
            currentTime;

        // Convert elapsed real time into zero or more
        // fixed simulation ticks.
        const int ticksExecuted =
            simulation.advance(
                elapsed
            );

        // Only redraw when simulation state changed.
        if (ticksExecuted > 0)
        {
            renderer.render(
                map,
                registry
            );

            const auto& position =
                registry.get<Position>(
                    goblin
                );

            const auto& movement =
                registry.get<MovementPath>(
                    goblin
                );

            std::cout
                << "\nSeed: "
                << WorldSeed
                << '\n';

            std::cout
                << "Simulation tick: "
                << simulation.time().tick
                << '\n';

            std::cout
                << "Goblin: ("
                << position.x
                << ", "
                << position.y
                << ")\n";

            std::cout
                << "Goal:   ("
                << goal.x
                << ", "
                << goal.y
                << ")\n";

            if (movement.finished())
            {
                running = false;
            }
        }

        // Don't burn an entire CPU core while we're
        // waiting for the next simulation tick.
        std::this_thread::sleep_for(
            std::chrono::milliseconds{1}
        );
    }

    std::cout
        << "\nGoblin reached destination.\n";

    return 0;
}

#include "ascii/Components.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/TerminalRenderer.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

using namespace ascii;

int main()
{
    constexpr std::uint64_t WorldSeed =
        123456789ULL;

    // --------------------------------------------------
    // Create simulation
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

    for (
        int y = 0;
        y < map.height();
        ++y
    )
    {
        for (
            int x = 0;
            x < map.width();
            ++x
        )
        {
            if (
                x == 0 ||
                y == 0 ||
                x ==
                    map.width() - 1 ||
                y ==
                    map.height() - 1
            )
            {
                map.at(
                    x,
                    y
                ).type =
                    TileType::Wall;
            }
        }
    }

    // --------------------------------------------------
    // Rock formation
    // --------------------------------------------------

    for (
        int y = 5;
        y <= 13;
        ++y
    )
    {
        for (
            int x = 24;
            x <= 30;
            ++x
        )
        {
            map.at(
                x,
                y
            ).type =
                TileType::Wall;
        }
    }

    // --------------------------------------------------
    // Miner goblin
    // --------------------------------------------------

    const auto miner =
        registry.create();

    registry.emplace<
        Goblin
    >(
        miner
    );

    registry.emplace<
        Miner
    >(
        miner
    );

    registry.emplace<
        Name
    >(
        miner,
        "Uru"
    );

    registry.emplace<
        Position
    >(
        miner,
        5,
        8
    );

    registry.emplace<
        Glyph
    >(
        miner,
        'g'
    );

    // --------------------------------------------------
    // Hauler goblin
    //
    // This goblin does NOT have Miner.
    //
    // It should therefore completely ignore mining
    // jobs.
    // --------------------------------------------------

    const auto hauler =
        registry.create();

    registry.emplace<
        Goblin
    >(
        hauler
    );

    registry.emplace<
        Hauler
    >(
        hauler
    );

    registry.emplace<
        Name
    >(
        hauler,
        "Kesh"
    );

    registry.emplace<
        Position
    >(
        hauler,
        5,
        12
    );

    registry.emplace<
        Glyph
    >(
        hauler,
        'g'
    );

    // --------------------------------------------------
    // Player mining commands
    // --------------------------------------------------

    simulation.designateMine(
        Position{24, 7}
    );

    simulation.designateMine(
        Position{24, 8}
    );

    simulation.designateMine(
        Position{24, 9}
    );

    simulation.designateMine(
        Position{24, 10}
    );

    // Deliberate duplicate.
    //
    // designationDedupSystem() should ignore this one.
    simulation.designateMine(
        Position{24, 8}
    );

    // --------------------------------------------------
    // Renderer
    // --------------------------------------------------

    TerminalRenderer renderer;

    renderer.render(
        map,
        registry
    );

    std::cout
        << "\nWorld seed: "
        << WorldSeed
        << '\n';

    std::cout
        << "Uru: Miner\n";

    std::cout
        << "Kesh: Hauler\n";

    // --------------------------------------------------
    // Fixed timestep loop
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
            std::chrono::
                duration_cast<
                    std::chrono::
                        nanoseconds
                >(
                    currentTime -
                    previousTime
                );

        previousTime =
            currentTime;

        const int ticksExecuted =
            simulation.advance(
                elapsed
            );

        if (
            ticksExecuted > 0
        )
        {
            renderer.render(
                map,
                registry
            );

            const JobBoard&
                jobs =
                    simulation.
                        jobBoard();

            std::cout
                << "\nTick: "
                << simulation.
                       time().tick
                << '\n';

            std::cout
                << "Jobs available: "
                << jobs.count(
                    JobState::
                        Available
                )
                << '\n';

            std::cout
                << "Jobs assigned:  "
                << jobs.count(
                    JobState::
                        Assigned
                )
                << '\n';

            std::cout
                << "Jobs complete:  "
                << jobs.count(
                    JobState::
                        Complete
                )
                << '\n';

            const auto&
                minerPosition =
                    registry.get<
                        Position
                    >(
                        miner
                    );

            const auto&
                haulerPosition =
                    registry.get<
                        Position
                    >(
                        hauler
                    );

            std::cout
                << "\nUru (Miner):  "
                << '('
                << minerPosition.x
                << ", "
                << minerPosition.y
                << ')'
                << '\n';

            std::cout
                << "Kesh (Hauler): "
                << '('
                << haulerPosition.x
                << ", "
                << haulerPosition.y
                << ')'
                << '\n';

            if (
                !simulation.
                    hasOutstandingWork()
            )
            {
                running = false;
            }
        }

        std::this_thread::
            sleep_for(
                std::chrono::
                    milliseconds{1}
            );
    }

    std::cout
        << "\nAll mining jobs complete.\n";

    return 0;
}

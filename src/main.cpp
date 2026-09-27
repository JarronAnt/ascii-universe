#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/TerminalRenderer.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using namespace ascii;

int main()
{
    constexpr std::uint64_t WorldSeed =
        123456789ULL;

    // ==================================================
    // Simulation
    // ==================================================

    Simulation simulation{
        40,
        20,
        WorldSeed
    };

    GameMap& map =
        simulation.map();

    entt::registry& registry =
        simulation.registry();

    // ==================================================
    // Outer walls
    // ==================================================

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
                x == 0
                ||
                y == 0
                ||
                x ==
                    map.width() - 1
                ||
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

    // ==================================================
    // Rock formation
    // ==================================================

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

    // ==================================================
    // Miner: Uru
    // ==================================================

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

    auto& minerName =
        registry.emplace<
            Name
        >(
            miner
        );

    minerName.value =
        "Uru";

    auto& minerPosition =
        registry.emplace<
            Position
        >(
            miner
        );

    minerPosition =
        Position{5, 8};

    auto& minerGlyph =
        registry.emplace<
            Glyph
        >(
            miner
        );

    minerGlyph.character =
        'g';

    // ==================================================
    // Hauler: Kesh
    // ==================================================

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

    auto& haulerName =
        registry.emplace<
            Name
        >(
            hauler
        );

    haulerName.value =
        "Kesh";

    auto& haulerPosition =
        registry.emplace<
            Position
        >(
            hauler
        );

    haulerPosition =
        Position{5, 12};

    auto& haulerGlyph =
        registry.emplace<
            Glyph
        >(
            hauler
        );

    haulerGlyph.character =
        'g';

    // ==================================================
    // Stone stockpile
    // ==================================================

    const auto stoneStockpile =
        simulation.createStockpile(
            Position{3, 2},
            Position{9, 5},
            std::vector<ItemType>{
                ItemType::Stone
            }
        );

    if (
        stoneStockpile ==
            entt::null
    )
    {
        std::cerr
            << "Failed to create stockpile.\n";

        return 1;
    }

    // ==================================================
    // Mining designations
    // ==================================================

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
    // Deduplication should ignore this.
    simulation.designateMine(
        Position{24, 8}
    );

    // ==================================================
    // Renderer
    // ==================================================

    TerminalRenderer renderer;

    // Hide terminal cursor while simulation runs.
    std::cout
        << "\033[?25l";

    auto renderStatus =
        [&]()
        {
            renderer.render(
                map,
                registry
            );

            const JobBoard& jobs =
                simulation.jobBoard();

            std::size_t groundItems =
                0;

            std::size_t carriedItems =
                0;

            std::size_t stockpiledItems =
                0;

            auto itemView =
                registry.view<
                    Item,
                    ItemState
                >();

            for (
                auto entity :
                itemView
            )
            {
                const auto& state =
                    itemView.get<
                        ItemState
                    >(
                        entity
                    );

                switch (
                    state.location
                )
                {
                    case ItemLocation::OnGround:
                        ++groundItems;
                        break;

                    case ItemLocation::Carried:
                        ++carriedItems;
                        break;

                    case ItemLocation::Stockpiled:
                        ++stockpiledItems;
                        break;
                }
            }

            const auto&
                currentMinerPosition =
                    registry.get<
                        Position
                    >(
                        miner
                    );

            const auto&
                currentHaulerPosition =
                    registry.get<
                        Position
                    >(
                        hauler
                    );

            const auto&
                stockpile =
                    registry.get<
                        Stockpile
                    >(
                        stoneStockpile
                    );

            std::cout
                << "\nWorld seed: "
                << WorldSeed
                << '\n';

            std::cout
                << "Tick: "
                << simulation.time().tick
                << "\n\n";

            std::cout
                << "Jobs available: "
                << jobs.count(
                    JobState::Available
                )
                << '\n';

            std::cout
                << "Jobs assigned:  "
                << jobs.count(
                    JobState::Assigned
                )
                << '\n';

            std::cout
                << "Jobs complete:  "
                << jobs.count(
                    JobState::Complete
                )
                << '\n';

            std::cout
                << "Jobs cancelled: "
                << jobs.count(
                    JobState::Cancelled
                )
                << "\n\n";

            std::cout
                << "Items on ground: "
                << groundItems
                << '\n';

            std::cout
                << "Items carried:   "
                << carriedItems
                << '\n';

            std::cout
                << "Items stored:    "
                << stockpiledItems
                << '\n';

            std::cout
                << "Reserved cells:  "
                << stockpile.
                       reservedCells.size()
                << "\n\n";

            std::cout
                << "Uru  (Miner):  ("
                << currentMinerPosition.x
                << ", "
                << currentMinerPosition.y
                << ')'
                << '\n';

            std::cout
                << "Kesh (Hauler): ("
                << currentHaulerPosition.x
                << ", "
                << currentHaulerPosition.y
                << ')'
                << '\n';

            if (
                registry.all_of<
                    CarryingItem
                >(
                    hauler
                )
            )
            {
                std::cout
                    << "Kesh is carrying stone.\n";
            }
            else
            {
                std::cout
                    << "Kesh is not carrying anything.\n";
            }

            std::cout.flush();
        };

    renderStatus();

    // ==================================================
    // Fixed timestep loop
    // ==================================================

    using Clock =
        std::chrono::steady_clock;

    auto previousTime =
        Clock::now();

    bool running =
        true;

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
            renderStatus();

            if (
                !simulation.
                    hasOutstandingWork()
            )
            {
                running =
                    false;
            }
        }

        std::this_thread::
            sleep_for(
                std::chrono::
                    milliseconds{1}
            );
    }

    // Restore terminal cursor.
    std::cout
        << "\033[?25h";

    renderStatus();

    std::cout
        << "\n\nAll mining and hauling jobs complete.\n";

    return 0;
}

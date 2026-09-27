#include "ascii/Color.hpp"
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
#include <string>
#include <thread>
#include <vector>

using namespace ascii;

int main()
{
    constexpr std::uint64_t
        WorldSeed =
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
    // Uru — Miner
    // ==================================================

    const auto miner =
        registry.create();

    registry.emplace<
        Goblin
    >(miner);

    registry.emplace<
        Miner
    >(miner);

    auto& minerName =
        registry.emplace<
            Name
        >(miner);

    minerName.value =
        "Uru";

    auto& minerPosition =
        registry.emplace<
            Position
        >(miner);

    minerPosition =
        Position{
            5,
            8
        };

    auto& minerGlyph =
        registry.emplace<
            Glyph
        >(miner);

    minerGlyph.character =
        'g';

    minerGlyph.color =
        TerminalColor::
            BrightGreen;

    // ==================================================
    // Kesh — Hauler
    // ==================================================

    const auto hauler =
        registry.create();

    registry.emplace<
        Goblin
    >(hauler);

    registry.emplace<
        Hauler
    >(hauler);

    auto& haulerName =
        registry.emplace<
            Name
        >(hauler);

    haulerName.value =
        "Kesh";

    auto& haulerPosition =
        registry.emplace<
            Position
        >(hauler);

    haulerPosition =
        Position{
            5,
            12
        };

    auto& haulerGlyph =
        registry.emplace<
            Glyph
        >(hauler);

    haulerGlyph.character =
        'g';

    haulerGlyph.color =
        TerminalColor::
            BrightYellow;

    // ==================================================
    // Stone stockpile
    // ==================================================

    const auto stoneStockpile =
        simulation.createStockpile(
            Position{
                3,
                2
            },

            Position{
                9,
                5
            },

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
            << "Failed to create "
               "stockpile.\n";

        return 1;
    }

    // ==================================================
    // Mining designations
    // ==================================================

    simulation.designateMine(
        Position{
            24,
            7
        }
    );

    simulation.designateMine(
        Position{
            24,
            8
        }
    );

    simulation.designateMine(
        Position{
            24,
            9
        }
    );

    simulation.designateMine(
        Position{
            24,
            10
        }
    );

    // Deliberate duplicate.
    simulation.designateMine(
        Position{
            24,
            8
        }
    );

    // ==================================================
    // Renderer
    // ==================================================

    TerminalRenderer renderer;

    const auto buildHud =
        [&](
            bool completed
        )
        {
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
                    >(entity);

                switch (
                    state.location
                )
                {
                    case ItemLocation::
                        OnGround:
                        ++groundItems;
                        break;

                    case ItemLocation::
                        Carried:
                        ++carriedItems;
                        break;

                    case ItemLocation::
                        Stockpiled:
                        ++stockpiledItems;
                        break;
                }
            }

            const JobBoard& jobs =
                simulation.jobBoard();

            const auto&
                currentMinerPosition =
                    registry.get<
                        Position
                    >(miner);

            const auto&
                currentHaulerPosition =
                    registry.get<
                        Position
                    >(hauler);

            const auto& stockpile =
                registry.get<
                    Stockpile
                >(
                    stoneStockpile
                );

            std::vector<std::string>
                lines;

            lines.push_back(
                "ASCII Universe | Tick "
                +
                std::to_string(
                    simulation.
                        time().tick
                )
            );

            lines.push_back(
                "Seed: "
                +
                std::to_string(
                    WorldSeed
                )
            );

            lines.push_back(
                "Jobs A:"
                +
                std::to_string(
                    jobs.count(
                        JobState::
                            Available
                    )
                )
                +
                " X:"
                +
                std::to_string(
                    jobs.count(
                        JobState::
                            Assigned
                    )
                )
                +
                " C:"
                +
                std::to_string(
                    jobs.count(
                        JobState::
                            Complete
                    )
                )
            );

            lines.push_back(
                "Items ground:"
                +
                std::to_string(
                    groundItems
                )
                +
                " carried:"
                +
                std::to_string(
                    carriedItems
                )
                +
                " stored:"
                +
                std::to_string(
                    stockpiledItems
                )
            );

            lines.push_back(
                "Stockpile reserved: "
                +
                std::to_string(
                    stockpile.
                        reservedCells.
                        size()
                )
            );

            lines.push_back(
                "Uru  Miner  ("
                +
                std::to_string(
                    currentMinerPosition.x
                )
                +
                ","
                +
                std::to_string(
                    currentMinerPosition.y
                )
                +
                ")"
            );

            lines.push_back(
                "Kesh Hauler ("
                +
                std::to_string(
                    currentHaulerPosition.x
                )
                +
                ","
                +
                std::to_string(
                    currentHaulerPosition.y
                )
                +
                ")"
            );

            if (
                registry.all_of<
                    CarryingItem
                >(hauler)
            )
            {
                lines.push_back(
                    "Kesh: carrying stone"
                );
            }
            else
            {
                lines.push_back(
                    "Kesh: empty handed"
                );
            }

            lines.push_back(
                completed
                    ?
                    "Status: COMPLETE"
                    :
                    "Status: RUNNING"
            );

            return lines;
        };

    renderer.render(
        map,
        registry,
        buildHud(false)
    );

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
            const bool complete =
                !simulation.
                    hasOutstandingWork();

            renderer.render(
                map,
                registry,
                buildHud(
                    complete
                )
            );

            if (complete)
            {
                running =
                    false;
            }
        }

        std::this_thread::
            sleep_for(
                std::chrono::
                    milliseconds{
                        1
                    }
            );
    }

    renderer.finish();

    return 0;
}

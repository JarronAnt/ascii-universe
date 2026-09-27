#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/SaveManager.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/TerminalRenderer.hpp"
#include "ascii/WorldGenerator.hpp"

#include <entt/entt.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace ascii;

namespace
{

struct Options
{
    std::optional<
        std::filesystem::path
    > loadPath;

    std::filesystem::path
        savePath{
            "saves/autosave.json"
        };

    bool savePathExplicit{
        false
    };

    std::optional<
        std::uint64_t
    > seed;

    std::optional<
        std::uint64_t
    > stopAfterTicks;
};

void printHelp()
{
    std::cout
        << "ASCII Universe\n\n"

        << "Usage:\n"

        << "  ./ascii_universe\n"

        << "  ./ascii_universe "
           "--seed 12345\n"

        << "  ./ascii_universe "
           "--load saves/autosave.json\n"

        << "  ./ascii_universe "
           "--seed 12345 "
           "--save saves/test.json\n"

        << "  ./ascii_universe "
           "--seed 12345 "
           "--save saves/test.json "
           "--stop-after 25\n";
}

Options parseArguments(
    int argc,
    char** argv
)
{
    Options options;

    for (
        int i = 1;
        i < argc;
        ++i
    )
    {
        const std::string
            argument =
                argv[i];

        if (
            argument ==
            "--help"
        )
        {
            printHelp();

            std::exit(0);
        }

        if (
            argument ==
            "--seed"
        )
        {
            if (
                i + 1 >= argc
            )
            {
                throw std::runtime_error(
                    "--seed requires a number."
                );
            }

            options.seed =
                std::stoull(
                    argv[++i]
                );

            continue;
        }

        if (
            argument ==
            "--load"
        )
        {
            if (
                i + 1 >= argc
            )
            {
                throw std::runtime_error(
                    "--load requires a path."
                );
            }

            options.loadPath =
                std::filesystem::path{
                    argv[++i]
                };

            continue;
        }

        if (
            argument ==
            "--save"
        )
        {
            if (
                i + 1 >= argc
            )
            {
                throw std::runtime_error(
                    "--save requires a path."
                );
            }

            options.savePath =
                std::filesystem::path{
                    argv[++i]
                };

            options.
                savePathExplicit =
                    true;

            continue;
        }

        if (
            argument ==
            "--stop-after"
        )
        {
            if (
                i + 1 >= argc
            )
            {
                throw std::runtime_error(
                    "--stop-after requires "
                    "a tick count."
                );
            }

            options.stopAfterTicks =
                std::stoull(
                    argv[++i]
                );

            continue;
        }

        throw std::runtime_error(
            "Unknown argument: "
            +
            argument
        );
    }

    if (
        options.loadPath
        &&
        !options.savePathExplicit
    )
    {
        options.savePath =
            *options.loadPath;
    }

    return options;
}

std::uint64_t
generateNewSeed()
{
    const auto now =
        std::chrono::
            high_resolution_clock::
            now().
            time_since_epoch().
            count();

    return static_cast<
        std::uint64_t
    >(now);
}

entt::entity
findGoblinByName(
    entt::registry& registry,
    std::string_view name
)
{
    auto view =
        registry.view<
            Goblin,
            Name
        >();

    for (auto entity : view)
    {
        if (
            view.get<
                Name
            >(entity).value
            ==
            name
        )
        {
            return entity;
        }
    }

    return entt::null;
}

entt::entity
findFirstStockpile(
    entt::registry& registry
)
{
    auto view =
        registry.view<
            Stockpile
        >();

    for (auto entity : view)
    {
        return entity;
    }

    return entt::null;
}

std::unique_ptr<Simulation>
createNewWorld(
    std::uint64_t seed
)
{
    constexpr int
        WorldWidth =
            50;

    constexpr int
        WorldHeight =
            24;

    auto simulation =
        std::make_unique<
            Simulation
        >(
            WorldWidth,
            WorldHeight,
            seed
        );

    auto& registry =
        simulation->
            registry();

    const auto layout =
        WorldGenerator::generate(
            simulation->map(),
            seed
        );

    // ==============================================
    // Miner
    // ==============================================

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
        layout.minerSpawn;

    auto& minerGlyph =
        registry.emplace<
            Glyph
        >(miner);

    minerGlyph.character =
        'g';

    minerGlyph.color =
        TerminalColor::
            BrightGreen;

    // ==============================================
    // Hauler
    // ==============================================

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
        layout.haulerSpawn;

    auto& haulerGlyph =
        registry.emplace<
            Glyph
        >(hauler);

    haulerGlyph.character =
        'g';

    haulerGlyph.color =
        TerminalColor::
            BrightYellow;

    // ==============================================
    // Stone stockpile
    // ==============================================

    simulation->
        createStockpile(
            layout.
                stockpileTopLeft,

            layout.
                stockpileBottomRight,

            std::vector<ItemType>{
                ItemType::Stone
            }
        );

    // ==============================================
    // Initial mining work
    // ==============================================

    for (
        const auto target :
        layout.miningTargets
    )
    {
        simulation->
            designateMine(
                target
            );
    }

    return simulation;
}

}

int main(
    int argc,
    char** argv
)
{
    try
    {
        const Options options =
            parseArguments(
                argc,
                argv
            );

        std::unique_ptr<
            Simulation
        > simulation;

        bool loaded =
            false;

        if (
            options.loadPath
        )
        {
            simulation =
                SaveManager::load(
                    *options.loadPath
                );

            loaded =
                true;
        }
        else
        {
            const std::uint64_t seed =
                options.seed.
                    value_or(
                        generateNewSeed()
                    );

            simulation =
                createNewWorld(
                    seed
                );
        }

        auto& map =
            simulation->map();

        auto& registry =
            simulation->registry();

        const auto miner =
            findGoblinByName(
                registry,
                "Uru"
            );

        const auto hauler =
            findGoblinByName(
                registry,
                "Kesh"
            );

        const auto stockpileEntity =
            findFirstStockpile(
                registry
            );

        if (
            miner == entt::null
            ||
            hauler == entt::null
            ||
            stockpileEntity ==
                entt::null
        )
        {
            throw std::runtime_error(
                "World is missing expected "
                "demo entities."
            );
        }

        TerminalRenderer renderer;

        const auto buildHud =
            [&](
                bool completed
            )
            {
                std::size_t
                    groundItems =
                        0;

                std::size_t
                    carriedItems =
                        0;

                std::size_t
                    stockpiledItems =
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
                        case
                            ItemLocation::
                                OnGround:

                            ++groundItems;
                            break;

                        case
                            ItemLocation::
                                Carried:

                            ++carriedItems;
                            break;

                        case
                            ItemLocation::
                                Stockpiled:

                            ++stockpiledItems;
                            break;
                    }
                }

                const auto& jobs =
                    simulation->
                        jobBoard();

                const auto& minerPos =
                    registry.get<
                        Position
                    >(miner);

                const auto& haulerPos =
                    registry.get<
                        Position
                    >(hauler);

                const auto& stockpile =
                    registry.get<
                        Stockpile
                    >(
                        stockpileEntity
                    );

                std::vector<
                    std::string
                > lines;

                lines.push_back(
                    "ASCII Universe | Tick "
                    +
                    std::to_string(
                        simulation->
                            time().tick
                    )
                );

                lines.push_back(
                    std::string{
                        loaded
                        ?
                        "Loaded world"
                        :
                        "Generated world"
                    }
                    +
                    " | Seed "
                    +
                    std::to_string(
                        simulation->
                            worldSeed()
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
                    "Reserved cells: "
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
                        minerPos.x
                    )
                    +
                    ","
                    +
                    std::to_string(
                        minerPos.y
                    )
                    +
                    ")"
                );

                lines.push_back(
                    "Kesh Hauler ("
                    +
                    std::to_string(
                        haulerPos.x
                    )
                    +
                    ","
                    +
                    std::to_string(
                        haulerPos.y
                    )
                    +
                    ")"
                );

                lines.push_back(
                    "Save: "
                    +
                    options.
                        savePath.
                        string()
                );

                lines.push_back(
                    completed
                    ?
                    "Status: COMPLETE"
                    :
                    "Status: RUNNING"
                );

                return lines;
            };

        bool complete =
            !simulation->
                hasOutstandingWork();

        renderer.render(
            map,
            registry,
            buildHud(
                complete
            )
        );

        using Clock =
            std::chrono::
                steady_clock;

        auto previousTime =
            Clock::now();

        const std::uint64_t
            startingTick =
                simulation->
                    time().tick;

        std::uint64_t
            lastAutosaveTick =
                startingTick;

        bool stoppedEarly =
            false;

        while (!complete)
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
                simulation->
                    advance(
                        elapsed
                    );

            if (
                ticksExecuted > 0
            )
            {
                complete =
                    !simulation->
                        hasOutstandingWork();

                renderer.render(
                    map,
                    registry,
                    buildHud(
                        complete
                    )
                );

                // Autosave every 25 simulation ticks.
                if (
                    simulation->
                        time().tick
                    >=
                    lastAutosaveTick + 25
                )
                {
                    SaveManager::save(
                        *simulation,
                        options.savePath
                    );

                    lastAutosaveTick =
                        simulation->
                            time().tick;
                }

                if (
                    options.
                        stopAfterTicks
                    &&
                    simulation->
                        time().tick
                    >=
                    startingTick
                    +
                    *options.
                        stopAfterTicks
                )
                {
                    SaveManager::save(
                        *simulation,
                        options.savePath
                    );

                    stoppedEarly =
                        true;

                    break;
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

        // Always save final/current state.
        SaveManager::save(
            *simulation,
            options.savePath
        );

        renderer.render(
            map,
            registry,
            buildHud(
                complete
            )
        );

        renderer.finish();

        if (stoppedEarly)
        {
            std::cout
                << "Saved after "
                << *options.
                    stopAfterTicks
                << " additional ticks.\n";
        }
        else
        {
            std::cout
                << "World saved to "
                << options.
                    savePath.
                    string()
                << '\n';
        }

        return 0;
    }
    catch (
        const std::exception& error
    )
    {
        std::cerr
            << "Fatal error: "
            << error.what()
            << '\n';

        return 1;
    }
}

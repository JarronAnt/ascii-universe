#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Material.hpp"
#include "ascii/SaveManager.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/TerminalInput.hpp"
#include "ascii/TerminalRenderer.hpp"
#include "ascii/WorldGenerator.hpp"

#include <entt/entt.hpp>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace ascii;

namespace
{

// ==================================================
// Signal handling
//
// We do NOT perform saving or terminal cleanup from
// inside the signal handler.
//
// The signal handler only sets a flag.
//
// The normal game loop sees that flag and exits
// cleanly, allowing:
//   - SaveManager::save()
//   - TerminalRenderer::finish()
//   - TerminalInput destructor
//
// to run normally.
// ==================================================

volatile std::sig_atomic_t interrupted =
    0;

void handleSignal(
    int
)
{
    interrupted =
        1;
}

// ==================================================
// Command-line options
// ==================================================

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

// ==================================================
// Help
// ==================================================

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
           "--save saves/world.json\n"

        << "  ./ascii_universe "
           "--seed 12345 "
           "--stop-after 100\n\n"

        << "Controls:\n"

        << "  [ or ,   View lower Z-level\n"

        << "  ] or .   View higher Z-level\n"

        << "  SPACE    Pause/unpause\n"

        << "  s        Save\n"

        << "  q        Save and quit\n"

        << "  Ctrl+C   Save and quit cleanly\n";
}

// ==================================================
// Parse command-line arguments
// ==================================================

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
        const std::string argument =
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

            options.savePathExplicit =
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
                    "--stop-after requires a number."
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

    // If loading a save and the user did not provide
    // a separate save path, continue saving back into
    // the same file.
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

// ==================================================
// Generate new world seed
// ==================================================

std::uint64_t generateSeed()
{
    return
        static_cast<
            std::uint64_t
        >(
            std::chrono::
                high_resolution_clock::
                now().
                time_since_epoch().
                count()
        );
}

// ==================================================
// Goblin creation helper
// ==================================================

void createGoblin(
    entt::registry& registry,
    const std::string& name,
    Position position,
    TerminalColor color,
    bool miner,
    bool hauler,
    bool woodcutter
)
{
    const auto entity =
        registry.create();

    registry.emplace<
        Goblin
    >(entity);

    registry.emplace<
        Name
    >(entity).value =
        name;

    registry.emplace<
        Position
    >(entity) =
        position;

    auto& glyph =
        registry.emplace<
            Glyph
        >(entity);

    glyph.character =
        'g';

    glyph.color =
        color;

    if (miner)
    {
        registry.emplace<
            Miner
        >(entity);
    }

    if (hauler)
    {
        registry.emplace<
            Hauler
        >(entity);
    }

    if (woodcutter)
    {
        registry.emplace<
            Woodcutter
        >(entity);
    }
}

// ==================================================
// New world creation
// ==================================================

std::unique_ptr<Simulation>
createNewWorld(
    std::uint64_t seed,
    int& initialViewZ
)
{
    constexpr int WorldWidth =
        60;

    constexpr int WorldHeight =
        28;

    constexpr int WorldDepth =
        12;

    auto simulation =
        std::make_unique<
            Simulation
        >(
            WorldWidth,
            WorldHeight,
            WorldDepth,
            seed
        );

    const auto layout =
        WorldGenerator::generate(
            simulation->map(),
            seed
        );

    initialViewZ =
        layout.defaultViewZ;

    auto& registry =
        simulation->registry();

    // ==================================================
    // Uru
    //
    // Dedicated underground miner.
    // ==================================================

    createGoblin(
        registry,
        "Uru",
        layout.minerSpawn,
        TerminalColor::BrightGreen,
        true,
        false,
        false
    );

    // ==================================================
    // Kesh
    //
    // Dedicated hauler.
    //
    // Can travel between Z-levels using stairs/ramps.
    // ==================================================

    createGoblin(
        registry,
        "Kesh",
        layout.haulerSpawn,
        TerminalColor::BrightYellow,
        false,
        true,
        false
    );

    // ==================================================
    // Brakka
    //
    // Dedicated surface woodcutter.
    // ==================================================

    createGoblin(
        registry,
        "Brakka",
        layout.woodcutterSpawn,
        TerminalColor::BrightCyan,
        false,
        false,
        true
    );

    // ==================================================
    // Surface stockpile
    //
    // Currently accepts all generated resource types.
    // ==================================================

    const auto stockpile =
        simulation->createStockpile(
            layout.stockpileMin,
            layout.stockpileMax,
            {
                ItemType::Stone,
                ItemType::Ore,
                ItemType::Soil,
                ItemType::Log
            }
        );

    if (
        stockpile ==
        entt::null
    )
    {
        throw std::runtime_error(
            "Failed to create starting stockpile."
        );
    }

    // ==================================================
    // Starting mining jobs
    // ==================================================

    for (
        const auto target :
        layout.miningTargets
    )
    {
        simulation->designateMine(
            target
        );
    }

    // ==================================================
    // Starting tree-felling jobs
    // ==================================================

    for (
        const auto target :
        layout.treeTargets
    )
    {
        simulation->
            designateFellTree(
                target
            );
    }

    // ==================================================
    // Starting vertical excavation tests
    // ==================================================

    simulation->designateDigDown(
        layout.digDownTarget
    );

    simulation->designateDigUp(
        layout.digUpTarget
    );

    return simulation;
}

// ==================================================
// Choose sensible Z-level after loading
// ==================================================

int defaultLoadedViewZ(
    entt::registry& registry,
    const GameMap& map
)
{
    // Prefer stockpile Z-level because that normally
    // corresponds to the fortress/surface home level.
    auto stockpiles =
        registry.view<
            Stockpile
        >();

    for (
        auto entity :
        stockpiles
    )
    {
        const auto& stockpile =
            stockpiles.get<
                Stockpile
            >(entity);

        return std::clamp(
            stockpile.bounds.min.z,
            0,
            map.depth() - 1
        );
    }

    // Otherwise show the highest occupied goblin level.
    auto goblins =
        registry.view<
            Goblin,
            Position
        >();

    int highest =
        0;

    for (
        auto entity :
        goblins
    )
    {
        highest =
            std::max(
                highest,
                goblins.get<
                    Position
                >(entity).z
            );
    }

    return std::clamp(
        highest,
        0,
        map.depth() - 1
    );
}

// ==================================================
// Count visible water tiles
// ==================================================

std::size_t countWaterCells(
    const GameMap& map,
    int z
)
{
    std::size_t result =
        0;

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
                map.at(
                    x,
                    y,
                    z
                ).liquid.depth > 0
            )
            {
                ++result;
            }
        }
    }

    return result;
}

// ==================================================
// Count resources by type
// ==================================================

std::size_t countItemType(
    entt::registry& registry,
    ItemType type
)
{
    std::size_t result =
        0;

    auto view =
        registry.view<
            Item
        >();

    for (
        auto entity :
        view
    )
    {
        if (
            view.get<
                Item
            >(entity).type ==
            type
        )
        {
            ++result;
        }
    }

    return result;
}

}

int main(
    int argc,
    char** argv
)
{
    try
    {
        // ==================================================
        // Signals
        // ==================================================

        std::signal(
            SIGINT,
            handleSignal
        );

        std::signal(
            SIGTERM,
            handleSignal
        );

        // ==================================================
        // Arguments
        // ==================================================

        const Options options =
            parseArguments(
                argc,
                argv
            );

        // ==================================================
        // Create/load simulation
        // ==================================================

        std::unique_ptr<
            Simulation
        > simulation;

        bool loaded =
            false;

        int viewZ =
            0;

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

            viewZ =
                defaultLoadedViewZ(
                    simulation->registry(),
                    simulation->map()
                );
        }
        else
        {
            const std::uint64_t seed =
                options.seed.
                    value_or(
                        generateSeed()
                    );

            simulation =
                createNewWorld(
                    seed,
                    viewZ
                );
        }

        auto& map =
            simulation->map();

        auto& registry =
            simulation->registry();

        // ==================================================
        // Terminal
        // ==================================================

        TerminalRenderer renderer;

        TerminalInput input;

        // ==================================================
        // Runtime state
        // ==================================================

        bool paused =
            false;

        bool running =
            true;

        std::string lastMessage =
            loaded
            ?
            "Loaded save."
            :
            "Generated new world.";

        // ==================================================
        // HUD builder
        // ==================================================

        const auto buildHud =
            [&]()
            {
                const JobBoard& jobs =
                    simulation->
                        jobBoard();

                std::size_t onGround =
                    0;

                std::size_t carried =
                    0;

                std::size_t stored =
                    0;

                auto itemStates =
                    registry.view<
                        Item,
                        ItemState
                    >();

                for (
                    auto entity :
                    itemStates
                )
                {
                    const auto& state =
                        itemStates.get<
                            ItemState
                        >(entity);

                    switch (
                        state.location
                    )
                    {
                        case ItemLocation::
                            OnGround:

                            ++onGround;
                            break;

                        case ItemLocation::
                            Carried:

                            ++carried;
                            break;

                        case ItemLocation::
                            Stockpiled:

                            ++stored;
                            break;
                    }
                }

                std::vector<
                    std::string
                > lines;

                // ------------------------------------------
                // Simulation header
                // ------------------------------------------

                lines.push_back(
                    "ASCII Universe | Tick "
                    +
                    std::to_string(
                        simulation->
                            time().tick
                    )
                    +
                    " | Z "
                    +
                    std::to_string(
                        viewZ
                    )
                    +
                    "/"
                    +
                    std::to_string(
                        map.depth() - 1
                    )
                );

                lines.push_back(
                    std::string{
                        paused
                        ?
                        "PAUSED"
                        :
                        (
                            simulation->
                                hasOutstandingWork()
                            ?
                            "WORKING"
                            :
                            "IDLE"
                        )
                    }
                    +
                    " | Seed "
                    +
                    std::to_string(
                        simulation->
                            worldSeed()
                    )
                );

                // ------------------------------------------
                // Jobs
                // ------------------------------------------

                lines.push_back(
                    "Jobs | available:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::
                                Available
                        )
                    )
                    +
                    " assigned:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::
                                Assigned
                        )
                    )
                    +
                    " complete:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::
                                Complete
                        )
                    )
                    +
                    " cancelled:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::
                                Cancelled
                        )
                    )
                );

                // ------------------------------------------
                // Items
                // ------------------------------------------

                lines.push_back(
                    "Items | ground:"
                    +
                    std::to_string(
                        onGround
                    )
                    +
                    " carried:"
                    +
                    std::to_string(
                        carried
                    )
                    +
                    " stored:"
                    +
                    std::to_string(
                        stored
                    )
                );

                lines.push_back(
                    "Resources | stone:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Stone
                        )
                    )
                    +
                    " ore:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Ore
                        )
                    )
                    +
                    " soil:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Soil
                        )
                    )
                    +
                    " logs:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Log
                        )
                    )
                );

                // ------------------------------------------
                // Water
                // ------------------------------------------

                lines.push_back(
                    "Water cells on current Z: "
                    +
                    std::to_string(
                        countWaterCells(
                            map,
                            viewZ
                        )
                    )
                );

                // ------------------------------------------
                // Goblins
                // ------------------------------------------

                auto goblins =
                    registry.view<
                        Goblin,
                        Name,
                        Position
                    >();

                for (
                    auto entity :
                    goblins
                )
                {
                    const auto& name =
                        goblins.get<
                            Name
                        >(entity).value;

                    const auto& position =
                        goblins.get<
                            Position
                        >(entity);

                    std::string profession =
                        "Goblin";

                    if (
                        registry.all_of<
                            Miner
                        >(entity)
                    )
                    {
                        profession =
                            "Miner";
                    }
                    else if (
                        registry.all_of<
                            Hauler
                        >(entity)
                    )
                    {
                        profession =
                            "Hauler";
                    }
                    else if (
                        registry.all_of<
                            Woodcutter
                        >(entity)
                    )
                    {
                        profession =
                            "Woodcutter";
                    }

                    lines.push_back(
                        name
                        +
                        " | "
                        +
                        profession
                        +
                        " | ("
                        +
                        std::to_string(
                            position.x
                        )
                        +
                        ","
                        +
                        std::to_string(
                            position.y
                        )
                        +
                        ","
                        +
                        std::to_string(
                            position.z
                        )
                        +
                        ")"
                    );
                }

                // ------------------------------------------
                // Legend
                // ------------------------------------------

                lines.push_back(
                    "Legend: # rock | . floor | T tree | 1-7 water | < up | > down | X up/down | ^ ramp"
                );

                // ------------------------------------------
                // Controls
                // ------------------------------------------

                lines.push_back(
                    "[/, lower Z | ]/. higher Z | SPACE pause | s save | q save+quit"
                );

                // ------------------------------------------
                // Last status message
                // ------------------------------------------

                lines.push_back(
                    lastMessage
                );

                return lines;
            };

        // ==================================================
        // Initial frame
        // ==================================================

        renderer.render(
            map,
            registry,
            viewZ,
            buildHud()
        );

        // ==================================================
        // Timing
        // ==================================================

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
                simulation->
                    time().tick;

        // ==================================================
        // Main loop
        // ==================================================

        while (
            running
            &&
            !interrupted
        )
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

            bool redraw =
                false;

            // ==============================================
            // Input
            // ==============================================

            while (true)
            {
                const auto key =
                    input.poll();

                if (!key)
                {
                    break;
                }

                switch (*key)
                {
                    // --------------------------------------
                    // Lower Z
                    // --------------------------------------

                    case '[':
                    case ',':
                    {
                        const int next =
                            std::max(
                                0,
                                viewZ - 1
                            );

                        if (
                            next != viewZ
                        )
                        {
                            viewZ =
                                next;

                            lastMessage =
                                "Viewing Z "
                                +
                                std::to_string(
                                    viewZ
                                );

                            redraw =
                                true;
                        }

                        break;
                    }

                    // --------------------------------------
                    // Higher Z
                    // --------------------------------------

                    case ']':
                    case '.':
                    {
                        const int next =
                            std::min(
                                map.depth() - 1,
                                viewZ + 1
                            );

                        if (
                            next != viewZ
                        )
                        {
                            viewZ =
                                next;

                            lastMessage =
                                "Viewing Z "
                                +
                                std::to_string(
                                    viewZ
                                );

                            redraw =
                                true;
                        }

                        break;
                    }

                    // --------------------------------------
                    // Pause
                    // --------------------------------------

                    case ' ':
                    {
                        paused =
                            !paused;

                        lastMessage =
                            paused
                            ?
                            "Simulation paused."
                            :
                            "Simulation resumed.";

                        // Important:
                        //
                        // Reset timing when unpausing so the
                        // game doesn't try to simulate all
                        // the real-world time spent paused.
                        previousTime =
                            Clock::now();

                        redraw =
                            true;

                        break;
                    }

                    // --------------------------------------
                    // Manual save
                    // --------------------------------------

                    case 's':
                    {
                        SaveManager::save(
                            *simulation,
                            options.savePath
                        );

                        lastMessage =
                            "Saved to "
                            +
                            options.
                                savePath.
                                string();

                        redraw =
                            true;

                        break;
                    }

                    // --------------------------------------
                    // Save + quit
                    // --------------------------------------

                    case 'q':
                    {
                        SaveManager::save(
                            *simulation,
                            options.savePath
                        );

                        lastMessage =
                            "Saved. Exiting.";

                        running =
                            false;

                        break;
                    }

                    default:
                        break;
                }
            }

            if (!running)
            {
                break;
            }

            // ==============================================
            // Simulation
            // ==============================================

            if (!paused)
            {
                const int ticksExecuted =
                    simulation->advance(
                        elapsed
                    );

                if (
                    ticksExecuted > 0
                )
                {
                    redraw =
                        true;
                }

                // ------------------------------------------
                // Autosave
                // ------------------------------------------

                if (
                    simulation->
                        time().tick
                    >=
                    lastAutosaveTick +
                        100
                )
                {
                    SaveManager::save(
                        *simulation,
                        options.savePath
                    );

                    lastAutosaveTick =
                        simulation->
                            time().tick;

                    lastMessage =
                        "Autosaved.";

                    redraw =
                        true;
                }

                // ------------------------------------------
                // Debug/testing stop-after
                // ------------------------------------------

                if (
                    options.stopAfterTicks
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

                    lastMessage =
                        "Reached --stop-after limit.";

                    running =
                        false;

                    break;
                }
            }
            else
            {
                // While paused, continually reset the time
                // origin so unpause does not cause massive
                // fixed-step catch-up.
                previousTime =
                    Clock::now();
            }

            // ==============================================
            // Render current Z slice
            // ==============================================

            if (redraw)
            {
                renderer.render(
                    map,
                    registry,
                    viewZ,
                    buildHud()
                );
            }

            // Avoid busy-looping the CPU.
            std::this_thread::
                sleep_for(
                    std::chrono::
                        milliseconds{
                            1
                        }
                );
        }

        // ==================================================
        // Graceful shutdown
        // ==================================================

        if (interrupted)
        {
            lastMessage =
                "Interrupt received. Saving...";

            SaveManager::save(
                *simulation,
                options.savePath
            );
        }
        else
        {
            // Safe final save even if q already saved.
            SaveManager::save(
                *simulation,
                options.savePath
            );
        }

        // Explicitly restore the normal terminal screen
        // before printing ordinary shell output.
        renderer.finish();

        if (interrupted)
        {
            std::cout
                << "Interrupted. World saved to "
                << options.
                    savePath.
                    string()
                << '\n';
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
        // If an exception happens after TerminalRenderer
        // has been constructed, its destructor still
        // restores the alternate screen/cursor.
        std::cerr
            << "Fatal error: "
            << error.what()
            << '\n';

        return 1;
    }
}

#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/SaveManager.hpp"
#include "ascii/SDLFrontend.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/WorldGenerator.hpp"

#include <SDL3/SDL_main.h>

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
// Arguments
// ==================================================

void printHelp()
{
    std::cout
        << "ASCII Universe\n\n"

        << "Usage:\n"
        << "  ./ascii_universe\n"
        << "  ./ascii_universe --seed 12345\n"
        << "  ./ascii_universe --load saves/autosave.json\n"
        << "  ./ascii_universe --seed 12345 --save saves/world.json\n"
        << "  ./ascii_universe --seed 12345 --stop-after 100\n\n"

        << "SDL Controls:\n"
        << "  [ , PageDown   Lower Z-level\n"
        << "  ] . PageUp     Higher Z-level\n"
        << "  Space          Pause/unpause\n"
        << "  S              Save\n"
        << "  Q / Escape     Save and quit\n";
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
// Seed
// ==================================================

std::uint64_t generateSeed()
{
    return
        static_cast<std::uint64_t>(
            std::chrono::
                high_resolution_clock::
                now().
                time_since_epoch().
                count()
        );
}

// ==================================================
// Goblin helper
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
// New world
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

    // Uru — underground miner.
    createGoblin(
        registry,
        "Uru",
        layout.minerSpawn,
        TerminalColor::BrightGreen,
        true,
        false,
        false
    );

    // Kesh — cross-level hauler.
    createGoblin(
        registry,
        "Kesh",
        layout.haulerSpawn,
        TerminalColor::BrightYellow,
        false,
        true,
        false
    );

    // Brakka — surface woodcutter.
    createGoblin(
        registry,
        "Brakka",
        layout.woodcutterSpawn,
        TerminalColor::BrightCyan,
        false,
        false,
        true
    );

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

    for (
        const auto target :
        layout.miningTargets
    )
    {
        simulation->designateMine(
            target
        );
    }

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

    simulation->designateDigDown(
        layout.digDownTarget
    );

    simulation->designateDigUp(
        layout.digUpTarget
    );

    return simulation;
}

// ==================================================
// Loaded camera Z
// ==================================================

int defaultLoadedViewZ(
    entt::registry& registry,
    const GameMap& map
)
{
    auto stockpiles =
        registry.view<
            Stockpile
        >();

    for (
        auto entity :
        stockpiles
    )
    {
        return std::clamp(
            stockpiles.get<
                Stockpile
            >(entity).bounds.min.z,
            0,
            map.depth() - 1
        );
    }

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
// HUD helpers
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

// ==================================================
// Main
// ==================================================

int main(
    int argc,
    char** argv
)
{
    try
    {
        std::signal(
            SIGINT,
            handleSignal
        );

        std::signal(
            SIGTERM,
            handleSignal
        );

        const Options options =
            parseArguments(
                argc,
                argv
            );

        std::unique_ptr<
            Simulation
        > simulation;

        int viewZ =
            0;

        std::string lastMessage;

        if (
            options.loadPath
        )
        {
            simulation =
                SaveManager::load(
                    *options.loadPath
                );

            viewZ =
                defaultLoadedViewZ(
                    simulation->registry(),
                    simulation->map()
                );

            lastMessage =
                "Loaded save.";
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

            lastMessage =
                "Generated new world.";
        }

        auto& map =
            simulation->map();

        auto& registry =
            simulation->registry();

        // ==================================================
        // SDL frontend
        // ==================================================

        SDLFrontend frontend{
            1280,
            720
        };

        bool paused =
            false;

        bool running =
            true;

        // ==================================================
        // HUD
        // ==================================================

        const auto buildHud =
            [&]()
            {
                const auto& jobs =
                    simulation->jobBoard();

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
                    switch (
                        itemStates.get<
                            ItemState
                        >(entity).location
                    )
                    {
                        case ItemLocation::OnGround:
                            ++onGround;
                            break;

                        case ItemLocation::Carried:
                            ++carried;
                            break;

                        case ItemLocation::Stockpiled:
                            ++stored;
                            break;
                    }
                }

                std::vector<std::string>
                    lines;

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

                lines.push_back(
                    "Jobs  available:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::Available
                        )
                    )
                    +
                    "  assigned:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::Assigned
                        )
                    )
                    +
                    "  complete:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::Complete
                        )
                    )
                    +
                    "  cancelled:"
                    +
                    std::to_string(
                        jobs.count(
                            JobState::Cancelled
                        )
                    )
                );

                lines.push_back(
                    "Items  ground:"
                    +
                    std::to_string(
                        onGround
                    )
                    +
                    "  carried:"
                    +
                    std::to_string(
                        carried
                    )
                    +
                    "  stored:"
                    +
                    std::to_string(
                        stored
                    )
                );

                lines.push_back(
                    "Resources  stone:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Stone
                        )
                    )
                    +
                    "  ore:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Ore
                        )
                    )
                    +
                    "  soil:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Soil
                        )
                    )
                    +
                    "  logs:"
                    +
                    std::to_string(
                        countItemType(
                            registry,
                            ItemType::Log
                        )
                    )
                );

                lines.push_back(
                    "Water cells on this Z: "
                    +
                    std::to_string(
                        countWaterCells(
                            map,
                            viewZ
                        )
                    )
                );

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

                lines.push_back(
                    "Legend: # rock  . floor  T tree  1-7 water  < up  > down  X up/down  ^ ramp  = stockpile"
                );

                lines.push_back(
                    "[/,/PgDn lower Z | ]/./PgUp higher Z | SPACE pause | S save | Q/ESC quit"
                );

                lines.push_back(
                    lastMessage
                );

                return lines;
            };

        // ==================================================
        // Initial frame
        // ==================================================

        frontend.render(
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

        const std::uint64_t startingTick =
            simulation->
                time().tick;

        std::uint64_t lastAutosaveTick =
            simulation->
                time().tick;

        // ==================================================
        // Game loop
        // ==================================================

        while (
            running
            &&
            !interrupted
        )
        {
            const auto now =
                Clock::now();

            const auto elapsed =
                std::chrono::
                    duration_cast<
                        std::chrono::
                            nanoseconds
                    >(
                        now -
                        previousTime
                    );

            previousTime =
                now;

            bool redraw =
                false;

            // ==============================================
            // SDL input/events
            // ==============================================

            const FrontendActions actions =
                frontend.pollActions();

            if (
                actions.quit
            )
            {
                running =
                    false;

                break;
            }

            if (
                actions.togglePause
            )
            {
                paused =
                    !paused;

                lastMessage =
                    paused
                    ?
                    "Simulation paused."
                    :
                    "Simulation resumed.";

                previousTime =
                    Clock::now();

                redraw =
                    true;
            }

            if (
                actions.viewZDown
            )
            {
                const int next =
                    std::max(
                        0,
                        viewZ - 1
                    );

                if (
                    next !=
                    viewZ
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
            }

            if (
                actions.viewZUp
            )
            {
                const int next =
                    std::min(
                        map.depth() - 1,
                        viewZ + 1
                    );

                if (
                    next !=
                    viewZ
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
            }

            if (
                actions.save
            )
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
            }

            if (
                actions.redraw
            )
            {
                redraw =
                    true;
            }

            // ==============================================
            // Simulation
            // ==============================================

            if (!paused)
            {
                const int ticks =
                    simulation->advance(
                        elapsed
                    );

                if (
                    ticks > 0
                )
                {
                    redraw =
                        true;
                }

                // ------------------------------------------
                // Autosave every 100 simulation ticks
                // ------------------------------------------

                if (
                    simulation->
                        time().tick
                    >=
                    lastAutosaveTick
                    +
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
                // Testing helper
                // ------------------------------------------

                if (
                    options.stopAfterTicks
                    &&
                    simulation->
                        time().tick
                    >=
                    startingTick
                    +
                    *options.stopAfterTicks
                )
                {
                    running =
                        false;

                    break;
                }
            }
            else
            {
                // Prevent fixed-step catchup after a pause.
                previousTime =
                    Clock::now();
            }

            // ==============================================
            // SDL render
            // ==============================================

            if (redraw)
            {
                frontend.render(
                    map,
                    registry,
                    viewZ,
                    buildHud()
                );
            }

            std::this_thread::
                sleep_for(
                    std::chrono::
                        milliseconds{
                            1
                        }
                );
        }

        // ==================================================
        // Final save
        // ==================================================

        SaveManager::save(
            *simulation,
            options.savePath
        );

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

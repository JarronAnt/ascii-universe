#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/PlayerController.hpp"
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
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

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

struct Options
{
    std::optional<
        std::filesystem::path
    > loadPath;

    std::filesystem::path savePath{
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
        << "  ./ascii_universe --seed 12345\n"
        << "  ./ascii_universe --load saves/autosave.json\n"
        << "  ./ascii_universe --seed 12345 --save saves/world.json\n"
        << "  ./ascii_universe --seed 12345 --stop-after 100\n\n"

        << "Controls:\n"
        << "  WASD              Pan camera\n"
        << "  Arrow keys        Move tile cursor\n"
        << "  [ ] / PgDn PgUp   Change Z-level\n"
        << "  Mouse             Hover/select tiles\n"
        << "  Left drag         Paint designation\n"
        << "  Right click       Return to Inspect\n"
        << "  M                 Mine mode\n"
        << "  T                 Fell-tree mode\n"
        << "  V                 Dig-down mode\n"
        << "  U                 Dig-up mode\n"
        << "  P                 Stockpile mode\n"
        << "  X                 Cancel mode\n"
        << "  I                 Inspect mode\n"
        << "  R                 Cycle stockpile filter\n"
        << "  C                 Configure stockpile under cursor\n"
        << "  Enter             Apply current mode to cursor\n"
        << "  Tab               Cycle goblins\n"
        << "  G                 Select goblin at cursor\n"
        << "  F                 Follow selected goblin\n"
        << "  1/2/3/4           1x/2x/4x/8x speed\n"
        << "  Space             Pause\n"
        << "  F5                Save\n"
        << "  Q                 Save and quit\n";
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

std::unique_ptr<Simulation>
createNewWorld(
    std::uint64_t seed,
    Position& initialFocus
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

    auto& registry =
        simulation->registry();

    // Miner underground.
    createGoblin(
        registry,
        "Uru",
        layout.minerSpawn,
        TerminalColor::BrightGreen,
        true,
        false,
        false
    );

    // Hauler on the surface.
    createGoblin(
        registry,
        "Kesh",
        layout.haulerSpawn,
        TerminalColor::BrightYellow,
        false,
        true,
        false
    );

    // Woodcutter on the surface.
    createGoblin(
        registry,
        "Brakka",
        layout.woodcutterSpawn,
        TerminalColor::BrightCyan,
        false,
        false,
        true
    );

    // Phase 5:
    //
    // NO programmatic stockpile.
    // NO programmatic mining.
    // NO programmatic tree felling.
    // NO programmatic stair excavation.
    //
    // The player must issue those orders.

    initialFocus =
        layout.haulerSpawn;

    return simulation;
}

Position loadedInitialFocus(
    entt::registry& registry,
    const GameMap& map
)
{
    auto goblins =
        registry.view<
            Goblin,
            Position
        >();

    std::vector<entt::entity>
        entities;

    for (auto entity : goblins)
    {
        entities.push_back(
            entity
        );
    }

    std::sort(
        entities.begin(),
        entities.end(),
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

    if (!entities.empty())
    {
        return
            registry.get<
                Position
            >(
                entities.front()
            );
    }

    return Position{
        map.width() / 2,
        map.height() / 2,
        map.depth() - 1
    };
}

}

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

        Position initialFocus{};

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

            initialFocus =
                loadedInitialFocus(
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
                    initialFocus
                );
        }

        PlayerController controller{
            simulation->map(),
            initialFocus
        };

        controller.setStatus(
            loaded
            ?
            "Loaded fortress."
            :
            "New fortress. Issue your first order."
        );

        SDLFrontend frontend{
            1440,
            900
        };

        bool running =
            true;

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
                        now
                        -
                        previousTime
                    );

            previousTime =
                now;

            const PlayerInput input =
                frontend.pollInput(
                    controller.state(),
                    simulation->map()
                );

            if (
                input.quit
            )
            {
                running =
                    false;

                break;
            }

            controller.handleInput(
                input,
                *simulation
            );

            if (
                input.save
            )
            {
                SaveManager::save(
                    *simulation,
                    options.savePath
                );

                controller.setStatus(
                    "Saved to "
                    +
                    options.
                        savePath.
                        string()
                );
            }

            const auto scaledElapsed =
                controller.scaleElapsed(
                    elapsed
                );

            if (
                scaledElapsed >
                std::chrono::
                    nanoseconds::zero()
            )
            {
                simulation->advance(
                    scaledElapsed
                );
            }

            controller.updateFollow(
                *simulation
            );

            // Autosave every 100 simulation ticks.
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

                controller.setStatus(
                    "Autosaved."
                );
            }

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
            }

            frontend.render(
                *simulation,
                controller.state()
            );

            std::this_thread::
                sleep_for(
                    std::chrono::
                        milliseconds{
                            1
                        }
                );
        }

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

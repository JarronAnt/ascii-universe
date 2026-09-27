#pragma once

#include "Color.hpp"
#include "Designations.hpp"
#include "Events.hpp"
#include "GameMap.hpp"
#include "Items.hpp"
#include "Jobs.hpp"
#include "Pathfinder.hpp"
#include "Position.hpp"
#include "Random.hpp"

#include <entt/entt.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

namespace ascii
{

struct SimulationTime
{
    std::uint64_t tick{0};
};

class Simulation
{
public:
    static constexpr
        std::uint32_t TickRate =
            10;

    static constexpr
        std::chrono::nanoseconds
            FixedStep{
                100'000'000
            };

    Simulation(
        int width,
        int height,
        int depth,
        std::uint64_t seed
    );

    int advance(
        std::chrono::nanoseconds
            elapsed
    );

    void step();

    entt::entity designateMine(
        Position position
    );

    entt::entity designateDigDown(
        Position position
    );

    entt::entity designateDigUp(
        Position position
    );

    entt::entity designateFellTree(
        Position position
    );

    entt::entity createStockpile(
        Position min,
        Position max,
        std::vector<ItemType>
            accepts
    );

    [[nodiscard]]
    std::uint64_t worldSeed() const;

    void restoreRuntimeState(
        std::uint64_t tick,
        std::uint64_t rngState
    );

    GameMap& map();
    const GameMap& map() const;

    entt::registry& registry();
    const entt::registry&
        registry() const;

    Random& random();
    const Random& random() const;

    JobBoard& jobBoard();
    const JobBoard& jobBoard() const;

    [[nodiscard]]
    const SimulationTime& time() const;

    [[nodiscard]]
    bool hasOutstandingWork();

private:
    entt::entity createDesignation(
        Position position,
        DesignationType type,
        char character,
        TerminalColor color
    );

    GameMap map_;

    entt::registry registry_;

    Random random_;

    std::uint64_t worldSeed_{};

    Pathfinder pathfinder_;

    JobBoard jobBoard_;

    EventQueue<ItemSpawnEvent>
        itemSpawnEvents_;

    EventQueue<ItemPickupEvent>
        itemPickupEvents_;

    EventQueue<ItemDropEvent>
        itemDropEvents_;

    SimulationTime time_;

    std::chrono::nanoseconds
        accumulator_{0};
};

}

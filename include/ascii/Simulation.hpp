#pragma once

#include "Components.hpp"
#include "GameMap.hpp"
#include "Random.hpp"

#include <entt/entt.hpp>

#include <chrono>
#include <cstdint>

namespace ascii
{

struct SimulationTime
{
    std::uint64_t tick{0};
};

class Simulation
{
public:
    // 10 simulation updates per second.
    //
    // Each simulation tick therefore represents
    // exactly 100 milliseconds of simulation time.
    static constexpr std::uint32_t TickRate = 10;

    static constexpr std::chrono::nanoseconds
        FixedStep{
            100'000'000
        };

    Simulation(
        int width,
        int height,
        std::uint64_t seed
    )
        : map_(width, height),
          random_(seed)
    {
    }

    // --------------------------------------------------
    // Real time -> simulation time
    // --------------------------------------------------
    //
    // The renderer/game loop gives us elapsed real time.
    //
    // We accumulate it until at least one complete
    // fixed simulation step exists.
    //
    // Example:
    //
    // frame 1: +16 ms
    // frame 2: +16 ms
    // frame 3: +16 ms
    // ...
    //
    // once accumulator >= 100 ms:
    //
    //     step()
    //
    // executes exactly once.
    //
    // Returns number of simulation ticks executed.
    int advance(
        std::chrono::nanoseconds elapsed
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

        int ticksExecuted = 0;

        while (accumulator_ >= FixedStep)
        {
            step();

            accumulator_ -= FixedStep;

            ++ticksExecuted;
        }

        return ticksExecuted;
    }

    // Execute exactly ONE simulation tick.
    //
    // This is deliberately independent of real time.
    //
    // Later this is extremely useful for:
    //
    // pause
    // single-step debugging
    // fast forward
    // replay
    // world history simulation
    // automated tests
    void step()
    {
        movementSystem();

        ++time_.tick;
    }

    GameMap& map()
    {
        return map_;
    }

    const GameMap& map() const
    {
        return map_;
    }

    entt::registry& registry()
    {
        return registry_;
    }

    const entt::registry& registry() const
    {
        return registry_;
    }

    Random& random()
    {
        return random_;
    }

    const Random& random() const
    {
        return random_;
    }

    [[nodiscard]]
    const SimulationTime& time() const
    {
        return time_;
    }

private:
    // --------------------------------------------------
    // Movement system
    // --------------------------------------------------
    //
    // Every simulation tick, every entity with:
    //
    // Position
    // MovementPath
    //
    // moves one tile forward along its path.
    void movementSystem()
    {
        auto view =
            registry_.view<
                Position,
                MovementPath
            >();

        for (auto entity : view)
        {
            auto& position =
                view.get<Position>(
                    entity
                );

            auto& path =
                view.get<MovementPath>(
                    entity
                );

            if (path.finished())
            {
                continue;
            }

            const Position next =
                path.nodes[
                    path.nextStep
                ];

            // The world may have changed after the
            // path was originally calculated.
            //
            // Later we'll replace this with automatic
            // path recalculation.
            if (
                !map_.inBounds(
                    next.x,
                    next.y
                )
                ||
                !map_.at(
                    next.x,
                    next.y
                ).walkable()
            )
            {
                // Mark path as finished/invalid.
                path.nextStep =
                    path.nodes.size();

                continue;
            }

            position = next;

            ++path.nextStep;
        }
    }

    GameMap map_;

    entt::registry registry_;

    Random random_;

    SimulationTime time_;

    // Real-time accumulator.
    //
    // IMPORTANT:
    // This does NOT affect simulation rules.
    //
    // It only controls when step() is called.
    std::chrono::nanoseconds accumulator_{0};
};

}

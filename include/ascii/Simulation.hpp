#pragma once

#include "GameMap.hpp"
#include <entt/entt.hpp>
#include <cstdint>

namespace ascii
{

    struct SimulationTime
    {
        std::uint64_t tick{};
    };

    class Simulation
    {
        public:
         Simulation(int width, int height)
            : map_(width, height)
         {}
        
         //fixed timestep
        void tick()
        {
            movementSystem();
            ++time_.tick;
        }
        
        //Getters 
        GameMap& map()
        {
            return map_;
        }

        entt::registry& registry()
        {
            return registry_;
        }

        SimulationTime time() const
        {
            return time_;
        }

    private:
        void movementSystem()
        {}

        GameMap map_;

        entt::registry registry_;

        SimulationTime time_;
    };

}

#pragma once

#include "GameMap.hpp"
#include "Position.hpp"

#include <cstdint>
#include <vector>

namespace ascii
{

struct WorldGenConfig
{
    int initialWallPercent{44};

    int smoothingPasses{5};
};

struct GeneratedWorldLayout
{
    Position minerSpawn{};

    Position haulerSpawn{};

    Position stockpileTopLeft{};

    Position stockpileBottomRight{};

    std::vector<Position>
        miningTargets;
};

class WorldGenerator
{
public:
    [[nodiscard]]
    static GeneratedWorldLayout
    generate(
        GameMap& map,
        std::uint64_t seed,
        const WorldGenConfig& config = {}
    );
};

}

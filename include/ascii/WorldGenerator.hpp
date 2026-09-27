#pragma once

#include "GameMap.hpp"
#include "Position.hpp"

#include <cstdint>
#include <vector>

namespace ascii
{

struct WorldGenConfig
{
    int surfaceVariation{2};

    int smoothingPasses{4};

    int veinCount{22};

    int treeChancePercent{16};
};

struct GeneratedWorldLayout
{
    int defaultViewZ{};

    Position minerSpawn{};

    Position haulerSpawn{};

    Position woodcutterSpawn{};

    Position stockpileMin{};

    Position stockpileMax{};

    std::vector<Position>
        miningTargets;

    std::vector<Position>
        treeTargets;

    Position digDownTarget{};

    Position digUpTarget{};
};

class WorldGenerator
{
public:
    [[nodiscard]]
    static GeneratedWorldLayout
    generate(
        GameMap& map,
        std::uint64_t seed,
        const WorldGenConfig&
            config = {}
    );
};

}

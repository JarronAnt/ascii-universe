#pragma once

#include "GameMap.hpp"
#include "Position.hpp"
#include "WorldEnvironment.hpp"

#include <cstdint>

namespace ascii
{

struct WorldGenConfig
{
    int erosionPasses{
        3
    };

    int minimumRivers{
        1
    };

    int maximumRivers{
        3
    };

    int minimumLakes{
        0
    };

    int maximumLakes{
        2
    };

    int oreVeinCount{
        80
    };

    int cavernLayers{
        2
    };

    int cavernChambersPerLayer{
        7
    };

    double forestDensity{
        1.0
    };
};

struct GeneratedWorldLayout
{
    int defaultViewZ{};

    Position embarkCenter{};

    Position minerSpawn{};
    Position haulerSpawn{};
    Position woodcutterSpawn{};

    LandformType landform{
        LandformType::Unknown
    };

    ClimateType climate{
        ClimateType::Unknown
    };
};

class WorldGenerator
{
public:
    [[nodiscard]]
    static GeneratedWorldLayout generate(
        GameMap& map,
        std::uint64_t seed,
        const WorldGenConfig& config = {}
    );
};

}

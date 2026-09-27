#include "ascii/WorldGenerator.hpp"

#include "ascii/Material.hpp"
#include "ascii/Random.hpp"
#include "ascii/Tile.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ascii
{

namespace
{

std::size_t surfaceIndex(
    int x,
    int y,
    int width
)
{
    return
        static_cast<std::size_t>(
            y * width + x
        );
}

MaterialType soilMaterial(
    Random& random
)
{
    switch (
        random.integer(
            0,
            2
        )
    )
    {
        case 0:
            return MaterialType::Soil;

        case 1:
            return MaterialType::Clay;

        default:
            return MaterialType::Sand;
    }
}

MaterialType baseMaterial(
    int depthBelowSurface,
    Random& random
)
{
    // Top three underground levels are
    // soil/sand/clay.
    if (
        depthBelowSurface <= 3
    )
    {
        return soilMaterial(
            random
        );
    }

    // Sedimentary layer.
    if (
        depthBelowSurface <= 5
    )
    {
        return
            random.chance(0.55)
            ?
            MaterialType::Limestone
            :
            MaterialType::Sandstone;
    }

    switch (
        random.integer(
            0,
            9
        )
    )
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return MaterialType::Granite;

        case 4:
        case 5:
        case 6:
            return MaterialType::Basalt;

        case 7:
        case 8:
            return MaterialType::Marble;

        default:
            return MaterialType::Obsidian;
    }
}

MaterialType randomVeinMaterial(
    Random& random,
    int z,
    int depth
)
{
    const bool deep =
        z <
        depth / 2;

    const int roll =
        random.integer(
            0,
            99
        );

    if (
        deep &&
        roll < 8
    )
    {
        return MaterialType::GoldOre;
    }

    if (
        deep &&
        roll < 18
    )
    {
        return MaterialType::SilverOre;
    }

    if (roll < 34)
    {
        return MaterialType::TinOre;
    }

    if (roll < 53)
    {
        return MaterialType::CopperOre;
    }

    if (roll < 74)
    {
        return MaterialType::IronOre;
    }

    if (roll < 89)
    {
        return MaterialType::Coal;
    }

    return MaterialType::Quartz;
}

void carveFloor(
    GameMap& map,
    Position position
)
{
    if (
        !map.inBounds(position)
    )
    {
        return;
    }

    Tile& tile =
        map.at(position);

    tile.shape =
        TileShape::Floor;

    tile.feature =
        TileFeature::None;

    tile.featureMaterial =
        MaterialType::None;
}

}

GeneratedWorldLayout
WorldGenerator::generate(
    GameMap& map,
    std::uint64_t seed,
    const WorldGenConfig& config
)
{
    if (
        map.width() < 50 ||
        map.height() < 24 ||
        map.depth() < 9
    )
    {
        throw std::runtime_error(
            "3D world requires at least 50x24x9."
        );
    }

    Random random{
        seed ^
        0x73E2A91C6B4D580FULL
    };

    const int width =
        map.width();

    const int height =
        map.height();

    const int depth =
        map.depth();

    const int baseSurfaceZ =
        depth - 3;

    const int centerY =
        height / 2;

    std::vector<int> surface(
        static_cast<std::size_t>(
            width * height
        ),
        baseSurfaceZ
    );

    // ==================================================
    // Random heightmap
    // ==================================================

    for (
        int y = 0;
        y < height;
        ++y
    )
    {
        for (
            int x = 0;
            x < width;
            ++x
        )
        {
            surface[
                surfaceIndex(
                    x,
                    y,
                    width
                )
            ] =
                std::clamp(
                    baseSurfaceZ +
                    random.integer(
                        -config.surfaceVariation,
                        config.surfaceVariation
                    ),
                    3,
                    depth - 2
                );
        }
    }

    constexpr std::array<
        Position,
        4
    > cardinal{
        Position{0, -1, 0},
        Position{1, 0, 0},
        Position{0, 1, 0},
        Position{-1, 0, 0}
    };

    for (
        int pass = 0;
        pass <
            config.smoothingPasses;
        ++pass
    )
    {
        auto next =
            surface;

        for (
            int y = 1;
            y < height - 1;
            ++y
        )
        {
            for (
                int x = 1;
                x < width - 1;
                ++x
            )
            {
                int total =
                    surface[
                        surfaceIndex(
                            x,
                            y,
                            width
                        )
                    ];

                int samples =
                    1;

                for (
                    const auto direction :
                    cardinal
                )
                {
                    total +=
                        surface[
                            surfaceIndex(
                                x +
                                    direction.x,
                                y +
                                    direction.y,
                                width
                            )
                        ];

                    ++samples;
                }

                next[
                    surfaceIndex(
                        x,
                        y,
                        width
                    )
                ] =
                    total /
                    samples;
            }
        }

        surface =
            std::move(next);
    }

    // Stable embark plateau.
    for (
        int y =
            centerY - 6;
        y <=
            centerY + 6;
        ++y
    )
    {
        for (
            int x = 1;
            x <= 25;
            ++x
        )
        {
            surface[
                surfaceIndex(
                    x,
                    y,
                    width
                )
            ] =
                baseSurfaceZ;
        }
    }

    // ==================================================
    // Fill geological column
    // ==================================================

    for (
        int y = 0;
        y < height;
        ++y
    )
    {
        for (
            int x = 0;
            x < width;
            ++x
        )
        {
            const int surfaceZ =
                surface[
                    surfaceIndex(
                        x,
                        y,
                        width
                    )
                ];

            for (
                int z = 0;
                z < depth;
                ++z
            )
            {
                Tile& tile =
                    map.at(
                        x,
                        y,
                        z
                    );

                tile =
                    Tile{};

                if (
                    z >
                    surfaceZ
                )
                {
                    tile.shape =
                        TileShape::Open;

                    tile.material =
                        MaterialType::None;

                    continue;
                }

                if (
                    z ==
                    surfaceZ
                )
                {
                    tile.shape =
                        TileShape::Floor;

                    tile.material =
                        MaterialType::Grass;

                    continue;
                }

                tile.shape =
                    TileShape::Wall;

                tile.material =
                    baseMaterial(
                        surfaceZ - z,
                        random
                    );
            }
        }
    }

    // ==================================================
    // Mineral veins
    // ==================================================

    for (
        int vein = 0;
        vein <
            config.veinCount;
        ++vein
    )
    {
        const int cx =
            random.integer(
                3,
                width - 4
            );

        const int cy =
            random.integer(
                3,
                height - 4
            );

        const int cz =
            random.integer(
                1,
                std::max(
                    1,
                    baseSurfaceZ - 3
                )
            );

        const int rx =
            random.integer(
                2,
                5
            );

        const int ry =
            random.integer(
                1,
                4
            );

        const int rz =
            random.integer(
                0,
                1
            );

        const MaterialType material =
            randomVeinMaterial(
                random,
                cz,
                depth
            );

        for (
            int z =
                std::max(
                    0,
                    cz - rz
                );
            z <=
                std::min(
                    depth - 1,
                    cz + rz
                );
            ++z
        )
        {
            for (
                int y =
                    std::max(
                        1,
                        cy - ry
                    );
                y <=
                    std::min(
                        height - 2,
                        cy + ry
                    );
                ++y
            )
            {
                for (
                    int x =
                        std::max(
                            1,
                            cx - rx
                        );
                    x <=
                        std::min(
                            width - 2,
                            cx + rx
                        );
                    ++x
                )
                {
                    const int dx =
                        x - cx;

                    const int dy =
                        y - cy;

                    if (
                        dx * dx *
                            ry * ry
                        +
                        dy * dy *
                            rx * rx
                        >
                        rx * rx *
                            ry * ry
                    )
                    {
                        continue;
                    }

                    Tile& tile =
                        map.at(
                            x,
                            y,
                            z
                        );

                    if (
                        tile.shape !=
                            TileShape::Wall ||
                        isSoilMaterial(
                            tile.material
                        )
                    )
                    {
                        continue;
                    }

                    tile.material =
                        material;
                }
            }
        }
    }

    // ==================================================
    // Surface ramps
    // ==================================================

    for (
        int y = 1;
        y < height - 1;
        ++y
    )
    {
        for (
            int x = 1;
            x < width - 1;
            ++x
        )
        {
            const int z =
                surface[
                    surfaceIndex(
                        x,
                        y,
                        width
                    )
                ];

            for (
                const auto direction :
                cardinal
            )
            {
                const int neighborZ =
                    surface[
                        surfaceIndex(
                            x +
                                direction.x,
                            y +
                                direction.y,
                            width
                        )
                    ];

                if (
                    neighborZ ==
                    z + 1
                )
                {
                    map.at(
                        x,
                        y,
                        z
                    ).shape =
                        TileShape::Ramp;

                    break;
                }
            }
        }
    }

    // ==================================================
    // Underground test fortress
    // ==================================================

    const int undergroundZ =
        baseSurfaceZ - 3;

    for (
        int y =
            centerY - 4;
        y <=
            centerY + 4;
        ++y
    )
    {
        for (
            int x = 8;
            x <= 20;
            ++x
        )
        {
            carveFloor(
                map,
                Position{
                    x,
                    y,
                    undergroundZ
                }
            );
        }
    }

    // Existing access shaft.
    constexpr int StairX =
        12;

    const int stairY =
        centerY;

    for (
        int z =
            undergroundZ;
        z <=
            baseSurfaceZ;
        ++z
    )
    {
        Tile& tile =
            map.at(
                StairX,
                stairY,
                z
            );

        tile.feature =
            TileFeature::None;

        if (
            z ==
            undergroundZ
        )
        {
            tile.shape =
                TileShape::UpStair;
        }
        else if (
            z ==
            baseSurfaceZ
        )
        {
            tile.shape =
                TileShape::DownStair;
        }
        else
        {
            tile.shape =
                TileShape::UpDownStair;
        }
    }

    // Guaranteed mining face.
    const std::array<
        MaterialType,
        4
    > guaranteed{
        MaterialType::Granite,
        MaterialType::IronOre,
        MaterialType::CopperOre,
        MaterialType::GoldOre
    };

    std::vector<Position>
        miningTargets;

    for (
        int i = 0;
        i < 4;
        ++i
    )
    {
        const Position target{
            21,
            centerY - 2 + i,
            undergroundZ
        };

        Tile& tile =
            map.at(target);

        tile.shape =
            TileShape::Wall;

        tile.material =
            guaranteed[
                static_cast<
                    std::size_t
                >(i)
            ];

        miningTargets.
            push_back(
                target
            );
    }

    // ==================================================
    // Trees
    // ==================================================

    for (
        int y = 1;
        y < height - 1;
        ++y
    )
    {
        for (
            int x = 1;
            x < width - 1;
            ++x
        )
        {
            if (
                x <= 25 &&
                std::abs(
                    y -
                    centerY
                ) <= 6
            )
            {
                continue;
            }

            const int z =
                surface[
                    surfaceIndex(
                        x,
                        y,
                        width
                    )
                ];

            Tile& tile =
                map.at(
                    x,
                    y,
                    z
                );

            if (
                !tile.baseWalkable() ||
                tile.liquid.depth > 0
            )
            {
                continue;
            }

            if (
                random.integer(
                    0,
                    99
                )
                <
                config.treeChancePercent
            )
            {
                tile.feature =
                    TileFeature::Tree;

                tile.featureMaterial =
                    random.chance(
                        0.55
                    )
                    ?
                    MaterialType::OakWood
                    :
                    MaterialType::PineWood;
            }
        }
    }

    const std::array<
        Position,
        4
    > guaranteedTrees{
        Position{
            20,
            centerY - 5,
            baseSurfaceZ
        },
        Position{
            21,
            centerY - 5,
            baseSurfaceZ
        },
        Position{
            22,
            centerY - 5,
            baseSurfaceZ
        },
        Position{
            23,
            centerY - 5,
            baseSurfaceZ
        }
    };

    for (
        std::size_t i = 0;
        i <
            guaranteedTrees.size();
        ++i
    )
    {
        Tile& tile =
            map.at(
                guaranteedTrees[i]
            );

        tile.shape =
            TileShape::Floor;

        tile.material =
            MaterialType::Grass;

        tile.feature =
            TileFeature::Tree;

        tile.featureMaterial =
            i % 2 == 0
            ?
            MaterialType::OakWood
            :
            MaterialType::PineWood;
    }

    // ==================================================
    // Water source / pond
    // ==================================================

    Position highest{
        width - 8,
        4,
        baseSurfaceZ
    };

    int highestZ =
        -1;

    for (
        int y = 3;
        y < height - 3;
        ++y
    )
    {
        for (
            int x =
                width / 2;
            x < width - 3;
            ++x
        )
        {
            const int z =
                surface[
                    surfaceIndex(
                        x,
                        y,
                        width
                    )
                ];

            if (
                z > highestZ
            )
            {
                highestZ =
                    z;

                highest =
                    Position{
                        x,
                        y,
                        z
                    };
            }
        }
    }

    const auto addWater =
        [&](
            Position position,
            std::uint8_t amount
        )
        {
            if (
                !map.inBounds(
                    position
                )
            )
            {
                return;
            }

            Tile& tile =
                map.at(
                    position
                );

            if (
                !tile.baseWalkable()
            )
            {
                return;
            }

            tile.feature =
                TileFeature::None;

            tile.featureMaterial =
                MaterialType::None;

            tile.liquid.type =
                LiquidType::Water;

            tile.liquid.depth =
                amount;
        };

    addWater(
        highest,
        7
    );

    for (
        const auto direction :
        cardinal
    )
    {
        const int nx =
            highest.x +
            direction.x;

        const int ny =
            highest.y +
            direction.y;

        if (
            nx <= 0 ||
            ny <= 0 ||
            nx >= width - 1 ||
            ny >= height - 1
        )
        {
            continue;
        }

        addWater(
            Position{
                nx,
                ny,
                surface[
                    surfaceIndex(
                        nx,
                        ny,
                        width
                    )
                ]
            },
            5
        );
    }

    // ==================================================
    // Layout
    // ==================================================

    GeneratedWorldLayout result;

    result.defaultViewZ =
        baseSurfaceZ;

    result.minerSpawn =
        Position{
            12,
            centerY + 1,
            undergroundZ
        };

    result.haulerSpawn =
        Position{
            5,
            centerY + 2,
            baseSurfaceZ
        };

    result.woodcutterSpawn =
        Position{
            5,
            centerY - 2,
            baseSurfaceZ
        };

    result.stockpileMin =
        Position{
            3,
            centerY - 5,
            baseSurfaceZ
        };

    result.stockpileMax =
        Position{
            9,
            centerY - 3,
            baseSurfaceZ
        };

    for (
        int y =
            result.stockpileMin.y;
        y <=
            result.stockpileMax.y;
        ++y
    )
    {
        for (
            int x =
                result.stockpileMin.x;
            x <=
                result.stockpileMax.x;
            ++x
        )
        {
            Tile& tile =
                map.at(
                    x,
                    y,
                    baseSurfaceZ
                );

            tile.shape =
                TileShape::Floor;

            tile.material =
                MaterialType::Grass;

            tile.feature =
                TileFeature::None;

            tile.featureMaterial =
                MaterialType::None;
        }
    }

    result.miningTargets =
        std::move(
            miningTargets
        );

    result.treeTargets.assign(
        guaranteedTrees.begin(),
        guaranteedTrees.end()
    );

    // Test vertical excavation jobs.
    result.digDownTarget =
        Position{
            15,
            centerY + 2,
            undergroundZ
        };

    result.digUpTarget =
        Position{
            17,
            centerY - 2,
            undergroundZ
        };

    return result;
}

}

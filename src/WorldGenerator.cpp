#include "ascii/WorldGenerator.hpp"

#include "ascii/Material.hpp"
#include "ascii/Random.hpp"
#include "ascii/Tile.hpp"
#include "ascii/worldgen/Noise.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ascii
{

namespace
{

using HeightField =
    std::vector<int>;

using ScalarField =
    std::vector<double>;

using ByteField =
    std::vector<std::uint8_t>;

constexpr std::array<
    Position,
    4
> Cardinal{
    Position{0, -1, 0},
    Position{1, 0, 0},
    Position{0, 1, 0},
    Position{-1, 0, 0}
};


// ==================================================
// Basic helpers
// ==================================================

[[nodiscard]]
std::size_t index2D(
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

[[nodiscard]]
double clamp01(
    double value
)
{
    return
        std::clamp(
            value,
            0.0,
            1.0
        );
}

[[nodiscard]]
double smoothStep(
    double edge0,
    double edge1,
    double value
)
{
    if (
        edge0 ==
        edge1
    )
    {
        return
            value < edge0
            ?
            0.0
            :
            1.0;
    }

    const double t =
        std::clamp(
            (
                value
                -
                edge0
            )
            /
            (
                edge1
                -
                edge0
            ),
            0.0,
            1.0
        );

    return
        t
        *
        t
        *
        (
            3.0
            -
            2.0
            *
            t
        );
}

// ==================================================
// Seed-scale world parameters
// ==================================================

[[nodiscard]]
LandformType chooseLandform(
    Random& random
)
{
    const int roll =
        random.integer(
            0,
            99
        );

    if (roll < 16)
    {
        return
            LandformType::Plains;
    }

    if (roll < 38)
    {
        return
            LandformType::RollingHills;
    }

    if (roll < 55)
    {
        return
            LandformType::Highlands;
    }

    if (roll < 72)
    {
        return
            LandformType::MountainRange;
    }

    if (roll < 87)
    {
        return
            LandformType::RiverValley;
    }

    return
        LandformType::Plateau;
}

[[nodiscard]]
ClimateType chooseClimate(
    Random& random
)
{
    const int roll =
        random.integer(
            0,
            99
        );

    if (roll < 42)
    {
        return
            ClimateType::Temperate;
    }

    if (roll < 66)
    {
        return
            ClimateType::WetTemperate;
    }

    if (roll < 83)
    {
        return
            ClimateType::Boreal;
    }

    return
        ClimateType::Dry;
}

[[nodiscard]]
double baseMoistureForClimate(
    ClimateType climate
)
{
    switch (climate)
    {
        case ClimateType::Temperate:
            return 0.55;

        case ClimateType::WetTemperate:
            return 0.76;

        case ClimateType::Boreal:
            return 0.61;

        case ClimateType::Dry:
            return 0.24;

        case ClimateType::Unknown:
        default:
            return 0.50;
    }
}
  

[[nodiscard]]
double baseTemperatureForClimate(
    ClimateType climate
)
{
    switch (climate)
    {
        case ClimateType::Temperate:
            return 0.58;

        case ClimateType::WetTemperate:
            return 0.61;

        case ClimateType::Boreal:
            return 0.36;

        case ClimateType::Dry:
            return 0.73;

        case ClimateType::Unknown:
        default:
            return 0.55;
    }
}
// ==================================================
// Elevation
// ==================================================

[[nodiscard]]
HeightField generateElevation(
    int width,
    int height,
    int depth,
    std::uint64_t seed,
    LandformType landform
)
{
    // Keep meaningful underground depth even in lowlands.
    const int minimumSurface =
        std::max(
            5,
            depth / 4
        );

    // Leave air above the highest terrain.
    const int maximumSurface =
        depth - 3;

    const int usableRange =
        std::max(
            1,
            maximumSurface
            -
            minimumSurface
        );

    HeightField surface(
        static_cast<std::size_t>(
            width * height
        ),
        minimumSurface
    );

    // A random world-space orientation allows mountain
    // chains to run diagonally instead of always aligning
    // with the grid.
    const double angle =
        worldgen::unitHash(
            seed
            ^
            0x91E10DA5C79E7B1DULL,
            0,
            0
        )
        *
        6.28318530717958647692;

    const double cosine =
        std::cos(
            angle
        );

    const double sine =
        std::sin(
            angle
        );

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
            const double nx =
                width > 1
                ?
                (
                    static_cast<double>(
                        x
                    )
                    /
                    static_cast<double>(
                        width - 1
                    )
                )
                *
                2.0
                -
                1.0
                :
                0.0;

            const double ny =
                height > 1
                ?
                (
                    static_cast<double>(
                        y
                    )
                    /
                    static_cast<double>(
                        height - 1
                    )
                )
                *
                2.0
                -
                1.0
                :
                0.0;

            // Broad continental / regional shape.
            const double macro =
                worldgen::fbm2D(
                    seed
                    ^
                    0xD2B74407B1CE6E93ULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    42.0,
                    5,
                    0.53
                );

            // Smaller hills and local terrain.
            const double detail =
                worldgen::fbm2D(
                    seed
                    ^
                    0xCA5A826395121157ULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    11.0,
                    3,
                    0.5
                );

            // Mountain-like ridges.
            const double ridgeNoise =
                worldgen::ridged2D(
                    seed
                    ^
                    0xA3B195354A39B70DULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    18.0,
                    4,
                    0.52
                );

            double normalized =
                0.5;

            switch (landform)
            {
                // ==========================================
                // Plains
                // ==========================================

                case LandformType::Plains:
                {
                    normalized =
                        0.41
                        +
                        macro
                        *
                        0.07
                        +
                        detail
                        *
                        0.035;

                    break;
                }

                // ==========================================
                // Rolling hills
                // ==========================================

                case LandformType::RollingHills:
                {
                    normalized =
                        0.43
                        +
                        macro
                        *
                        0.18
                        +
                        detail
                        *
                        0.08;

                    break;
                }

                // ==========================================
                // Highlands
                // ==========================================

                case LandformType::Highlands:
                {
                    normalized =
                        0.57
                        +
                        macro
                        *
                        0.17
                        +
                        (
                            ridgeNoise
                            -
                            0.5
                        )
                        *
                        0.14
                        +
                        detail
                        *
                        0.05;

                    break;
                }

                // ==========================================
                // Mountain chain
                // ==========================================

                case LandformType::MountainRange:
                {
                    // Rotate coordinates so the dominant
                    // range orientation differs by seed.
                    const double rotatedY =
                        -nx
                        *
                        sine
                        +
                        ny
                        *
                        cosine;

                    // Warp the mountain chain so it bends.
                    const double bend =
                        worldgen::fbm2D(
                            seed
                            ^
                            0x9E3779B185EBCA87ULL,
                            static_cast<double>(
                                x
                            ),
                            static_cast<double>(
                                y
                            ),
                            50.0,
                            3,
                            0.55
                        )
                        *
                        0.22;

                    const double distance =
                        std::abs(
                            rotatedY
                            -
                            bend
                        );

                    // Gaussian-style mountain envelope.
                    const double envelope =
                        std::exp(
                            -(
                                distance
                                *
                                distance
                            )
                            /
                            0.055
                        );

                    normalized =
                        0.29
                        +
                        envelope
                        *
                        (
                            0.38
                            +
                            ridgeNoise
                            *
                            0.26
                        )
                        +
                        macro
                        *
                        0.07
                        +
                        detail
                        *
                        0.035;

                    break;
                }

                // ==========================================
                // River valley
                // ==========================================

                case LandformType::RiverValley:
                {
                    const double bend =
                        worldgen::fbm2D(
                            seed
                            ^
                            0x8CB92BA72F3D8DD7ULL,
                            static_cast<double>(
                                x
                            ),
                            static_cast<double>(
                                y
                            ),
                            55.0,
                            3,
                            0.55
                        )
                        *
                        0.24;

                    const double distance =
                        std::abs(
                            ny
                            -
                            bend
                        );

                    const double valley =
                        std::exp(
                            -(
                                distance
                                *
                                distance
                            )
                            /
                            0.065
                        );

                    normalized =
                        0.62
                        -
                        valley
                        *
                        0.34
                        +
                        macro
                        *
                        0.09
                        +
                        detail
                        *
                        0.035;

                    break;
                }

                // ==========================================
                // Plateau
                // ==========================================

                case LandformType::Plateau:
                {
                    const double edge =
                        std::max(
                            std::abs(
                                nx
                            ),
                            std::abs(
                                ny
                            )
                        );

                    const double warpedEdge =
                        edge
                        +
                        macro
                        *
                        0.10;

                    const double plateau =
                        1.0
                        -
                        smoothStep(
                            0.43,
                            0.72,
                            warpedEdge
                        );

                    normalized =
                        0.31
                        +
                        plateau
                        *
                        0.39
                        +
                        macro
                        *
                        0.08
                        +
                        detail
                        *
                        0.035;

                    break;
                }

                case LandformType::Unknown:
                default:
                {
                    // Fallback terrain if an invalid/unknown
                    // landform somehow reaches generation.
                    //
                    // Newly generated worlds should never
                    // normally use Unknown because
                    // chooseLandform() always returns a real
                    // landform.
                    normalized =
                        0.43
                        +
                        macro
                        *
                        0.18
                        +
                        detail
                        *
                        0.08;

                    break;
                }
            }

            normalized =
                clamp01(
                    normalized
                );

            surface[
                index2D(
                    x,
                    y,
                    width
                )
            ] =
                std::clamp(
                    minimumSurface
                    +
                    static_cast<int>(
                        std::lround(
                            normalized
                            *
                            static_cast<double>(
                                usableRange
                            )
                        )
                    ),
                    minimumSurface,
                    maximumSurface
                );
        }
    }

    return surface;
}

// ==================================================
// Simple thermal-style erosion
//
// We don't need a full geological erosion simulator yet.
//
// This mainly removes unnatural one-tile height spikes
// while retaining large mountains/valleys.
// ==================================================

void erodeSurface(
    HeightField& surface,
    int width,
    int height,
    int depth,
    int passes
)
{
    const int minimumSurface =
        std::max(
            5,
            depth / 4
        );

    const int maximumSurface =
        depth - 3;

    for (
        int pass = 0;
        pass < passes;
        ++pass
    )
    {
        HeightField next =
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
                const std::size_t currentIndex =
                    index2D(
                        x,
                        y,
                        width
                    );

                int total =
                    0;

                int lowest =
                    surface[
                        currentIndex
                    ];

                int highest =
                    surface[
                        currentIndex
                    ];

                for (
                    const Position direction :
                    Cardinal
                )
                {
                    const int value =
                        surface[
                            index2D(
                                x
                                +
                                direction.x,
                                y
                                +
                                direction.y,
                                width
                            )
                        ];

                    total +=
                        value;

                    lowest =
                        std::min(
                            lowest,
                            value
                        );

                    highest =
                        std::max(
                            highest,
                            value
                        );
                }

                const int average =
                    static_cast<int>(
                        std::lround(
                            static_cast<double>(
                                total
                            )
                            /
                            4.0
                        )
                    );

                const int current =
                    surface[
                        currentIndex
                    ];

                if (
                    current
                    -
                    lowest
                    >
                    2
                    &&
                    current
                    >
                    average
                )
                {
                    next[
                        currentIndex
                    ] =
                        current
                        -
                        1;
                }
                else if (
                    highest
                    -
                    current
                    >
                    2
                    &&
                    current
                    <
                    average
                )
                {
                    next[
                        currentIndex
                    ] =
                        current
                        +
                        1;
                }

                next[
                    currentIndex
                ] =
                    std::clamp(
                        next[
                            currentIndex
                        ],
                        minimumSurface,
                        maximumSurface
                    );
            }
        }

        surface =
            std::move(
                next
            );
    }
}

// ==================================================
// Local slope
// ==================================================

[[nodiscard]]
int localSlope(
    const HeightField& surface,
    int width,
    int height,
    int x,
    int y
)
{
    const int center =
        surface[
            index2D(
                x,
                y,
                width
            )
        ];

    int slope =
        0;

    for (
        const Position direction :
        Cardinal
    )
    {
        const int nx =
            x
            +
            direction.x;

        const int ny =
            y
            +
            direction.y;

        if (
            nx < 0
            ||
            ny < 0
            ||
            nx >= width
            ||
            ny >= height
        )
        {
            continue;
        }

        slope =
            std::max(
                slope,
                std::abs(
                    center
                    -
                    surface[
                        index2D(
                            nx,
                            ny,
                            width
                        )
                    ]
                )
            );
    }

    return slope;
}

// ==================================================
// Climate
// ==================================================

void generateClimateFields(
    const HeightField& surface,
    int width,
    int height,
    int depth,
    std::uint64_t seed,
    ClimateType climate,
    ScalarField& moisture,
    ScalarField& temperature
)
{
    moisture.assign(
        static_cast<std::size_t>(
            width * height
        ),
        0.5
    );

    temperature.assign(
        static_cast<std::size_t>(
            width * height
        ),
        0.5
    );

    const int minimumSurface =
        std::max(
            5,
            depth / 4
        );

    const int maximumSurface =
        depth - 3;

    const double elevationRange =
        static_cast<double>(
            std::max(
                1,
                maximumSurface
                -
                minimumSurface
            )
        );

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
            const std::size_t i =
                index2D(
                    x,
                    y,
                    width
                );

            const double elevation =
                static_cast<double>(
                    surface[i]
                    -
                    minimumSurface
                )
                /
                elevationRange;

            const double wetNoise =
                worldgen::fbm2D(
                    seed
                    ^
                    0xDB4F0B9175AE2165ULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    34.0,
                    4,
                    0.55
                );

            const double rainShadowNoise =
                worldgen::fbm2D(
                    seed
                    ^
                    0xBBE0563303A4615FULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    60.0,
                    2,
                    0.6
                );

            const double temperatureNoise =
                worldgen::fbm2D(
                    seed
                    ^
                    0xA0F2EC75A1FE1575ULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    46.0,
                    3,
                    0.55
                );

            moisture[i] =
                clamp01(
                    baseMoistureForClimate(
                        climate
                    )
                    +
                    wetNoise
                    *
                    0.23
                    +
                    rainShadowNoise
                    *
                    0.08
                    -
                    elevation
                    *
                    0.09
                );

            // Higher terrain is colder.
            temperature[i] =
                clamp01(
                    baseTemperatureForClimate(
                        climate
                    )
                    +
                    temperatureNoise
                    *
                    0.09
                    -
                    elevation
                    *
                    0.30
                );
        }
    }
}

// ==================================================
// Surface water generation
// ==================================================

[[nodiscard]]
int distanceToNearestEdge(
    int x,
    int y,
    int width,
    int height
)
{
    return
        std::min({
            x,
            y,
            width - 1 - x,
            height - 1 - y
        });
}

// ==================================================
// River tracing
//
// This is not a full watershed solver yet.
//
// Rivers:
//
// - originate preferentially in high terrain
// - move toward low terrain
// - have seeded meandering
// - slightly erode the surface
// - breach tiny local basins
// - merge into existing rivers
// ==================================================

void traceRiver(
    HeightField& surface,
    ByteField& waterDepth,
    int width,
    int height,
    int depth,
    std::uint64_t seed,
    Random& random,
    Position source
)
{
    const int minimumSurface =
        std::max(
            5,
            depth / 4
        );

    std::vector<std::uint8_t>
        visited(
            static_cast<std::size_t>(
                width * height
            ),
            0
        );

    Position current =
        source;

    const int maximumSteps =
        (
            width
            +
            height
        )
        *
        4;

    for (
        int step = 0;
        step < maximumSteps;
        ++step
    )
    {
        if (
            current.x < 1
            ||
            current.y < 1
            ||
            current.x >= width - 1
            ||
            current.y >= height - 1
        )
        {
            break;
        }

        const std::size_t currentIndex =
            index2D(
                current.x,
                current.y,
                width
            );

        if (
            visited[
                currentIndex
            ]
            !=
            0
        )
        {
            break;
        }

        visited[
            currentIndex
        ] =
            1;

        // Carve a shallow river channel.
        surface[
            currentIndex
        ] =
            std::max(
                minimumSurface,
                surface[
                    currentIndex
                ]
                -
                1
            );

        waterDepth[
            currentIndex
        ] =
            std::max<std::uint8_t>(
                waterDepth[
                    currentIndex
                ],
                static_cast<std::uint8_t>(
                    random.integer(
                        3,
                        5
                    )
                )
            );

        // Occasionally make the river two cells wide.
        if (
            step % 5 == 0
        )
        {
            const Position side =
                Cardinal[
                    static_cast<std::size_t>(
                        random.integer(
                            0,
                            3
                        )
                    )
                ];

            const int sx =
                current.x
                +
                side.x;

            const int sy =
                current.y
                +
                side.y;

            if (
                sx > 0
                &&
                sy > 0
                &&
                sx < width - 1
                &&
                sy < height - 1
            )
            {
                const std::size_t sideIndex =
                    index2D(
                        sx,
                        sy,
                        width
                    );

                surface[
                    sideIndex
                ] =
                    std::min(
                        surface[
                            sideIndex
                        ],
                        surface[
                            currentIndex
                        ]
                    );

                waterDepth[
                    sideIndex
                ] =
                    std::max<std::uint8_t>(
                        waterDepth[
                            sideIndex
                        ],
                        2
                    );
            }
        }

        double bestScore =
            std::numeric_limits<
                double
            >::infinity();

        Position next =
            current;

        for (
            const Position direction :
            Cardinal
        )
        {
            const int nx =
                current.x
                +
                direction.x;

            const int ny =
                current.y
                +
                direction.y;

            if (
                nx < 0
                ||
                ny < 0
                ||
                nx >= width
                ||
                ny >= height
            )
            {
                continue;
            }

            const std::size_t neighborIndex =
                index2D(
                    nx,
                    ny,
                    width
                );

            if (
                visited[
                    neighborIndex
                ]
                !=
                0
            )
            {
                continue;
            }

            const double meander =
                worldgen::signedHash(
                    seed
                    ^
                    0xE7037ED1A0B428DBULL,
                    nx,
                    ny,
                    step
                );

            // Lower elevation is preferred.
            //
            // A weak distance-to-edge bias encourages
            // drainage toward the map boundary.
            const double score =
                static_cast<double>(
                    surface[
                        neighborIndex
                    ]
                )
                *
                5.0
                +
                static_cast<double>(
                    distanceToNearestEdge(
                        nx,
                        ny,
                        width,
                        height
                    )
                )
                *
                0.055
                +
                meander
                *
                0.75;

            if (
                score <
                bestScore
            )
            {
                bestScore =
                    score;

                next =
                    Position{
                        nx,
                        ny,
                        0
                    };
            }
        }

        if (
            next.x ==
                current.x
            &&
            next.y ==
                current.y
        )
        {
            break;
        }

        const std::size_t nextIndex =
            index2D(
                next.x,
                next.y,
                width
            );

        // Break through very small local depressions.
        if (
            surface[
                nextIndex
            ]
            >
            surface[
                currentIndex
            ]
        )
        {
            surface[
                nextIndex
            ] =
                surface[
                    currentIndex
                ];
        }

        // Join another water feature.
        if (
            waterDepth[
                nextIndex
            ]
            >
            0
            &&
            step > 4
        )
        {
            current =
                next;

            waterDepth[
                nextIndex
            ] =
                std::max<std::uint8_t>(
                    waterDepth[
                        nextIndex
                    ],
                    4
                );

            break;
        }

        current =
            next;
    }
}

void generateRivers(
    HeightField& surface,
    ByteField& waterDepth,
    int width,
    int height,
    int depth,
    std::uint64_t seed,
    Random& random,
    int count
)
{
    for (
        int river = 0;
        river < count;
        ++river
    )
    {
        Position source{
            width / 2,
            height / 2,
            0
        };

        double bestScore =
            -std::numeric_limits<
                double
            >::infinity();

        // Choose a high-elevation source rather than
        // simply selecting a random position.
        for (
            int attempt = 0;
            attempt < 160;
            ++attempt
        )
        {
            const int x =
                random.integer(
                    4,
                    width - 5
                );

            const int y =
                random.integer(
                    4,
                    height - 5
                );

            const std::size_t i =
                index2D(
                    x,
                    y,
                    width
                );

            const double score =
                static_cast<double>(
                    surface[i]
                )
                *
                4.0
                +
                worldgen::signedHash(
                    seed
                    ^
                    0x589965CC75374CC3ULL,
                    x,
                    y,
                    river
                );

            if (
                score >
                bestScore
            )
            {
                bestScore =
                    score;

                source =
                    Position{
                        x,
                        y,
                        0
                    };
            }
        }

        traceRiver(
            surface,
            waterDepth,
            width,
            height,
            depth,
            seed
            +
            static_cast<std::uint64_t>(
                river
            )
            *
            0x9E3779B97F4A7C15ULL,
            random,
            source
        );
    }
}

// ==================================================
// Lakes
// ==================================================

void generateLakes(
    HeightField& surface,
    ByteField& waterDepth,
    int width,
    int height,
    int depth,
    std::uint64_t seed,
    Random& random,
    int count
)
{
    const int minimumSurface =
        std::max(
            5,
            depth / 4
        );

    for (
        int lake = 0;
        lake < count;
        ++lake
    )
    {
        Position center{
            width / 2,
            height / 2,
            0
        };

        double bestScore =
            std::numeric_limits<
                double
            >::infinity();

        // Search several candidates and prefer relatively
        // low terrain.
        for (
            int attempt = 0;
            attempt < 100;
            ++attempt
        )
        {
            const int x =
                random.integer(
                    5,
                    width - 6
                );

            const int y =
                random.integer(
                    5,
                    height - 6
                );

            const std::size_t i =
                index2D(
                    x,
                    y,
                    width
                );

            const double basinNoise =
                worldgen::signedHash(
                    seed
                    ^
                    0xC13FA9A902A6328FULL,
                    x,
                    y,
                    lake
                );

            const double score =
                static_cast<double>(
                    surface[i]
                )
                +
                basinNoise
                *
                0.8;

            if (
                score <
                bestScore
            )
            {
                bestScore =
                    score;

                center =
                    Position{
                        x,
                        y,
                        0
                    };
            }
        }

        const int radiusX =
            random.integer(
                3,
                7
            );

        const int radiusY =
            random.integer(
                2,
                5
            );

        const int lakeFloor =
            std::max(
                minimumSurface,
                surface[
                    index2D(
                        center.x,
                        center.y,
                        width
                    )
                ]
                -
                1
            );

        for (
            int y =
                std::max(
                    1,
                    center.y
                    -
                    radiusY
                );
            y <=
                std::min(
                    height - 2,
                    center.y
                    +
                    radiusY
                );
            ++y
        )
        {
            for (
                int x =
                    std::max(
                        1,
                        center.x
                        -
                        radiusX
                    );
                x <=
                    std::min(
                        width - 2,
                        center.x
                        +
                        radiusX
                    );
                ++x
            )
            {
                const double dx =
                    static_cast<double>(
                        x
                        -
                        center.x
                    )
                    /
                    static_cast<double>(
                        radiusX
                    );

                const double dy =
                    static_cast<double>(
                        y
                        -
                        center.y
                    )
                    /
                    static_cast<double>(
                        radiusY
                    );

                const double distance =
                    dx * dx
                    +
                    dy * dy;

                // Break the perfect ellipse into a more
                // natural shoreline.
                const double irregularity =
                    worldgen::valueNoise2D(
                        seed
                        ^
                        0x91A2DEC89025CC1DULL,
                        static_cast<double>(
                            x
                        )
                        /
                        3.0,
                        static_cast<double>(
                            y
                        )
                        /
                        3.0
                    )
                    *
                    0.15;

                if (
                    distance
                    >
                    1.0
                    +
                    irregularity
                )
                {
                    continue;
                }

                const std::size_t i =
                    index2D(
                        x,
                        y,
                        width
                    );

                surface[i] =
                    std::min(
                        surface[i],
                        lakeFloor
                    );

                // Deeper near the center.
                const double centerWeight =
                    clamp01(
                        1.0
                        -
                        distance
                    );

                const int depthValue =
                    std::clamp(
                        4
                        +
                        static_cast<int>(
                            std::lround(
                                centerWeight
                                *
                                3.0
                            )
                        ),
                        4,
                        7
                    );

                waterDepth[i] =
                    std::max<std::uint8_t>(
                        waterDepth[i],
                        static_cast<std::uint8_t>(
                            depthValue
                        )
                    );
            }
        }
    }
}

// ==================================================
// Distance transform from surface water
//
// Used for riparian moisture and embark evaluation.
// ==================================================

[[nodiscard]]
std::vector<int> distanceFromWater(
    const ByteField& waterDepth,
    int width,
    int height
)
{
    constexpr int Far =
        1'000'000;

    std::vector<int> distance(
        static_cast<std::size_t>(
            width * height
        ),
        Far
    );

    std::queue<Position>
        frontier;

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
            const std::size_t i =
                index2D(
                    x,
                    y,
                    width
                );

            if (
                waterDepth[i]
                ==
                0
            )
            {
                continue;
            }

            distance[i] =
                0;

            frontier.push(
                Position{
                    x,
                    y,
                    0
                }
            );
        }
    }

    while (
        !frontier.empty()
    )
    {
        const Position current =
            frontier.front();

        frontier.pop();

        const int currentDistance =
            distance[
                index2D(
                    current.x,
                    current.y,
                    width
                )
            ];

        for (
            const Position direction :
            Cardinal
        )
        {
            const int nx =
                current.x
                +
                direction.x;

            const int ny =
                current.y
                +
                direction.y;

            if (
                nx < 0
                ||
                ny < 0
                ||
                nx >= width
                ||
                ny >= height
            )
            {
                continue;
            }

            const std::size_t i =
                index2D(
                    nx,
                    ny,
                    width
                );

            if (
                distance[i]
                <=
                currentDistance
                +
                1
            )
            {
                continue;
            }

            distance[i] =
                currentDistance
                +
                1;

            frontier.push(
                Position{
                    nx,
                    ny,
                    0
                }
            );
        }
    }

    return distance;
}

// ==================================================
// Riparian moisture
//
// Water changes nearby ecology instead of merely being
// rendered as an isolated blue feature.
// ==================================================

void addRiparianMoisture(
    ScalarField& moisture,
    const ByteField& waterDepth,
    int width,
    int height
)
{
    const std::vector<int> distance =
        distanceFromWater(
            waterDepth,
            width,
            height
        );

    for (
        std::size_t i = 0;
        i < moisture.size();
        ++i
    )
    {
        if (
            distance[i]
            >
            18
        )
        {
            continue;
        }

        const double influence =
            1.0
            -
            static_cast<double>(
                distance[i]
            )
            /
            18.0;

        moisture[i] =
            clamp01(
                moisture[i]
                +
                influence
                *
                0.24
            );
    }
}

// ==================================================
// Geological provinces
//
// Instead of every column independently rolling a
// random rock, large regions share a broad geological
// history.
// ==================================================

enum class GeologyProvince
{
    Sedimentary,
    Granitic,
    Volcanic,
    Metamorphic
};

[[nodiscard]]
GeologyProvince geologyProvinceAt(
    std::uint64_t seed,
    LandformType landform,
    int x,
    int y
)
{
    const double province =
        worldgen::fbm2D(
            seed
            ^
            0xD6E8FEB86659FD93ULL,
            static_cast<double>(
                x
            ),
            static_cast<double>(
                y
            ),
            52.0,
            4,
            0.58
        );

    const double structure =
        worldgen::ridged2D(
            seed
            ^
            0xA4093822299F31D0ULL,
            static_cast<double>(
                x
            ),
            static_cast<double>(
                y
            ),
            36.0,
            3,
            0.55
        );

    // Mountain regions are especially likely to expose
    // intrusive or metamorphic geology.
    if (
        landform ==
            LandformType::MountainRange
        &&
        structure >
            0.68
    )
    {
        return
            province > 0.0
            ?
            GeologyProvince::Granitic
            :
            GeologyProvince::Metamorphic;
    }

    if (
        province <
        -0.34
    )
    {
        return
            GeologyProvince::Sedimentary;
    }

    if (
        province <
        0.02
    )
    {
        return
            GeologyProvince::Metamorphic;
    }

    if (
        province <
        0.43
    )
    {
        return
            GeologyProvince::Granitic;
    }

    return
        GeologyProvince::Volcanic;
}

// ==================================================
// Soil
// ==================================================

[[nodiscard]]
MaterialType soilMaterialAt(
    std::uint64_t seed,
    double moisture,
    double temperature,
    int x,
    int y
)
{
    if (
        moisture >
            0.82
        &&
        temperature <
            0.56
    )
    {
        return
            MaterialType::Peat;
    }

    if (
        moisture <
        0.22
    )
    {
        return
            MaterialType::Sand;
    }

    const double sediment =
        worldgen::fbm2D(
            seed
            ^
            0x13198A2E03707344ULL,
            static_cast<double>(
                x
            ),
            static_cast<double>(
                y
            ),
            18.0,
            3,
            0.5
        );

    if (
        sediment >
        0.38
    )
    {
        return
            MaterialType::Clay;
    }

    if (
        sediment <
        -0.40
    )
    {
        return
            MaterialType::Silt;
    }

    if (
        moisture >
        0.48
    )
    {
        return
            MaterialType::Loam;
    }

    return
        MaterialType::Soil;
}

[[nodiscard]]
int soilDepthAt(
    LandformType landform,
    double moisture,
    int slope,
    double localNoise
)
{
    int result =
        1
        +
        static_cast<int>(
            std::lround(
                moisture
                *
                2.5
            )
        );

    // Flat wet terrain accumulates more sediment.
    if (
        slope ==
        0
    )
    {
        ++result;
    }

    // Steeper ground loses material through erosion.
    if (
        slope >=
        2
    )
    {
        --result;
    }

    if (
        landform ==
            LandformType::MountainRange
        ||
        landform ==
            LandformType::Highlands
    )
    {
        --result;
    }

    if (
        localNoise >
        0.45
    )
    {
        ++result;
    }
    else if (
        localNoise <
        -0.45
    )
    {
        --result;
    }

    return
        std::clamp(
            result,
            1,
            4
        );
}

// ==================================================
// Geological strata
//
// Rock changes primarily with depth within a regional
// geological province.
//
// Noise and gentle tilting make layers dip and warp
// rather than forming perfectly horizontal slabs.
// ==================================================

[[nodiscard]]
MaterialType bedrockMaterialAt(
    GeologyProvince province,
    int bedrockDepth,
    int x,
    int y,
    int width,
    std::uint64_t seed
)
{
    const double tiltX =
        worldgen::signedHash(
            seed
            ^
            0x243F6A8885A308D3ULL,
            1,
            0
        )
        *
        0.035;

    const double tiltY =
        worldgen::signedHash(
            seed
            ^
            0x13198A2E03707344ULL,
            0,
            1
        )
        *
        0.035;

    const double warp =
        worldgen::fbm2D(
            seed
            ^
            0x082EFA98EC4E6C89ULL,
            static_cast<double>(
                x
            ),
            static_cast<double>(
                y
            ),
            30.0,
            3,
            0.55
        )
        *
        2.0;

    const double centeredX =
        static_cast<double>(
            x
            -
            width / 2
        );

    const double effective =
        static_cast<double>(
            bedrockDepth
        )
        +
        centeredX
        *
        tiltX
        +
        static_cast<double>(
            y
        )
        *
        tiltY
        +
        warp;

    const int depth =
        std::max(
            0,
            static_cast<int>(
                std::floor(
                    effective
                )
            )
        );

    switch (province)
    {
        // ==============================================
        // Sedimentary succession
        // ==============================================

        case GeologyProvince::Sedimentary:
        {
            if (depth < 3)
            {
                return
                    MaterialType::Shale;
            }

            if (depth < 6)
            {
                return
                    MaterialType::Limestone;
            }

            if (depth < 10)
            {
                return
                    MaterialType::Sandstone;
            }

            if (depth < 14)
            {
                return
                    MaterialType::Dolomite;
            }

            return
                MaterialType::Shale;
        }

        // ==============================================
        // Intrusive / continental crust
        // ==============================================

        case GeologyProvince::Granitic:
        {
            if (depth < 3)
            {
                return
                    MaterialType::Gneiss;
            }

            if (depth < 9)
            {
                return
                    MaterialType::Granite;
            }

            if (depth < 14)
            {
                return
                    MaterialType::Diorite;
            }

            return
                MaterialType::Granite;
        }

        // ==============================================
        // Volcanic
        // ==============================================

        case GeologyProvince::Volcanic:
        {
            // Sparse near-surface obsidian lenses.
            if (
                depth <
                2
            )
            {
                const double glassLens =
                    worldgen::signedHash(
                        seed
                        ^
                        0x452821E638D01377ULL,
                        x,
                        y,
                        depth
                    );

                if (
                    glassLens >
                    0.72
                )
                {
                    return
                        MaterialType::Obsidian;
                }
            }

            if (
                depth <
                8
            )
            {
                return
                    MaterialType::Basalt;
            }

            return
                MaterialType::Gabbro;
        }

        // ==============================================
        // Metamorphic
        // ==============================================

        case GeologyProvince::Metamorphic:
        {
            if (depth < 3)
            {
                return
                    MaterialType::Slate;
            }

            if (depth < 7)
            {
                return
                    MaterialType::Schist;
            }

            if (depth < 12)
            {
                return
                    MaterialType::Gneiss;
            }

            if (depth < 15)
            {
                return
                    MaterialType::Marble;
            }

            return
                MaterialType::Gneiss;
        }
    }

    return
        MaterialType::Granite;
}

// ==================================================
// Surface exposure
// ==================================================

[[nodiscard]]
MaterialType surfaceMaterialAt(
    const HeightField& surface,
    int width,
    int height,
    int depth,
    std::uint64_t seed,
    LandformType landform,
    double moisture,
    double temperature,
    int x,
    int y
)
{
    const int slope =
        localSlope(
            surface,
            width,
            height,
            x,
            y
        );

    // Very dry surfaces become sand.
    if (
        moisture <
        0.18
    )
    {
        return
            MaterialType::Sand;
    }

    // Cold/wet low-gradient terrain can develop peat.
    if (
        moisture >
            0.86
        &&
        temperature <
            0.52
        &&
        slope ==
            0
    )
    {
        return
            MaterialType::Peat;
    }

    // Steep terrain exposes bedrock instead of magically
    // retaining grass-covered soil.
    if (
        slope >=
            3
        ||
        (
            landform ==
                LandformType::MountainRange
            &&
            slope >=
                2
        )
    )
    {
        const int surfaceZ =
            surface[
                index2D(
                    x,
                    y,
                    width
                )
            ];

        const GeologyProvince province =
            geologyProvinceAt(
                seed,
                landform,
                x,
                y
            );

        return
            bedrockMaterialAt(
                province,
                0,
                x,
                y,
                width,
                seed
                ^
                static_cast<std::uint64_t>(
                    surfaceZ
                    +
                    depth
                )
            );
    }

    return
        MaterialType::Grass;
}

// ==================================================
// Populate the full 3D tile volume
// ==================================================

void fillTerrainVolume(
    GameMap& map,
    const HeightField& surface,
    const ScalarField& moisture,
    const ScalarField& temperature,
    const ByteField& waterDepth,
    std::uint64_t seed,
    LandformType landform
)
{
    const int width =
        map.width();

    const int height =
        map.height();

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
            const std::size_t i =
                index2D(
                    x,
                    y,
                    width
                );

            const int surfaceZ =
                surface[i];

            const int slope =
                localSlope(
                    surface,
                    width,
                    height,
                    x,
                    y
                );

            const double soilNoise =
                worldgen::fbm2D(
                    seed
                    ^
                    0xBE5466CF34E90C6CULL,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    22.0,
                    3,
                    0.5
                );

            const int soilDepth =
                soilDepthAt(
                    landform,
                    moisture[i],
                    slope,
                    soilNoise
                );

            const MaterialType soil =
                soilMaterialAt(
                    seed,
                    moisture[i],
                    temperature[i],
                    x,
                    y
                );

            const GeologyProvince province =
                geologyProvinceAt(
                    seed,
                    landform,
                    x,
                    y
                );

            for (
                int z = 0;
                z < map.depth();
                ++z
            )
            {
                Tile& tile =
                    map.at(
                        x,
                        y,
                        z
                    );

                // Reset every field in case generation is
                // called on an already-used map.
                tile =
                    Tile{};

                // ==========================================
                // Air
                // ==========================================

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

                // ==========================================
                // Surface
                // ==========================================

                if (
                    z ==
                    surfaceZ
                )
                {
                    tile.shape =
                        TileShape::Floor;

                    tile.material =
                        surfaceMaterialAt(
                            surface,
                            width,
                            height,
                            map.depth(),
                            seed,
                            landform,
                            moisture[i],
                            temperature[i],
                            x,
                            y
                        );

                    if (
                        waterDepth[i]
                        >
                        0
                    )
                    {
                        tile.liquid.type =
                            LiquidType::Water;

                        tile.liquid.depth =
                            waterDepth[i];
                    }

                    continue;
                }

                // ==========================================
                // Underground
                // ==========================================

                const int belowSurface =
                    surfaceZ
                    -
                    z;

                tile.shape =
                    TileShape::Wall;

                if (
                    belowSurface
                    <=
                    soilDepth
                )
                {
                    tile.material =
                        soil;
                }
                else
                {
                    tile.material =
                        bedrockMaterialAt(
                            province,
                            belowSurface
                            -
                            soilDepth
                            -
                            1,
                            x,
                            y,
                            width,
                            seed
                        );
                }
            }
        }
    }
}

// ==================================================
// Ore geology
//
// Deposits are not globally interchangeable.
//
// Different minerals prefer particular host rocks.
// ==================================================

[[nodiscard]]
bool hostAllowsOre(
    MaterialType ore,
    MaterialType host
)
{
    switch (ore)
    {
        case MaterialType::Coal:
        {
            return
                host ==
                    MaterialType::Shale
                ||
                host ==
                    MaterialType::Sandstone
                ||
                host ==
                    MaterialType::Limestone;
        }

        case MaterialType::IronOre:
        {
            return
                host ==
                    MaterialType::Shale
                ||
                host ==
                    MaterialType::Basalt
                ||
                host ==
                    MaterialType::Gabbro
                ||
                host ==
                    MaterialType::Slate;
        }

        case MaterialType::CopperOre:
        {
            return
                host ==
                    MaterialType::Basalt
                ||
                host ==
                    MaterialType::Gabbro
                ||
                host ==
                    MaterialType::Granite
                ||
                host ==
                    MaterialType::Limestone
                ||
                host ==
                    MaterialType::Dolomite;
        }

        case MaterialType::TinOre:
        {
            return
                host ==
                    MaterialType::Granite
                ||
                host ==
                    MaterialType::Gneiss
                ||
                host ==
                    MaterialType::Diorite;
        }

        case MaterialType::SilverOre:
        {
            return
                host ==
                    MaterialType::Limestone
                ||
                host ==
                    MaterialType::Dolomite
                ||
                host ==
                    MaterialType::Granite
                ||
                host ==
                    MaterialType::Gneiss;
        }

        case MaterialType::GoldOre:
        {
            return
                host ==
                    MaterialType::Granite
                ||
                host ==
                    MaterialType::Gneiss
                ||
                host ==
                    MaterialType::Schist;
        }

        case MaterialType::Quartz:
        {
            return
                host ==
                    MaterialType::Granite
                ||
                host ==
                    MaterialType::Gneiss
                ||
                host ==
                    MaterialType::Schist
                ||
                host ==
                    MaterialType::Slate
                ||
                host ==
                    MaterialType::Diorite;
        }

        default:
            return false;
    }
}

struct WeightedOre
{
    MaterialType material{
        MaterialType::None
    };

    int weight{};
};

[[nodiscard]]
MaterialType chooseOreForHost(
    MaterialType host,
    int depthBelowSurface,
    Random& random
)
{
    std::vector<WeightedOre>
        candidates;

    const auto add =
        [&](
            MaterialType material,
            int weight
        )
        {
            if (
                weight > 0
                &&
                hostAllowsOre(
                    material,
                    host
                )
            )
            {
                candidates.push_back(
                    WeightedOre{
                        material,
                        weight
                    }
                );
            }
        };

    add(
        MaterialType::Coal,
        34
    );

    add(
        MaterialType::IronOre,
        30
    );

    add(
        MaterialType::CopperOre,
        22
    );

    add(
        MaterialType::Quartz,
        25
    );

    add(
        MaterialType::TinOre,
        depthBelowSurface >= 5
        ?
        13
        :
        5
    );

    add(
        MaterialType::SilverOre,
        depthBelowSurface >= 6
        ?
        10
        :
        3
    );

    add(
        MaterialType::GoldOre,
        depthBelowSurface >= 8
        ?
        7
        :
        1
    );

    if (
        candidates.empty()
    )
    {
        return
            MaterialType::None;
    }

    int totalWeight =
        0;

    for (
        const WeightedOre candidate :
        candidates
    )
    {
        totalWeight +=
            candidate.weight;
    }

    int roll =
        random.integer(
            1,
            totalWeight
        );

    for (
        const WeightedOre candidate :
        candidates
    )
    {
        roll -=
            candidate.weight;

        if (
            roll <=
            0
        )
        {
            return
                candidate.material;
        }
    }

    return
        candidates.back().
            material;
}

// ==================================================
// Ore painting
// ==================================================

void paintOreSphere(
    GameMap& map,
    Position center,
    int radius,
    MaterialType ore
)
{
    for (
        int dz = -radius;
        dz <= radius;
        ++dz
    )
    {
        for (
            int dy = -radius;
            dy <= radius;
            ++dy
        )
        {
            for (
                int dx = -radius;
                dx <= radius;
                ++dx
            )
            {
                // Manhattan-ish noisy blob.
                if (
                    std::abs(dx)
                    +
                    std::abs(dy)
                    +
                    std::abs(dz)
                    >
                    radius
                    +
                    1
                )
                {
                    continue;
                }

                const Position position{
                    center.x
                    +
                    dx,
                    center.y
                    +
                    dy,
                    center.z
                    +
                    dz
                };

                if (
                    !map.inBounds(
                        position
                    )
                )
                {
                    continue;
                }

                Tile& tile =
                    map.at(
                        position
                    );

                if (
                    tile.shape !=
                        TileShape::Wall
                    ||
                    isSoilMaterial(
                        tile.material
                    )
                    ||
                    isOreMaterial(
                        tile.material
                    )
                    ||
                    !hostAllowsOre(
                        ore,
                        tile.material
                    )
                )
                {
                    continue;
                }

                tile.material =
                    ore;
            }
        }
    }
}

// ==================================================
// 3D wandering ore veins
// ==================================================

void generateOreVeins(
    GameMap& map,
    const HeightField& surface,
    std::uint64_t seed,
    Random& random,
    int requestedVeins
)
{
    const int width =
        map.width();

    const int height =
        map.height();

    int generated =
        0;

    int attempts =
        0;

    const int maximumAttempts =
        requestedVeins
        *
        12;

    while (
        generated <
            requestedVeins
        &&
        attempts <
            maximumAttempts
    )
    {
        ++attempts;

        const int x =
            random.integer(
                2,
                width - 3
            );

        const int y =
            random.integer(
                2,
                height - 3
            );

        const int surfaceZ =
            surface[
                index2D(
                    x,
                    y,
                    width
                )
            ];

        if (
            surfaceZ <=
            4
        )
        {
            continue;
        }

        const int z =
            random.integer(
                1,
                surfaceZ - 2
            );

        Tile& startTile =
            map.at(
                x,
                y,
                z
            );

        if (
            startTile.shape !=
                TileShape::Wall
            ||
            isSoilMaterial(
                startTile.material
            )
            ||
            isOreMaterial(
                startTile.material
            )
        )
        {
            continue;
        }

        const int depthBelowSurface =
            surfaceZ
            -
            z;

        const MaterialType ore =
            chooseOreForHost(
                startTile.material,
                depthBelowSurface,
                random
            );

        if (
            ore ==
            MaterialType::None
        )
        {
            continue;
        }

        Position current{
            x,
            y,
            z
        };

        int dx =
            random.integer(
                -1,
                1
            );

        int dy =
            random.integer(
                -1,
                1
            );

        int dz =
            random.integer(
                -1,
                1
            );

        if (
            dx == 0
            &&
            dy == 0
            &&
            dz == 0
        )
        {
            dx =
                1;
        }

        const int length =
            random.integer(
                6,
                18
            );

        for (
            int step = 0;
            step < length;
            ++step
        )
        {
            // Most vein segments are thin.
            //
            // Some swell into small deposits.
            const int radius =
                random.chance(
                    0.22
                )
                ?
                1
                :
                0;

            paintOreSphere(
                map,
                current,
                radius,
                ore
            );

            // Change direction every few cells to avoid
            // perfectly straight lines.
            if (
                step > 0
                &&
                step % 4 == 0
                &&
                random.chance(
                    0.55
                )
            )
            {
                dx =
                    std::clamp(
                        dx
                        +
                        random.integer(
                            -1,
                            1
                        ),
                        -1,
                        1
                    );

                dy =
                    std::clamp(
                        dy
                        +
                        random.integer(
                            -1,
                            1
                        ),
                        -1,
                        1
                    );

                dz =
                    std::clamp(
                        dz
                        +
                        random.integer(
                            -1,
                            1
                        ),
                        -1,
                        1
                    );

                if (
                    dx == 0
                    &&
                    dy == 0
                    &&
                    dz == 0
                )
                {
                    dx =
                        1;
                }
            }

            current.x =
                std::clamp(
                    current.x
                    +
                    dx,
                    1,
                    width - 2
                );

            current.y =
                std::clamp(
                    current.y
                    +
                    dy,
                    1,
                    height - 2
                );

            const int currentSurface =
                surface[
                    index2D(
                        current.x,
                        current.y,
                        width
                    )
                ];

            current.z =
                std::clamp(
                    current.z
                    +
                    dz,
                    1,
                    std::max(
                        1,
                        currentSurface - 2
                    )
                );
        }

        ++generated;
    }

    // Currently seed is represented through Random and
    // host geology. Keep the parameter available for
    // future stateless ore-noise modulation.
    (void)seed;
}

// ==================================================
// Caverns
// ==================================================

void carveCavernCell(
    GameMap& map,
    const HeightField& surface,
    int x,
    int y,
    int z
)
{
    // Keep a solid border around the simulation volume.
    if (
        x <= 1
        ||
        y <= 1
        ||
        x >=
            map.width()
            -
            2
        ||
        y >=
            map.height()
            -
            2
        ||
        z <= 0
        ||
        z >=
            map.depth()
            -
            1
    )
    {
        return;
    }

    const int surfaceZ =
        surface[
            index2D(
                x,
                y,
                map.width()
            )
        ];

    // Caverns should not accidentally break straight
    // through the surface.
    if (
        surfaceZ
        -
        z
        <
        4
    )
    {
        return;
    }

    Tile& tile =
        map.at(
            x,
            y,
            z
        );

    if (
        tile.shape !=
        TileShape::Wall
    )
    {
        return;
    }

    // Keep the host material.
    //
    // A limestone cavern remains limestone, a granite
    // cavern remains granite, and an ore vein intersected
    // by a cavern remains exposed as that ore material.
    tile.shape =
        TileShape::Floor;

    tile.feature =
        TileFeature::None;

    tile.featureMaterial =
        MaterialType::None;
}

void carveCavernChamber(
    GameMap& map,
    const HeightField& surface,
    std::uint64_t seed,
    Position center,
    int radiusX,
    int radiusY
)
{
    for (
        int y =
            std::max(
                2,
                center.y
                -
                radiusY
            );
        y <=
            std::min(
                map.height() - 3,
                center.y
                +
                radiusY
            );
        ++y
    )
    {
        for (
            int x =
                std::max(
                    2,
                    center.x
                    -
                    radiusX
                );
            x <=
                std::min(
                    map.width() - 3,
                    center.x
                    +
                    radiusX
                );
            ++x
        )
        {
            const double dx =
                static_cast<double>(
                    x
                    -
                    center.x
                )
                /
                static_cast<double>(
                    radiusX
                );

            const double dy =
                static_cast<double>(
                    y
                    -
                    center.y
                )
                /
                static_cast<double>(
                    radiusY
                );

            const double shape =
                dx * dx
                +
                dy * dy;

            const double noise =
                worldgen::fbm2D(
                    seed,
                    static_cast<double>(
                        x
                    ),
                    static_cast<double>(
                        y
                    ),
                    7.0,
                    3,
                    0.55
                );

            if (
                shape
                >
                1.0
                +
                noise
                *
                0.24
            )
            {
                continue;
            }

            carveCavernCell(
                map,
                surface,
                x,
                y,
                center.z
            );
        }
    }
}

void carveCavernTunnel(
    GameMap& map,
    const HeightField& surface,
    Position start,
    Position goal,
    Random& random
)
{
    Position current =
        start;

    const int safetyLimit =
        map.width()
        +
        map.height();

    for (
        int step = 0;
        step < safetyLimit;
        ++step
    )
    {
        carveCavernCell(
            map,
            surface,
            current.x,
            current.y,
            current.z
        );

        // Roughen the tunnel walls.
        if (
            random.chance(
                0.32
            )
        )
        {
            for (
                const Position direction :
                Cardinal
            )
            {
                if (
                    random.chance(
                        0.22
                    )
                )
                {
                    carveCavernCell(
                        map,
                        surface,
                        current.x
                        +
                        direction.x,
                        current.y
                        +
                        direction.y,
                        current.z
                    );
                }
            }
        }

        if (
            current.x ==
                goal.x
            &&
            current.y ==
                goal.y
        )
        {
            break;
        }

        const int deltaX =
            goal.x
            -
            current.x;

        const int deltaY =
            goal.y
            -
            current.y;

        const bool moveX =
            deltaX != 0
            &&
            (
                deltaY == 0
                ||
                random.chance(
                    0.5
                )
            );

        if (moveX)
        {
            current.x +=
                deltaX > 0
                ?
                1
                :
                -1;
        }
        else if (
            deltaY != 0
        )
        {
            current.y +=
                deltaY > 0
                ?
                1
                :
                -1;
        }
    }
}

void addCavernPools(
    GameMap& map,
    Position center,
    Random& random
)
{
    const int radiusX =
        random.integer(
            2,
            4
        );

    const int radiusY =
        random.integer(
            1,
            3
        );

    for (
        int y =
            center.y
            -
            radiusY;
        y <=
            center.y
            +
            radiusY;
        ++y
    )
    {
        for (
            int x =
                center.x
                -
                radiusX;
            x <=
                center.x
                +
                radiusX;
            ++x
        )
        {
            if (
                !map.inBounds(
                    x,
                    y,
                    center.z
                )
            )
            {
                continue;
            }

            const double dx =
                static_cast<double>(
                    x
                    -
                    center.x
                )
                /
                static_cast<double>(
                    radiusX
                );

            const double dy =
                static_cast<double>(
                    y
                    -
                    center.y
                )
                /
                static_cast<double>(
                    radiusY
                );

            if (
                dx * dx
                +
                dy * dy
                >
                1.0
            )
            {
                continue;
            }

            Tile& tile =
                map.at(
                    x,
                    y,
                    center.z
                );

            if (
                tile.shape !=
                TileShape::Floor
            )
            {
                continue;
            }

            tile.liquid.type =
                LiquidType::Water;

            tile.liquid.depth =
                static_cast<std::uint8_t>(
                    random.integer(
                        2,
                        6
                    )
                );
        }
    }
}

void generateCaverns(
    GameMap& map,
    const HeightField& surface,
    std::uint64_t seed,
    Random& random,
    int layerCount,
    int chambersPerLayer
)
{
    if (
        layerCount <=
        0
    )
    {
        return;
    }

    const int deepMin =
        2;

    const int deepMax =
        std::max(
            deepMin,
            map.depth() - 8
        );

    for (
        int layer = 0;
        layer < layerCount;
        ++layer
    )
    {
        const double fraction =
            static_cast<double>(
                layer + 1
            )
            /
            static_cast<double>(
                layerCount + 1
            );

        const int z =
            std::clamp(
                deepMin
                +
                static_cast<int>(
                    std::lround(
                        fraction
                        *
                        static_cast<double>(
                            deepMax
                            -
                            deepMin
                        )
                    )
                ),
                1,
                map.depth() - 2
            );

        std::vector<Position>
            centers;

        int attempts =
            0;

        while (
            static_cast<int>(
                centers.size()
            )
            <
            chambersPerLayer
            &&
            attempts
            <
            chambersPerLayer
            *
            20
        )
        {
            ++attempts;

            const int x =
                random.integer(
                    6,
                    map.width() - 7
                );

            const int y =
                random.integer(
                    5,
                    map.height() - 6
                );

            const int surfaceZ =
                surface[
                    index2D(
                        x,
                        y,
                        map.width()
                    )
                ];

            if (
                surfaceZ
                -
                z
                <
                5
            )
            {
                continue;
            }

            const Position center{
                x,
                y,
                z
            };

            centers.push_back(
                center
            );

            carveCavernChamber(
                map,
                surface,
                seed
                ^
                static_cast<std::uint64_t>(
                    layer
                )
                *
                0x9E3779B97F4A7C15ULL
                ^
                static_cast<std::uint64_t>(
                    centers.size()
                )
                *
                0xD1B54A32D192ED03ULL,
                center,
                random.integer(
                    3,
                    8
                ),
                random.integer(
                    2,
                    5
                )
            );
        }

        // Connect chambers into a contiguous cavern network.
        for (
            std::size_t i = 1;
            i < centers.size();
            ++i
        )
        {
            carveCavernTunnel(
                map,
                surface,
                centers[
                    i - 1
                ],
                centers[
                    i
                ],
                random
            );
        }

        if (
            !centers.empty()
            &&
            random.chance(
                0.70
            )
        )
        {
            addCavernPools(
                map,
                centers[
                    static_cast<std::size_t>(
                        random.integer(
                            0,
                            static_cast<int>(
                                centers.size()
                            )
                            -
                            1
                        )
                    )
                ],
                random
            );
        }
    }
}

// ==================================================
// Surface ramps
//
// Ramps are added where one surface tile directly
// borders terrain one Z-level higher.
//
// Your existing Pathfinder already understands these.
// ==================================================

void createSurfaceRamps(
    GameMap& map,
    const HeightField& surface
)
{
    const int width =
        map.width();

    const int height =
        map.height();

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
                    index2D(
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
                tile.shape !=
                TileShape::Floor
            )
            {
                continue;
            }

            for (
                const Position direction :
                Cardinal
            )
            {
                const int neighborZ =
                    surface[
                        index2D(
                            x
                            +
                            direction.x,
                            y
                            +
                            direction.y,
                            width
                        )
                    ];

                if (
                    neighborZ ==
                    z + 1
                )
                {
                    tile.shape =
                        TileShape::Ramp;

                    break;
                }
            }
        }
    }
}

// ==================================================
// Forest regions
//
// Trees depend on:
//
// - regional moisture
// - temperature
// - contiguous low-frequency noise
// - terrain slope
// - coarse climate
//
// This creates woodland patches rather than uniform
// independent random tree placement.
// ==================================================

void generateForests(
    GameMap& map,
    const HeightField& surface,
    const ScalarField& moisture,
    const ScalarField& temperature,
    std::uint64_t seed,
    ClimateType climate,
    double densityMultiplier
)
{
    const int width =
        map.width();

    const int height =
        map.height();

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
            const std::size_t i =
                index2D(
                    x,
                    y,
                    width
                );

            const int z =
                surface[i];

            Tile& tile =
                map.at(
                    x,
                    y,
                    z
                );

            if (
                tile.shape !=
                    TileShape::Floor
                ||
                tile.liquid.depth >
                    0
                ||
                tile.feature !=
                    TileFeature::None
            )
            {
                continue;
            }

            if (
                tile.material ==
                    MaterialType::Sand
                ||
                tile.material ==
                    MaterialType::Obsidian
                ||
                localSlope(
                    surface,
                    width,
                    height,
                    x,
                    y
                )
                >
                1
            )
            {
                continue;
            }

            const double patchNoise =
                (
                    worldgen::fbm2D(
                        seed
                        ^
                        0x3C6EF372FE94F82BULL,
                        static_cast<double>(
                            x
                        ),
                        static_cast<double>(
                            y
                        ),
                        17.0,
                        4,
                        0.56
                    )
                    +
                    1.0
                )
                *
                0.5;

            double suitability =
                moisture[i]
                *
                0.62
                +
                patchNoise
                *
                0.42;

            if (
                temperature[i]
                >
                0.80
            )
            {
                suitability -=
                    0.22;
            }

            if (
                temperature[i]
                <
                0.20
            )
            {
                suitability -=
                    0.18;
            }

            if (
                climate ==
                ClimateType::Dry
            )
            {
                suitability -=
                    0.03;
            }

            const double forestThreshold =
                climate ==
                    ClimateType::Dry
                ?
                0.36
                :
                0.49;

            const double probability =
                std::clamp(
                    (
                        suitability
                        -
                        forestThreshold
                    )
                    *
                    1.55
                    *
                    densityMultiplier,
                    0.0,
                    0.88
                );

            if (
                worldgen::unitHash(
                    seed
                    ^
                    0xB7E151628AED2A6BULL,
                    x,
                    y
                )
                >=
                probability
            )
            {
                continue;
            }

            tile.feature =
                TileFeature::Tree;

            const bool favorPine =
                climate ==
                    ClimateType::Boreal
                ||
                temperature[i]
                <
                0.44
                ||
                z
                >
                map.depth()
                *
                3
                /
                4;

            if (favorPine)
            {
                tile.featureMaterial =
                    worldgen::unitHash(
                        seed
                        ^
                        0xBF58476D1CE4E5B9ULL,
                        x,
                        y
                    )
                    <
                    0.82
                    ?
                    MaterialType::PineWood
                    :
                    MaterialType::OakWood;
            }
            else
            {
                tile.featureMaterial =
                    worldgen::unitHash(
                        seed
                        ^
                        0x94D049BB133111EBULL,
                        x,
                        y
                    )
                    <
                    0.72
                    ?
                    MaterialType::OakWood
                    :
                    MaterialType::PineWood;
            }
        }
    }
}

// ==================================================
// Embark evaluation
// ==================================================

[[nodiscard]]
int nearbyTreeCount(
    const GameMap& map,
    Position center,
    int radius
)
{
    int result =
        0;

    for (
        int y =
            std::max(
                0,
                center.y
                -
                radius
            );
        y <=
            std::min(
                map.height() - 1,
                center.y
                +
                radius
            );
        ++y
    )
    {
        for (
            int x =
                std::max(
                    0,
                    center.x
                    -
                    radius
                );
            x <=
                std::min(
                    map.width() - 1,
                    center.x
                    +
                    radius
                );
            ++x
        )
        {
            const int dx =
                x
                -
                center.x;

            const int dy =
                y
                -
                center.y;

            if (
                dx * dx
                +
                dy * dy
                >
                radius * radius
            )
            {
                continue;
            }

            // Surface elevations vary, so inspect a small
            // vertical band around the embark center.
            for (
                int z =
                    std::max(
                        0,
                        center.z - 2
                    );
                z <=
                    std::min(
                        map.depth() - 1,
                        center.z + 2
                    );
                ++z
            )
            {
                if (
                    map.at(
                        x,
                        y,
                        z
                    ).feature
                    ==
                    TileFeature::Tree
                )
                {
                    ++result;
                    break;
                }
            }
        }
    }

    return result;
}

[[nodiscard]]
bool spawnTileUsable(
    const GameMap& map,
    const HeightField& surface,
    int x,
    int y,
    int requiredZ
)
{
    if (
        x < 1
        ||
        y < 1
        ||
        x >=
            map.width() - 1
        ||
        y >=
            map.height() - 1
    )
    {
        return false;
    }

    if (
        surface[
            index2D(
                x,
                y,
                map.width()
            )
        ]
        !=
        requiredZ
    )
    {
        return false;
    }

    const Tile& tile =
        map.at(
            x,
            y,
            requiredZ
        );

    return
        tile.baseWalkable()
        &&
        tile.liquid.depth
        ==
        0;
}

// ==================================================
// Natural embark finder
//
// Worldgen no longer flattens an artificial starting
// platform.
//
// Instead it generates the whole world first and then
// scores naturally occurring locations based on:
//
// - nearby usable terrain
// - trees
// - fresh water at a useful distance
// - moisture
// - proximity to map center
//
// Only the three literal spawn cells are cleared after
// selection.
// ==================================================

[[nodiscard]]
GeneratedWorldLayout chooseEmbark(
    GameMap& map,
    const HeightField& surface,
    const ScalarField& moisture,
    const ByteField& waterDepth,
    LandformType landform,
    ClimateType climate
)
{
    const int width =
        map.width();

    const int height =
        map.height();

    const std::vector<int> waterDistance =
        distanceFromWater(
            waterDepth,
            width,
            height
        );

    Position best{
        width / 2,
        height / 2,
        surface[
            index2D(
                width / 2,
                height / 2,
                width
            )
        ]
    };

    double bestScore =
        -std::numeric_limits<
            double
        >::infinity();

    for (
        int y = 4;
        y < height - 5;
        ++y
    )
    {
        for (
            int x = 4;
            x < width - 5;
            ++x
        )
        {
            const int z =
                surface[
                    index2D(
                        x,
                        y,
                        width
                    )
                ];

            // Need three connected start cells at exactly
            // the same Z-level.
            if (
                !spawnTileUsable(
                    map,
                    surface,
                    x,
                    y,
                    z
                )
                ||
                !spawnTileUsable(
                    map,
                    surface,
                    x + 1,
                    y,
                    z
                )
                ||
                !spawnTileUsable(
                    map,
                    surface,
                    x,
                    y + 1,
                    z
                )
            )
            {
                continue;
            }

            int flatTiles =
                0;

            for (
                int oy = -3;
                oy <= 3;
                ++oy
            )
            {
                for (
                    int ox = -3;
                    ox <= 3;
                    ++ox
                )
                {
                    const int nx =
                        x + ox;

                    const int ny =
                        y + oy;

                    if (
                        nx < 0
                        ||
                        ny < 0
                        ||
                        nx >= width
                        ||
                        ny >= height
                    )
                    {
                        continue;
                    }

                    if (
                        std::abs(
                            surface[
                                index2D(
                                    nx,
                                    ny,
                                    width
                                )
                            ]
                            -
                            z
                        )
                        <=
                        1
                    )
                    {
                        ++flatTiles;
                    }
                }
            }

            const Position candidate{
                x,
                y,
                z
            };

            const int trees =
                nearbyTreeCount(
                    map,
                    candidate,
                    8
                );

            const int water =
                waterDistance[
                    index2D(
                        x,
                        y,
                        width
                    )
                ];

            double waterScore =
                0.0;

            // Near water is valuable, but spawning literally
            // on the river bank is less desirable.
            if (
                water >= 4
                &&
                water <= 18
            )
            {
                waterScore =
                    18.0;
            }
            else if (
                water < 4
            )
            {
                waterScore =
                    -8.0;
            }
            else if (
                water < 30
            )
            {
                waterScore =
                    7.0;
            }

            const double centerDistance =
                std::abs(
                    x
                    -
                    width / 2
                )
                +
                std::abs(
                    y
                    -
                    height / 2
                );

            const double score =
                static_cast<double>(
                    flatTiles
                )
                *
                1.35
                +
                static_cast<double>(
                    std::min(
                        trees,
                        28
                    )
                )
                *
                0.75
                +
                waterScore
                +
                moisture[
                    index2D(
                        x,
                        y,
                        width
                    )
                ]
                *
                10.0
                -
                centerDistance
                *
                0.08;

            if (
                score >
                bestScore
            )
            {
                bestScore =
                    score;

                best =
                    candidate;
            }
        }
    }

    // ==================================================
    // Clear only the exact three spawn cells.
    //
    // We do not flatten a giant embark platform.
    // ==================================================

    const std::array<
        Position,
        3
    > spawns{
        best,
        Position{
            best.x + 1,
            best.y,
            best.z
        },
        Position{
            best.x,
            best.y + 1,
            best.z
        }
    };

    for (
        const Position spawn :
        spawns
    )
    {
        Tile& tile =
            map.at(
                spawn
            );

        tile.shape =
            TileShape::Floor;

        tile.feature =
            TileFeature::None;

        tile.featureMaterial =
            MaterialType::None;

        tile.liquid =
            Liquid{};

        if (
            tile.material ==
            MaterialType::None
        )
        {
            tile.material =
                MaterialType::Grass;
        }
    }

    GeneratedWorldLayout result;

    result.defaultViewZ =
        best.z;

    result.embarkCenter =
        best;

    result.minerSpawn =
        spawns[0];

    result.haulerSpawn =
        spawns[1];

    result.woodcutterSpawn =
        spawns[2];

    result.landform =
        landform;

    result.climate =
        climate;

    return result;
}

}

// ==================================================
// Public generator
// ==================================================

GeneratedWorldLayout
WorldGenerator::generate(
    GameMap& map,
    std::uint64_t seed,
    const WorldGenConfig& config
)
{
    // ==================================================
    // Map requirements
    //
    // The old 60x28x12 test map does not provide enough
    // room for meaningful watersheds, mountain chains,
    // forests, geological strata and multiple caverns.
    // ==================================================

    if (
        map.width() <
            64
        ||
        map.height() <
            40
        ||
        map.depth() <
            16
    )
    {
        throw std::runtime_error(
            "Phase 5.5 world generation requires at least 64x40x16."
        );
    }

    // ==================================================
    // Config validation
    // ==================================================

    if (
        config.minimumRivers <
            0
        ||
        config.maximumRivers <
            config.minimumRivers
        ||
        config.minimumLakes <
            0
        ||
        config.maximumLakes <
            config.minimumLakes
        ||
        config.oreVeinCount <
            0
        ||
        config.cavernLayers <
            0
        ||
        config.cavernChambersPerLayer <
            0
        ||
        config.forestDensity <
            0.0
    )
    {
        throw std::runtime_error(
            "Invalid WorldGenConfig values."
        );
    }

    // World generation uses its own deterministic stream.
    //
    // Runtime simulation randomness remains independent.
    Random random{
        seed
        ^
        0x73E2A91C6B4D580FULL
    };

    // ==================================================
    // 1. Macro world identity
    // ==================================================

    const LandformType landform =
        chooseLandform(
            random
        );

    const ClimateType climate =
        chooseClimate(
            random
        );

    // ==================================================
    // 2. Surface elevation
    // ==================================================

    HeightField surface =
        generateElevation(
            map.width(),
            map.height(),
            map.depth(),
            seed,
            landform
        );

    // ==================================================
    // 3. Erosion
    // ==================================================

    erodeSurface(
        surface,
        map.width(),
        map.height(),
        map.depth(),
        config.erosionPasses
    );

    // ==================================================
    // 4. Climate fields
    // ==================================================

    ScalarField moisture;

    ScalarField temperature;

    generateClimateFields(
        surface,
        map.width(),
        map.height(),
        map.depth(),
        seed,
        climate,
        moisture,
        temperature
    );

    // ==================================================
    // 5. Hydrology
    // ==================================================

    ByteField surfaceWater(
        static_cast<std::size_t>(
            map.width()
            *
            map.height()
        ),
        0
    );

    int riverCount =
        random.integer(
            config.minimumRivers,
            config.maximumRivers
        );

    int lakeCount =
        random.integer(
            config.minimumLakes,
            config.maximumLakes
        );

    // Climate biases water abundance.
    if (
        climate ==
        ClimateType::Dry
    )
    {
        riverCount =
            std::max(
                0,
                riverCount - 1
            );

        lakeCount =
            std::max(
                0,
                lakeCount - 1
            );
    }
    else if (
        climate ==
        ClimateType::WetTemperate
    )
    {
        riverCount =
            std::min(
                config.maximumRivers + 1,
                riverCount + 1
            );

        lakeCount =
            std::min(
                config.maximumLakes + 1,
                lakeCount + 1
            );
    }

    generateRivers(
        surface,
        surfaceWater,
        map.width(),
        map.height(),
        map.depth(),
        seed,
        random,
        riverCount
    );

    generateLakes(
        surface,
        surfaceWater,
        map.width(),
        map.height(),
        map.depth(),
        seed,
        random,
        lakeCount
    );

    // Rivers/lakes now influence nearby ecology.
    addRiparianMoisture(
        moisture,
        surfaceWater,
        map.width(),
        map.height()
    );

    // ==================================================
    // 6. Convert elevation/climate/geology into actual
    //    3D tiles
    // ==================================================

    fillTerrainVolume(
        map,
        surface,
        moisture,
        temperature,
        surfaceWater,
        seed,
        landform
    );

    // ==================================================
    // 7. Mineral deposits
    //
    // Done after host geology exists because ore type
    // depends on the rock it forms within.
    // ==================================================

    generateOreVeins(
        map,
        surface,
        seed,
        random,
        config.oreVeinCount
    );

    // ==================================================
    // 8. Caverns
    //
    // Caverns can expose the ore/geology generated above.
    // ==================================================

    generateCaverns(
        map,
        surface,
        seed,
        random,
        config.cavernLayers,
        config.cavernChambersPerLayer
    );

    // ==================================================
    // 9. Natural surface traversal
    // ==================================================

    createSurfaceRamps(
        map,
        surface
    );

    // ==================================================
    // 10. Vegetation
    //
    // This happens after water generation so riparian
    // moisture affects forest distribution.
    // ==================================================

    generateForests(
        map,
        surface,
        moisture,
        temperature,
        seed,
        climate,
        config.forestDensity
    );

    // ==================================================
    // 11. Embark selection
    //
    // Find a starting area in the generated environment
    // rather than creating the environment around a
    // predetermined starting area.
    // ==================================================

    return
        chooseEmbark(
            map,
            surface,
            moisture,
            surfaceWater,
            landform,
            climate
        );
}

}

#include "ascii/systems/WaterSystem.hpp"

#include "ascii/Position.hpp"
#include "ascii/Tile.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ascii::systems
{

namespace
{

constexpr int MaxWaterDepth =
    7;

std::size_t index(
    const GameMap& map,
    Position position
)
{
    return
        static_cast<
            std::size_t
        >(
            (
                position.z
                *
                map.height()
                +
                position.y
            )
            *
            map.width()
            +
            position.x
        );
}

bool canReceiveWater(
    const GameMap& map,
    Position position
)
{
    return
        map.inBounds(
            position
        )
        &&
        map.at(
            position
        ).canHoldLiquid();
}

}

void updateWater(
    GameMap& map
)
{
    const std::size_t count =
        map.cellCount();

    std::vector<int>
        current(
            count,
            0
        );

    // Snapshot current water levels.
    for (
        int z = 0;
        z < map.depth();
        ++z
    )
    {
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
                const Position position{
                    x,
                    y,
                    z
                };

                current[
                    index(
                        map,
                        position
                    )
                ] =
                    map.at(
                        position
                    ).liquid.depth;
            }
        }
    }

    // ==================================================
    // Gravity/downhill pass
    // ==================================================

    std::vector<int>
        verticalDelta(
            count,
            0
        );

    constexpr std::array<
        Position,
        4
    > horizontal{
        Position{0, -1, 0},
        Position{1, 0, 0},
        Position{0, 1, 0},
        Position{-1, 0, 0}
    };

    for (
        int z =
            map.depth() - 1;
        z >= 0;
        --z
    )
    {
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
                const Position source{
                    x,
                    y,
                    z
                };

                const auto sourceIndex =
                    index(
                        map,
                        source
                    );

                const int available =
                    current[
                        sourceIndex
                    ];

                if (
                    available <= 0
                )
                {
                    continue;
                }

                std::vector<Position>
                    lowerCandidates;

                const Tile& sourceTile =
                    map.at(source);

                // Straight down stairs / shafts.
                if (
                    z > 0
                    &&
                    (
                        sourceTile.
                            hasDownConnection()
                        ||
                        sourceTile.shape ==
                            TileShape::Open
                    )
                )
                {
                    const Position below{
                        x,
                        y,
                        z - 1
                    };

                    if (
                        canReceiveWater(
                            map,
                            below
                        )
                    )
                    {
                        lowerCandidates.
                            push_back(
                                below
                            );
                    }
                }

                // Downhill over ramps.
                if (
                    z > 0
                )
                {
                    for (
                        const auto direction :
                        horizontal
                    )
                    {
                        const Position lower{
                            x +
                                direction.x,

                            y +
                                direction.y,

                            z - 1
                        };

                        if (
                            canReceiveWater(
                                map,
                                lower
                            )
                            &&
                            map.at(
                                lower
                            ).shape ==
                                TileShape::Ramp
                        )
                        {
                            lowerCandidates.
                                push_back(
                                    lower
                                );
                        }
                    }
                }

                if (
                    lowerCandidates.empty()
                )
                {
                    continue;
                }

                auto destination =
                    lowerCandidates.front();

                int bestDepth =
                    current[
                        index(
                            map,
                            destination
                        )
                    ];

                for (
                    const auto candidate :
                    lowerCandidates
                )
                {
                    const int depth =
                        current[
                            index(
                                map,
                                candidate
                            )
                        ];

                    if (
                        depth <
                        bestDepth
                    )
                    {
                        destination =
                            candidate;

                        bestDepth =
                            depth;
                    }
                }

                const auto destIndex =
                    index(
                        map,
                        destination
                    );

                const int capacity =
                    MaxWaterDepth
                    -
                    (
                        current[
                            destIndex
                        ]
                        +
                        verticalDelta[
                            destIndex
                        ]
                    );

                if (
                    capacity <= 0
                )
                {
                    continue;
                }

                const int transfer =
                    std::min(
                        available,
                        capacity
                    );

                verticalDelta[
                    sourceIndex
                ] -=
                    transfer;

                verticalDelta[
                    destIndex
                ] +=
                    transfer;
            }
        }
    }

    std::vector<int>
        stage =
            current;

    for (
        std::size_t i = 0;
        i < count;
        ++i
    )
    {
        stage[i] =
            std::clamp(
                stage[i]
                +
                verticalDelta[i],
                0,
                MaxWaterDepth
            );
    }

    // ==================================================
    // Horizontal equalization pass
    // ==================================================

    std::vector<int>
        horizontalDelta(
            count,
            0
        );

    for (
        int z = 0;
        z < map.depth();
        ++z
    )
    {
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
                const Position source{
                    x,
                    y,
                    z
                };

                if (
                    !map.at(
                        source
                    ).
                    supportsHorizontalLiquid()
                )
                {
                    continue;
                }

                const auto sourceIndex =
                    index(
                        map,
                        source
                    );

                const int sourceDepth =
                    stage[
                        sourceIndex
                    ];

                if (
                    sourceDepth <= 1
                )
                {
                    continue;
                }

                bool found =
                    false;

                Position best{};

                int bestDepth =
                    sourceDepth;

                for (
                    const auto direction :
                    horizontal
                )
                {
                    const Position neighbor{
                        x +
                            direction.x,

                        y +
                            direction.y,

                        z
                    };

                    if (
                        !map.inBounds(
                            neighbor
                        )
                        ||
                        !map.at(
                            neighbor
                        ).
                        supportsHorizontalLiquid()
                    )
                    {
                        continue;
                    }

                    const int neighborDepth =
                        stage[
                            index(
                                map,
                                neighbor
                            )
                        ];

                    if (
                        neighborDepth <
                        bestDepth
                    )
                    {
                        found =
                            true;

                        best =
                            neighbor;

                        bestDepth =
                            neighborDepth;
                    }
                }

                if (
                    !found
                    ||
                    sourceDepth <=
                        bestDepth + 1
                )
                {
                    continue;
                }

                const auto bestIndex =
                    index(
                        map,
                        best
                    );

                if (
                    stage[
                        bestIndex
                    ]
                    +
                    horizontalDelta[
                        bestIndex
                    ]
                    >=
                    MaxWaterDepth
                )
                {
                    continue;
                }

                horizontalDelta[
                    sourceIndex
                ] -=
                    1;

                horizontalDelta[
                    bestIndex
                ] +=
                    1;
            }
        }
    }

    // ==================================================
    // Commit
    // ==================================================

    for (
        int z = 0;
        z < map.depth();
        ++z
    )
    {
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
                const Position position{
                    x,
                    y,
                    z
                };

                const auto i =
                    index(
                        map,
                        position
                    );

                const int depth =
                    std::clamp(
                        stage[i]
                        +
                        horizontalDelta[i],
                        0,
                        MaxWaterDepth
                    );

                auto& liquid =
                    map.at(
                        position
                    ).liquid;

                liquid.depth =
                    static_cast<
                        std::uint8_t
                    >(depth);

                liquid.type =
                    depth > 0
                    ?
                    LiquidType::Water
                    :
                    LiquidType::None;
            }
        }
    }
}

}

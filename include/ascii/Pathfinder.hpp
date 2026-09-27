#pragma once

#include "GameMap.hpp"
#include "Position.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <queue>
#include <vector>

namespace ascii
{

class Pathfinder
{
public:
    [[nodiscard]]
    std::optional<
        std::vector<Position>
    >
    findPath(
        const GameMap& map,
        Position start,
        Position goal
    ) const
    {
        if (
            !map.inBounds(start)
            ||
            !map.inBounds(goal)
        )
        {
            return std::nullopt;
        }

        if (
            !map.at(start).walkable()
            ||
            !map.at(goal).walkable()
        )
        {
            return std::nullopt;
        }

        if (
            start == goal
        )
        {
            return
                std::vector<Position>{};
        }

        const std::size_t size =
            map.cellCount();

        constexpr int Infinite =
            std::numeric_limits<
                int
            >::max();

        constexpr std::size_t
            NoParent =
                std::numeric_limits<
                    std::size_t
                >::max();

        std::vector<int>
            gScore(
                size,
                Infinite
            );

        std::vector<std::size_t>
            cameFrom(
                size,
                NoParent
            );

        std::vector<std::uint8_t>
            closed(
                size,
                0
            );

        std::priority_queue<
            OpenNode,
            std::vector<OpenNode>,
            Compare
        > open;

        const auto startIndex =
            index(
                start,
                map
            );

        const auto goalIndex =
            index(
                goal,
                map
            );

        gScore[startIndex] =
            0;

        std::uint64_t order =
            0;

        const int startH =
            heuristic(
                start,
                goal
            );

        open.push(
            OpenNode{
                start,
                0,
                startH,
                startH,
                order++
            }
        );

        while (!open.empty())
        {
            const OpenNode current =
                open.top();

            open.pop();

            const auto currentIndex =
                index(
                    current.position,
                    map
                );

            if (
                current.g !=
                gScore[currentIndex]
                ||
                closed[
                    currentIndex
                ]
            )
            {
                continue;
            }

            if (
                current.position ==
                goal
            )
            {
                return reconstruct(
                    cameFrom,
                    startIndex,
                    goalIndex,
                    map
                );
            }

            closed[currentIndex] =
                1;

            const auto tryNeighbor =
                [&](
                    Position neighbor
                )
                {
                    if (
                        !map.inBounds(
                            neighbor
                        )
                        ||
                        !map.at(
                            neighbor
                        ).walkable()
                    )
                    {
                        return;
                    }

                    const auto neighborIndex =
                        index(
                            neighbor,
                            map
                        );

                    if (
                        closed[
                            neighborIndex
                        ]
                    )
                    {
                        return;
                    }

                    const int tentativeG =
                        current.g + 1;

                    if (
                        tentativeG >=
                        gScore[
                            neighborIndex
                        ]
                    )
                    {
                        return;
                    }

                    cameFrom[
                        neighborIndex
                    ] =
                        currentIndex;

                    gScore[
                        neighborIndex
                    ] =
                        tentativeG;

                    const int h =
                        heuristic(
                            neighbor,
                            goal
                        );

                    open.push(
                        OpenNode{
                            neighbor,
                            tentativeG,
                            h,
                            tentativeG + h,
                            order++
                        }
                    );
                };

            // ==========================================
            // Same Z
            // ==========================================

            constexpr std::array<
                Position,
                4
            > directions{
                Position{0, -1, 0},
                Position{1, 0, 0},
                Position{0, 1, 0},
                Position{-1, 0, 0}
            };

            for (
                const auto direction :
                directions
            )
            {
                tryNeighbor(
                    Position{
                        current.position.x
                            + direction.x,

                        current.position.y
                            + direction.y,

                        current.position.z
                    }
                );
            }

            const Tile& currentTile =
                map.at(
                    current.position
                );

            // ==========================================
            // Stairs
            // ==========================================

            if (
                currentTile.
                    hasUpConnection()
            )
            {
                const Position above{
                    current.position.x,
                    current.position.y,
                    current.position.z + 1
                };

                if (
                    map.inBounds(above)
                    &&
                    map.at(
                        above
                    ).hasDownConnection()
                )
                {
                    tryNeighbor(
                        above
                    );
                }
            }

            if (
                currentTile.
                    hasDownConnection()
            )
            {
                const Position below{
                    current.position.x,
                    current.position.y,
                    current.position.z - 1
                };

                if (
                    map.inBounds(below)
                    &&
                    map.at(
                        below
                    ).hasUpConnection()
                )
                {
                    tryNeighbor(
                        below
                    );
                }
            }

            // ==========================================
            // Ramps
            // ==========================================

            if (
                currentTile.shape ==
                    TileShape::Ramp
            )
            {
                for (
                    const auto direction :
                    directions
                )
                {
                    tryNeighbor(
                        Position{
                            current.position.x
                                + direction.x,

                            current.position.y
                                + direction.y,

                            current.position.z + 1
                        }
                    );
                }
            }

            // Descend onto a lower ramp.
            for (
                const auto direction :
                directions
            )
            {
                const Position lower{
                    current.position.x
                        + direction.x,

                    current.position.y
                        + direction.y,

                    current.position.z - 1
                };

                if (
                    map.inBounds(lower)
                    &&
                    map.at(
                        lower
                    ).shape ==
                        TileShape::Ramp
                )
                {
                    tryNeighbor(
                        lower
                    );
                }
            }
        }

        return std::nullopt;
    }

private:
    struct OpenNode
    {
        Position position{};

        int g{};
        int h{};
        int f{};

        std::uint64_t order{};
    };

    struct Compare
    {
        bool operator()(
            const OpenNode& lhs,
            const OpenNode& rhs
        ) const
        {
            if (
                lhs.f != rhs.f
            )
            {
                return
                    lhs.f > rhs.f;
            }

            if (
                lhs.h != rhs.h
            )
            {
                return
                    lhs.h > rhs.h;
            }

            return
                lhs.order >
                rhs.order;
        }
    };

    [[nodiscard]]
    static int heuristic(
        Position a,
        Position b
    )
    {
        return
            std::abs(
                a.x - b.x
            )
            +
            std::abs(
                a.y - b.y
            )
            +
            std::abs(
                a.z - b.z
            );
    }

    [[nodiscard]]
    static std::size_t index(
        Position position,
        const GameMap& map
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

    [[nodiscard]]
    static Position
    positionFromIndex(
        std::size_t value,
        const GameMap& map
    )
    {
        const std::size_t
            width =
                static_cast<
                    std::size_t
                >(map.width());

        const std::size_t
            height =
                static_cast<
                    std::size_t
                >(map.height());

        const std::size_t
            layerSize =
                width * height;

        const int z =
            static_cast<int>(
                value /
                layerSize
            );

        const auto withinLayer =
            value %
            layerSize;

        const int y =
            static_cast<int>(
                withinLayer /
                width
            );

        const int x =
            static_cast<int>(
                withinLayer %
                width
            );

        return Position{
            x,
            y,
            z
        };
    }

    [[nodiscard]]
    static std::optional<
        std::vector<Position>
    >
    reconstruct(
        const std::vector<
            std::size_t
        >& cameFrom,
        std::size_t startIndex,
        std::size_t goalIndex,
        const GameMap& map
    )
    {
        constexpr std::size_t
            NoParent =
                std::numeric_limits<
                    std::size_t
                >::max();

        std::vector<Position>
            path;

        std::size_t current =
            goalIndex;

        while (
            current !=
            startIndex
        )
        {
            path.push_back(
                positionFromIndex(
                    current,
                    map
                )
            );

            current =
                cameFrom[
                    current
                ];

            if (
                current ==
                NoParent
            )
            {
                return std::nullopt;
            }
        }

        std::reverse(
            path.begin(),
            path.end()
        );

        return path;
    }
};

}

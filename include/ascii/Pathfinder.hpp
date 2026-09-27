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
    std::optional<std::vector<Position>>
    findPath(
        const GameMap& map,
        Position start,
        Position goal
    ) const
    {
        if (
            !map.inBounds(start) ||
            !map.inBounds(goal)
        )
        {
            return std::nullopt;
        }

        if (
            !map.at(start).walkable() ||
            !map.at(goal).walkable()
        )
        {
            return std::nullopt;
        }

        if (start == goal)
        {
            return std::vector<Position>{};
        }

        constexpr int Infinite =
            std::numeric_limits<int>::max();

        constexpr std::size_t NoParent =
            std::numeric_limits<std::size_t>::max();

        std::vector<int> gScore(
            map.cellCount(),
            Infinite
        );

        std::vector<std::size_t> cameFrom(
            map.cellCount(),
            NoParent
        );

        std::vector<std::uint8_t> closed(
            map.cellCount(),
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

        const int h =
            heuristic(
                start,
                goal
            );

        open.push(
            OpenNode{
                start,
                0,
                h,
                h,
                order++
            }
        );

        constexpr std::array<Position, 4>
            directions{
                Position{0, -1, 0},
                Position{1, 0, 0},
                Position{0, 1, 0},
                Position{-1, 0, 0}
            };

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
                current.g != gScore[currentIndex] ||
                closed[currentIndex]
            )
            {
                continue;
            }

            if (
                current.position == goal
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
                        !map.inBounds(neighbor) ||
                        !map.at(neighbor).walkable()
                    )
                    {
                        return;
                    }

                    const auto neighborIndex =
                        index(
                            neighbor,
                            map
                        );

                    if (closed[neighborIndex])
                    {
                        return;
                    }

                    const int tentative =
                        current.g + 1;

                    if (
                        tentative >=
                        gScore[neighborIndex]
                    )
                    {
                        return;
                    }

                    cameFrom[neighborIndex] =
                        currentIndex;

                    gScore[neighborIndex] =
                        tentative;

                    const int neighborH =
                        heuristic(
                            neighbor,
                            goal
                        );

                    open.push(
                        OpenNode{
                            neighbor,
                            tentative,
                            neighborH,
                            tentative +
                                neighborH,
                            order++
                        }
                    );
                };

            // Same Z-level.
            for (
                const auto direction :
                directions
            )
            {
                tryNeighbor(
                    Position{
                        current.position.x +
                            direction.x,
                        current.position.y +
                            direction.y,
                        current.position.z
                    }
                );
            }

            const Tile& currentTile =
                map.at(
                    current.position
                );

            // Stairs up.
            if (
                currentTile.hasUpConnection()
            )
            {
                const Position above{
                    current.position.x,
                    current.position.y,
                    current.position.z + 1
                };

                if (
                    map.inBounds(above) &&
                    map.at(above).
                        hasDownConnection()
                )
                {
                    tryNeighbor(above);
                }
            }

            // Stairs down.
            if (
                currentTile.hasDownConnection()
            )
            {
                const Position below{
                    current.position.x,
                    current.position.y,
                    current.position.z - 1
                };

                if (
                    map.inBounds(below) &&
                    map.at(below).
                        hasUpConnection()
                )
                {
                    tryNeighbor(below);
                }
            }

            // Ramp upward.
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
                            current.position.x +
                                direction.x,
                            current.position.y +
                                direction.y,
                            current.position.z + 1
                        }
                    );
                }
            }

            // Step downward onto a lower ramp.
            if (
                current.position.z > 0
            )
            {
                for (
                    const auto direction :
                    directions
                )
                {
                    const Position lower{
                        current.position.x +
                            direction.x,
                        current.position.y +
                            direction.y,
                        current.position.z - 1
                    };

                    if (
                        map.inBounds(lower) &&
                        map.at(lower).shape ==
                            TileShape::Ramp
                    )
                    {
                        tryNeighbor(lower);
                    }
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
            if (lhs.f != rhs.f)
            {
                return lhs.f > rhs.f;
            }

            if (lhs.h != rhs.h)
            {
                return lhs.h > rhs.h;
            }

            return lhs.order > rhs.order;
        }
    };

    [[nodiscard]]
    static int heuristic(
        Position a,
        Position b
    )
    {
        return
            std::abs(a.x - b.x) +
            std::abs(a.y - b.y) +
            std::abs(a.z - b.z);
    }

    [[nodiscard]]
    static std::size_t index(
        Position position,
        const GameMap& map
    )
    {
        return
            (
                static_cast<std::size_t>(
                    position.z
                ) *
                static_cast<std::size_t>(
                    map.height()
                ) +
                static_cast<std::size_t>(
                    position.y
                )
            ) *
            static_cast<std::size_t>(
                map.width()
            ) +
            static_cast<std::size_t>(
                position.x
            );
    }

    [[nodiscard]]
    static Position positionFromIndex(
        std::size_t value,
        const GameMap& map
    )
    {
        const std::size_t width =
            static_cast<std::size_t>(
                map.width()
            );

        const std::size_t height =
            static_cast<std::size_t>(
                map.height()
            );

        const std::size_t layerSize =
            width * height;

        const int z =
            static_cast<int>(
                value /
                layerSize
            );

        const std::size_t within =
            value %
            layerSize;

        const int y =
            static_cast<int>(
                within /
                width
            );

        const int x =
            static_cast<int>(
                within %
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
        const std::vector<std::size_t>&
            cameFrom,
        std::size_t startIndex,
        std::size_t goalIndex,
        const GameMap& map
    )
    {
        constexpr std::size_t NoParent =
            std::numeric_limits<std::size_t>::max();

        std::vector<Position> path;

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
                cameFrom[current];

            if (
                current == NoParent
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

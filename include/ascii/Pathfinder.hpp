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
    // Find a path from start to goal.
    //
    // Returned path:
    //
    //     DOES NOT contain the start tile.
    //     DOES contain the goal tile.
    //
    // Example:
    //
    // start = (1, 1)
    // goal  = (4, 1)
    //
    // result:
    //
    // (2,1)
    // (3,1)
    // (4,1)
    //
    [[nodiscard]]
    std::optional<std::vector<Position>>
    findPath(
        const GameMap& map,
        Position start,
        Position goal
    ) const
    {
        if (
            !map.inBounds(start.x, start.y) ||
            !map.inBounds(goal.x, goal.y)
        )
        {
            return std::nullopt;
        }

        if (
            !map.at(start.x, start.y).walkable() ||
            !map.at(goal.x, goal.y).walkable()
        )
        {
            return std::nullopt;
        }

        if (start == goal)
        {
            return std::vector<Position>{};
        }

        const int width = map.width();
        const int height = map.height();

        const std::size_t mapSize =
            static_cast<std::size_t>(
                width * height
            );

        constexpr int InfiniteCost =
            std::numeric_limits<int>::max();

        constexpr std::size_t NoParent =
            std::numeric_limits<std::size_t>::max();

        // Lowest known cost from start -> tile.
        std::vector<int> gScore(
            mapSize,
            InfiniteCost
        );

        // Used to reconstruct the final path.
        std::vector<std::size_t> cameFrom(
            mapSize,
            NoParent
        );

        // Tiles already fully evaluated.
        std::vector<std::uint8_t> closed(
            mapSize,
            0
        );

        std::priority_queue<
            OpenNode,
            std::vector<OpenNode>,
            OpenNodeCompare
        > open;

        const std::size_t startIndex =
            index(start, width);

        const std::size_t goalIndex =
            index(goal, width);

        gScore[startIndex] = 0;

        std::uint64_t insertionOrder = 0;

        open.push(
            OpenNode{
                start,
                0,
                heuristic(start, goal),
                heuristic(start, goal),
                insertionOrder++
            }
        );

        // Fixed neighbor order is deliberate.
        //
        // This helps make equal-cost path choices
        // deterministic.
        constexpr std::array<Position, 4> directions{
            Position{0, -1}, // north
            Position{1, 0},  // east
            Position{0, 1},  // south
            Position{-1, 0}  // west
        };

        while (!open.empty())
        {
            const OpenNode current =
                open.top();

            open.pop();

            const std::size_t currentIndex =
                index(
                    current.position,
                    width
                );

            // Ignore stale priority-queue entries.
            if (
                current.g !=
                gScore[currentIndex]
            )
            {
                continue;
            }

            if (closed[currentIndex])
            {
                continue;
            }

            // Goal reached.
            if (
                current.position ==
                goal
            )
            {
                return reconstructPath(
                    cameFrom,
                    startIndex,
                    goalIndex,
                    width
                );
            }

            closed[currentIndex] = 1;

            for (const Position direction :
                 directions)
            {
                const Position neighbor{
                    current.position.x
                        + direction.x,

                    current.position.y
                        + direction.y
                };

                if (
                    !map.inBounds(
                        neighbor.x,
                        neighbor.y
                    )
                )
                {
                    continue;
                }

                if (
                    !map.at(
                        neighbor.x,
                        neighbor.y
                    ).walkable()
                )
                {
                    continue;
                }

                const std::size_t neighborIndex =
                    index(
                        neighbor,
                        width
                    );

                if (closed[neighborIndex])
                {
                    continue;
                }

                // Every tile currently costs 1.
                const int tentativeG =
                    current.g + 1;

                if (
                    tentativeG >=
                    gScore[neighborIndex]
                )
                {
                    continue;
                }

                cameFrom[neighborIndex] =
                    currentIndex;

                gScore[neighborIndex] =
                    tentativeG;

                const int h =
                    heuristic(
                        neighbor,
                        goal
                    );

                const int f =
                    tentativeG + h;

                open.push(
                    OpenNode{
                        neighbor,
                        tentativeG,
                        h,
                        f,
                        insertionOrder++
                    }
                );
            }
        }

        // No route exists.
        return std::nullopt;
    }

private:
    struct OpenNode
    {
        Position position;

        int g{};
        int h{};
        int f{};

        std::uint64_t insertionOrder{};
    };

    struct OpenNodeCompare
    {
        bool operator()(
            const OpenNode& lhs,
            const OpenNode& rhs
        ) const
        {
            // Lowest f wins.
            if (lhs.f != rhs.f)
            {
                return lhs.f > rhs.f;
            }

            // If f is equal, prefer the node
            // closer to the destination.
            if (lhs.h != rhs.h)
            {
                return lhs.h > rhs.h;
            }

            // Final deterministic tie breaker.
            return
                lhs.insertionOrder >
                rhs.insertionOrder;
        }
    };

    [[nodiscard]]
    static int heuristic(
        Position a,
        Position b
    )
    {
        // Manhattan distance for 4-direction movement.
        return
            std::abs(a.x - b.x)
            +
            std::abs(a.y - b.y);
    }

    [[nodiscard]]
    static std::size_t index(
        Position position,
        int width
    )
    {
        return static_cast<std::size_t>(
            position.y * width
            +
            position.x
        );
    }

    [[nodiscard]]
    static Position positionFromIndex(
        std::size_t indexValue,
        int width
    )
    {
        return Position{
            static_cast<int>(
                indexValue %
                static_cast<std::size_t>(width)
            ),

            static_cast<int>(
                indexValue /
                static_cast<std::size_t>(width)
            )
        };
    }

    [[nodiscard]]
    static std::optional<std::vector<Position>>
    reconstructPath(
        const std::vector<std::size_t>& cameFrom,
        std::size_t startIndex,
        std::size_t goalIndex,
        int width
    )
    {
        constexpr std::size_t NoParent =
            std::numeric_limits<std::size_t>::max();

        std::vector<Position> path;

        std::size_t current =
            goalIndex;

        while (current != startIndex)
        {
            path.push_back(
                positionFromIndex(
                    current,
                    width
                )
            );

            current =
                cameFrom[current];

            if (current == NoParent)
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

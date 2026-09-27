#include "ascii/WorldGenerator.hpp"

#include "ascii/Random.hpp"
#include "ascii/Tile.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ascii
{

namespace
{

std::size_t index(
    int x,
    int y,
    int width
)
{
    return static_cast<
        std::size_t
    >(
        y * width + x
    );
}

int surroundingWalls(
    const std::vector<TileType>&
        tiles,
    int width,
    int height,
    int x,
    int y
)
{
    int count =
        0;

    for (
        int dy = -1;
        dy <= 1;
        ++dy
    )
    {
        for (
            int dx = -1;
            dx <= 1;
            ++dx
        )
        {
            if (
                dx == 0
                &&
                dy == 0
            )
            {
                continue;
            }

            const int nx =
                x + dx;

            const int ny =
                y + dy;

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
                ++count;
                continue;
            }

            if (
                tiles[
                    index(
                        nx,
                        ny,
                        width
                    )
                ]
                ==
                TileType::Wall
            )
            {
                ++count;
            }
        }
    }

    return count;
}

void setTile(
    GameMap& map,
    int x,
    int y,
    TileType type
)
{
    if (
        map.inBounds(
            x,
            y
        )
    )
    {
        map.at(
            x,
            y
        ).type =
            type;
    }
}

void carveRectangle(
    GameMap& map,
    int left,
    int top,
    int right,
    int bottom
)
{
    for (
        int y = top;
        y <= bottom;
        ++y
    )
    {
        for (
            int x = left;
            x <= right;
            ++x
        )
        {
            setTile(
                map,
                x,
                y,
                TileType::Floor
            );
        }
    }
}

}

GeneratedWorldLayout
WorldGenerator::generate(
    GameMap& map,
    std::uint64_t seed,
    const WorldGenConfig& config
)
{
    const int width =
        map.width();

    const int height =
        map.height();

    if (
        width < 30
        ||
        height < 18
    )
    {
        throw std::runtime_error(
            "WorldGenerator requires at least "
            "a 30x18 map."
        );
    }

    // Separate world-generation stream.
    //
    // Runtime simulation randomness remains
    // independent from terrain generation.
    Random random{
        seed ^
        0xA57C9E3B4D128F61ULL
    };

    std::vector<TileType>
        current(
            static_cast<std::size_t>(
                width * height
            ),
            TileType::Floor
        );

    // ==================================================
    // Initial random noise
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
            const bool border =
                x == 0
                ||
                y == 0
                ||
                x == width - 1
                ||
                y == height - 1;

            if (border)
            {
                current[
                    index(
                        x,
                        y,
                        width
                    )
                ] =
                    TileType::Wall;

                continue;
            }

            const int roll =
                random.integer(
                    0,
                    99
                );

            current[
                index(
                    x,
                    y,
                    width
                )
            ] =
                roll <
                    config.
                        initialWallPercent
                ?
                TileType::Wall
                :
                TileType::Floor;
        }
    }

    // ==================================================
    // Cellular automata smoothing
    // ==================================================

    for (
        int pass = 0;
        pass <
            config.smoothingPasses;
        ++pass
    )
    {
        std::vector<TileType>
            next =
                current;

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
                const int neighbors =
                    surroundingWalls(
                        current,
                        width,
                        height,
                        x,
                        y
                    );

                next[
                    index(
                        x,
                        y,
                        width
                    )
                ] =
                    neighbors >= 5
                    ?
                    TileType::Wall
                    :
                    TileType::Floor;
            }
        }

        current =
            std::move(next);
    }

    // ==================================================
    // Copy generated terrain into map
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
            map.at(
                x,
                y
            ).type =
                current[
                    index(
                        x,
                        y,
                        width
                    )
                ];
        }
    }

    // ==================================================
    // Guaranteed playable colony area
    // ==================================================

    const int centerY =
        height / 2;

    const int mineX =
        width - 10;

    // Starting room.
    carveRectangle(
        map,
        2,
        centerY - 4,
        11,
        centerY + 4
    );

    // Corridor toward mine.
    carveRectangle(
        map,
        10,
        centerY - 1,
        mineX - 1,
        centerY + 1
    );

    // Wider access along the mine face.
    carveRectangle(
        map,
        mineX - 2,
        centerY - 4,
        mineX - 1,
        centerY + 4
    );

    // ==================================================
    // Guaranteed rock vein
    // ==================================================

    for (
        int y = centerY - 4;
        y <= centerY + 4;
        ++y
    )
    {
        for (
            int x = mineX;
            x <=
                std::min(
                    mineX + 4,
                    width - 2
                );
            ++x
        )
        {
            setTile(
                map,
                x,
                y,
                TileType::Wall
            );
        }
    }

    // Ensure outer boundary remains solid.
    for (
        int x = 0;
        x < width;
        ++x
    )
    {
        setTile(
            map,
            x,
            0,
            TileType::Wall
        );

        setTile(
            map,
            x,
            height - 1,
            TileType::Wall
        );
    }

    for (
        int y = 0;
        y < height;
        ++y
    )
    {
        setTile(
            map,
            0,
            y,
            TileType::Wall
        );

        setTile(
            map,
            width - 1,
            y,
            TileType::Wall
        );
    }

    // ==================================================
    // Layout metadata
    // ==================================================

    GeneratedWorldLayout result;

    result.minerSpawn =
        Position{
            5,
            centerY
        };

    result.haulerSpawn =
        Position{
            5,
            centerY + 2
        };

    result.stockpileTopLeft =
        Position{
            3,
            centerY - 3
        };

    result.stockpileBottomRight =
        Position{
            9,
            centerY - 1
        };

    result.miningTargets = {
        Position{
            mineX,
            centerY - 2
        },
        Position{
            mineX,
            centerY - 1
        },
        Position{
            mineX,
            centerY
        },
        Position{
            mineX,
            centerY + 1
        }
    };

    // Stockpile must be floor.
    carveRectangle(
        map,
        result.stockpileTopLeft.x,
        result.stockpileTopLeft.y,
        result.stockpileBottomRight.x,
        result.stockpileBottomRight.y
    );

    return result;
}

}

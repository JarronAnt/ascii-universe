#pragma once

#include "Position.hpp"
#include "Tile.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

namespace ascii
{

class GameMap
{
public:
    GameMap(
        int width,
        int height,
        int depth
    )
        : width_(width),
          height_(height),
          depth_(depth),
          tiles_(
              static_cast<std::size_t>(
                  width
                  *
                  height
                  *
                  depth
              )
          )
    {
    }

    [[nodiscard]]
    int width() const
    {
        return width_;
    }

    [[nodiscard]]
    int height() const
    {
        return height_;
    }

    [[nodiscard]]
    int depth() const
    {
        return depth_;
    }

    [[nodiscard]]
    int topZ() const
    {
        return depth_ - 1;
    }

    [[nodiscard]]
    std::size_t cellCount() const
    {
        return tiles_.size();
    }

    [[nodiscard]]
    bool inBounds(
        int x,
        int y,
        int z
    ) const
    {
        return
            x >= 0
            &&
            y >= 0
            &&
            z >= 0
            &&
            x < width_
            &&
            y < height_
            &&
            z < depth_;
    }

    [[nodiscard]]
    bool inBounds(
        Position position
    ) const
    {
        return inBounds(
            position.x,
            position.y,
            position.z
        );
    }

    Tile& at(
        int x,
        int y,
        int z
    )
    {
        assert(
            inBounds(
                x,
                y,
                z
            )
        );

        return
            tiles_[
                index(
                    x,
                    y,
                    z
                )
            ];
    }

    const Tile& at(
        int x,
        int y,
        int z
    ) const
    {
        assert(
            inBounds(
                x,
                y,
                z
            )
        );

        return
            tiles_[
                index(
                    x,
                    y,
                    z
                )
            ];
    }

    Tile& at(
        Position position
    )
    {
        return at(
            position.x,
            position.y,
            position.z
        );
    }

    const Tile& at(
        Position position
    ) const
    {
        return at(
            position.x,
            position.y,
            position.z
        );
    }

private:
    [[nodiscard]]
    std::size_t index(
        int x,
        int y,
        int z
    ) const
    {
        return
            static_cast<std::size_t>(
                (
                    z * height_
                    +
                    y
                )
                *
                width_
                +
                x
            );
    }

    int width_{};
    int height_{};
    int depth_{};

    std::vector<Tile>
        tiles_;
};

}

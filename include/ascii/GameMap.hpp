#pragma once

#include "Tile.hpp"

#include <cassert>
#include <vector>

namespace ascii
{

    //game map object
    class GameMap
    {
    public:
        GameMap(int width, int height)
            : width_(width),
            height_(height),
            tiles_(static_cast<std::size_t>(width * height))
    {}
    
    //get width and height
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

    //check of in bounds of map
    [[nodiscard]]
    bool inBounds(int x, int y) const
    {
        return
            x >= 0 &&
            y >= 0 &&
            x < width_ &&
            y < height_;
    }
    
    //get tile at positon const and mutable overload
    Tile& at(int x, int y)
    {
        assert(inBounds(x, y));
        return tiles_[index(x, y)];
    }

    const Tile& at(int x, int y) const
    {
        assert(inBounds(x, y));
        return tiles_[index(x, y)];
    }

private:
    //2D to 1D conversion
    [[nodiscard]]
    std::size_t index(int x, int y) const
    {
        return static_cast<std::size_t>(
            y * width_ + x
        );
    }

    int width_;
    int height_;

    std::vector<Tile> tiles_;
};

}

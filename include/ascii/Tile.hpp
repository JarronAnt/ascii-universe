#pragma once

namespace ascii
{

    enum class TileType
    {
        Floor,
        Wall
    };

    struct Tile
    {
        //give var type default val of floor
        TileType type{TileType::Floor};
        
        //compiler warning
        [[nodiscard]]
        bool walkable() const
        {
            //check if tile type is floor thus
            //walkable
            return type == TileType::Floor;
        }
    };

}

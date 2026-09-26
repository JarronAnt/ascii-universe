#include "ascii/GameMap.hpp"
#include <iostream>

using namespace ascii;

int main()
{
    //create game map
    GameMap map{40, 20};
    
    //set the edge of the map to wall tiles
    //NOTE: default is floor tiles so no need to account for that in this portion of the code
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            if (
                x == 0 ||
                y == 0 ||
                x == map.width() - 1 ||
                y == map.height() - 1
            )
            {
                map.at(x, y).type = TileType::Wall;
            }
        }
    }
    //draw # or . based on wall or floor tiles
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            switch (map.at(x, y).type)
            {
                case TileType::Floor:
                    std::cout << "\033[36m.\033[0m";
                    break;

                case TileType::Wall:
                    std::cout << '#';
                    break;
            }
        }

        std::cout << '\n';
    }
}

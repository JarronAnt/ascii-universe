#include "ascii/GameMap.hpp"
#include "ascii/TerminalRenderer.hpp"
#include "ascii/Components.hpp"

#include <entt/entt.hpp>

using namespace ascii;

int main()
{
    GameMap map{40, 20};

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

    entt::registry registry;
    auto goblin = registry.create();
    registry.emplace<Goblin>(goblin);

    registry.emplace<Name>(goblin, "Uru");
    registry.emplace<Position>(goblin,10,8);
    registry.emplace<Glyph>(goblin, 'g');

    TerminalRenderer renderer;

    renderer.render(map, registry);
}

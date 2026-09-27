#pragma once

#include "Components.hpp"
#include "GameMap.hpp"

#include <entt/entt.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace ascii
{

class TerminalRenderer
{
public:
    void clear()
    {
        // Clear terminal and move cursor to top-left.
        std::cout << "\033[2J\033[H";
    }

    void render(
        const GameMap& map,
        entt::registry& registry
    )
    {
        clear();

        // --------------------------------------------------
        // Create framebuffer
        // --------------------------------------------------

        std::vector<std::string> buffer(
            static_cast<std::size_t>(map.height()),
            std::string(
                static_cast<std::size_t>(map.width()),
                ' '
            )
        );

        // --------------------------------------------------
        // Render map tiles into framebuffer
        // --------------------------------------------------

        for (int y = 0; y < map.height(); ++y)
        {
            for (int x = 0; x < map.width(); ++x)
            {
                switch (map.at(x, y).type)
                {
                    case TileType::Floor:
                        buffer[y][x] = '.';
                        break;

                    case TileType::Wall:
                        buffer[y][x] = '#';
                        break;
                }
            }
        }

        // --------------------------------------------------
        // Render ECS entities into framebuffer
        // --------------------------------------------------

        auto view = registry.view<Position, Glyph>();

        for (auto entity : view)
        {
            const auto& position =
                view.get<Position>(entity);

            const auto& glyph =
                view.get<Glyph>(entity);

            // Make sure the entity is actually on the map.
            if (!map.inBounds(position.x, position.y))
            {
                continue;
            }

            buffer[position.y][position.x] =
                glyph.character;
        }

        // --------------------------------------------------
        // Present framebuffer
        // --------------------------------------------------

        for (const auto& row : buffer)
        {
            std::cout << row << '\n';
        }

        std::cout.flush();
    }
};

}

#pragma once

#include "Color.hpp"
#include "GameMap.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace ascii
{

class TerminalRenderer
{
public:
    TerminalRenderer() = default;

    ~TerminalRenderer();

    TerminalRenderer(
        const TerminalRenderer&
    ) = delete;

    TerminalRenderer&
    operator=(
        const TerminalRenderer&
    ) = delete;

    void render(
        const GameMap& map,
        entt::registry& registry,
        const std::vector<std::string>&
            hudLines = {}
    );

    // Restore terminal state and leave the cursor
    // beneath the rendered game.
    void finish();

private:
    struct Cell
    {
        char character{' '};

        TerminalColor color{
            TerminalColor::Default
        };

        friend bool operator==(
            const Cell&,
            const Cell&
        ) = default;
    };

    void initializeScreen(
        std::size_t width,
        std::size_t height
    );

    std::vector<Cell>
        previousFrame_;

    std::size_t frameWidth_{0};

    std::size_t frameHeight_{0};

    bool initialized_{false};

    bool finished_{false};
};

}

#include "ascii/TerminalRenderer.hpp"

#include "ascii/Components.hpp"
#include "ascii/Position.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

namespace ascii
{

TerminalRenderer::~TerminalRenderer()
{
    finish();
}

void TerminalRenderer::initializeScreen(
    std::size_t width,
    std::size_t height
)
{
    frameWidth_ =
        width;

    frameHeight_ =
        height;

    previousFrame_.assign(
        frameWidth_ *
            frameHeight_,
        Cell{
            '\0',
            TerminalColor::Default
        }
    );

    // Clear only when starting or when the frame
    // dimensions have changed.
    std::cout
        << "\033[2J"
        << "\033[H"
        << "\033[?25l";

    std::cout.flush();

    initialized_ =
        true;

    finished_ =
        false;
}

void TerminalRenderer::render(
    const GameMap& map,
    entt::registry& registry,
    const std::vector<std::string>&
        hudLines
)
{
    // ==================================================
    // Determine frame size
    // ==================================================

    std::size_t width =
        static_cast<std::size_t>(
            map.width()
        );

    for (
        const auto& line :
        hudLines
    )
    {
        width =
            std::max(
                width,
                line.size()
            );
    }

    // Map + blank separator + HUD.
    const std::size_t height =
        static_cast<std::size_t>(
            map.height()
        )
        +
        1
        +
        hudLines.size();

    if (
        !initialized_
        ||
        frameWidth_ != width
        ||
        frameHeight_ != height
    )
    {
        initializeScreen(
            width,
            height
        );
    }

    finished_ =
        false;

    // Keep cursor hidden during gameplay.
    std::cout
        << "\033[?25l";

    // ==================================================
    // Back buffer
    // ==================================================

    std::vector<Cell>
        nextFrame(
            width * height
        );

    const auto setCell =
        [&](
            std::size_t x,
            std::size_t y,
            char character,
            TerminalColor color
        )
        {
            if (
                x >= width
                ||
                y >= height
            )
            {
                return;
            }

            nextFrame[
                y * width + x
            ] = Cell{
                character,
                color
            };
        };

    // ==================================================
    // Terrain
    // ==================================================

    for (
        int y = 0;
        y < map.height();
        ++y
    )
    {
        for (
            int x = 0;
            x < map.width();
            ++x
        )
        {
            switch (
                map.at(
                    x,
                    y
                ).type
            )
            {
                case TileType::Floor:
                    setCell(
                        static_cast<
                            std::size_t
                        >(x),
                        static_cast<
                            std::size_t
                        >(y),
                        '.',
                        TerminalColor::
                            BrightBlack
                    );

                    break;

                case TileType::Wall:
                    setCell(
                        static_cast<
                            std::size_t
                        >(x),
                        static_cast<
                            std::size_t
                        >(y),
                        '#',
                        TerminalColor::
                            BrightWhite
                    );

                    break;
            }
        }
    }

    // ==================================================
    // Stockpiles
    //
    // Stockpile floor overlays normal floor.
    // ==================================================

    auto stockpileView =
        registry.view<
            Stockpile
        >();

    std::vector<entt::entity>
        stockpiles;

    for (
        auto entity :
        stockpileView
    )
    {
        stockpiles.push_back(
            entity
        );
    }

    std::sort(
        stockpiles.begin(),
        stockpiles.end(),
        [](
            entt::entity first,
            entt::entity second
        )
        {
            return
                entt::to_integral(
                    first
                )
                <
                entt::to_integral(
                    second
                );
        }
    );

    for (
        auto entity :
        stockpiles
    )
    {
        const auto& stockpile =
            registry.get<
                Stockpile
            >(entity);

        for (
            int y =
                stockpile.bounds.
                    topLeft.y;
            y <=
                stockpile.bounds.
                    bottomRight.y;
            ++y
        )
        {
            for (
                int x =
                    stockpile.bounds.
                        topLeft.x;
                x <=
                    stockpile.bounds.
                        bottomRight.x;
                ++x
            )
            {
                if (
                    !map.inBounds(
                        x,
                        y
                    )
                    ||
                    !map.at(
                        x,
                        y
                    ).walkable()
                )
                {
                    continue;
                }

                setCell(
                    static_cast<
                        std::size_t
                    >(x),
                    static_cast<
                        std::size_t
                    >(y),
                    '=',
                    TerminalColor::
                        BrightCyan
                );
            }
        }
    }

    // ==================================================
    // Entities
    //
    // Entities overlay terrain and stockpiles.
    // ==================================================

    auto entityView =
        registry.view<
            Position,
            Glyph
        >();

    std::vector<entt::entity>
        entities;

    for (
        auto entity :
        entityView
    )
    {
        entities.push_back(
            entity
        );
    }

    // Keep overlay order deterministic.
    std::sort(
        entities.begin(),
        entities.end(),
        [](
            entt::entity first,
            entt::entity second
        )
        {
            return
                entt::to_integral(
                    first
                )
                <
                entt::to_integral(
                    second
                );
        }
    );

    for (
        auto entity :
        entities
    )
    {
        const auto& position =
            registry.get<
                Position
            >(entity);

        const auto& glyph =
            registry.get<
                Glyph
            >(entity);

        if (
            !map.inBounds(
                position.x,
                position.y
            )
        )
        {
            continue;
        }

        setCell(
            static_cast<
                std::size_t
            >(position.x),
            static_cast<
                std::size_t
            >(position.y),
            glyph.character,
            glyph.color
        );
    }

    // ==================================================
    // HUD
    // ==================================================

    const std::size_t hudStart =
        static_cast<std::size_t>(
            map.height()
        )
        +
        1;

    for (
        std::size_t lineIndex = 0;
        lineIndex <
            hudLines.size();
        ++lineIndex
    )
    {
        const auto& line =
            hudLines[
                lineIndex
            ];

        for (
            std::size_t x = 0;
            x < line.size();
            ++x
        )
        {
            setCell(
                x,
                hudStart +
                    lineIndex,
                line[x],
                TerminalColor::
                    BrightWhite
            );
        }
    }

    // ==================================================
    // Dirty-cell render
    // ==================================================

    std::ostringstream output;

    TerminalColor activeColor =
        TerminalColor::Default;

    for (
        std::size_t y = 0;
        y < height;
        ++y
    )
    {
        for (
            std::size_t x = 0;
            x < width;
            ++x
        )
        {
            const std::size_t index =
                y * width + x;

            const Cell& next =
                nextFrame[index];

            const Cell& previous =
                previousFrame_[index];

            if (
                next == previous
            )
            {
                continue;
            }

            // ANSI cursor position is 1-based.
            output
                << "\033["
                << (y + 1)
                << ';'
                << (x + 1)
                << 'H';

            if (
                next.color !=
                    activeColor
            )
            {
                output
                    << "\033["
                    << ansiForegroundCode(
                        next.color
                    )
                    << 'm';

                activeColor =
                    next.color;
            }

            output
                << next.character;
        }
    }

    // Don't leak our text color into the shell.
    output
        << "\033[0m";

    std::cout
        << output.str();

    std::cout.flush();

    previousFrame_ =
        std::move(
            nextFrame
        );
}

void TerminalRenderer::finish()
{
    if (finished_)
    {
        return;
    }

    if (initialized_)
    {
        // Put shell cursor below our rendered frame.
        std::cout
            << "\033[0m"
            << "\033[?25h"
            << "\033["
            << (frameHeight_ + 1)
            << ";1H"
            << '\n';
    }
    else
    {
        std::cout
            << "\033[0m"
            << "\033[?25h";
    }

    std::cout.flush();

    finished_ =
        true;
}

}

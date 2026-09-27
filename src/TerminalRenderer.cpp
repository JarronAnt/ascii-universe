#include "ascii/TerminalRenderer.hpp"

#include "ascii/Components.hpp"
#include "ascii/Material.hpp"
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

    // Enter the terminal's alternate screen buffer.
    //
    // This keeps the game completely separate from the
    // normal shell screen. When finish() is called, the
    // original shell contents are restored.
    std::cout
        << "\033[?1049h"

        // Clear alternate screen.
        << "\033[2J"

        // Move cursor to top-left.
        << "\033[H"

        // Hide cursor.
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
    int viewZ,
    const std::vector<std::string>&
        hudLines
)
{
    if (
        viewZ < 0 ||
        viewZ >= map.depth()
    )
    {
        return;
    }

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

    const std::size_t height =
        static_cast<std::size_t>(
            map.height()
        )
        +
        1
        +
        hudLines.size();

    if (
        !initialized_ ||
        frameWidth_ != width ||
        frameHeight_ != height
    )
    {
        if (initialized_)
        {
            finish();
        }

        initializeScreen(
            width,
            height
        );
    }

    finished_ =
        false;

    // Keep the cursor hidden while the simulation
    // is being rendered.
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
                x >= width ||
                y >= height
            )
            {
                return;
            }

            nextFrame[
                y * width + x
            ] =
                Cell{
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
            const Tile& tile =
                map.at(
                    x,
                    y,
                    viewZ
                );

            char character =
                ' ';

            TerminalColor color =
                materialColor(
                    tile.material
                );

            // ------------------------------------------
            // Water
            // ------------------------------------------

            if (
                tile.liquid.type ==
                    LiquidType::Water
                &&
                tile.liquid.depth > 0
            )
            {
                // Show water depth directly.
                //
                // DF-style:
                //
                // 1 = shallow
                // 7 = full
                character =
                    static_cast<char>(
                        '0' +
                        tile.liquid.depth
                    );

                color =
                    tile.liquid.depth >= 5
                    ?
                    TerminalColor::BrightBlue
                    :
                    TerminalColor::Cyan;
            }

            // ------------------------------------------
            // Trees
            // ------------------------------------------

            else if (
                tile.feature ==
                    TileFeature::Tree
            )
            {
                character =
                    'T';

                color =
                    materialColor(
                        tile.featureMaterial
                    );
            }

            // ------------------------------------------
            // Terrain geometry
            // ------------------------------------------

            else
            {
                switch (
                    tile.shape
                )
                {
                    case TileShape::Open:
                        character =
                            ' ';

                        color =
                            TerminalColor::Default;

                        break;

                    case TileShape::Floor:
                        character =
                            '.';
                        break;

                    case TileShape::Wall:
                        character =
                            '#';
                        break;

                    case TileShape::Ramp:
                        character =
                            '^';
                        break;

                    case TileShape::UpStair:
                        character =
                            '<';
                        break;

                    case TileShape::DownStair:
                        character =
                            '>';
                        break;

                    case TileShape::UpDownStair:
                        character =
                            'X';
                        break;
                }
            }

            setCell(
                static_cast<std::size_t>(
                    x
                ),
                static_cast<std::size_t>(
                    y
                ),
                character,
                color
            );
        }
    }

    // ==================================================
    // Stockpiles
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
            entt::entity lhs,
            entt::entity rhs
        )
        {
            return
                entt::to_integral(
                    lhs
                )
                <
                entt::to_integral(
                    rhs
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

        if (
            viewZ <
                stockpile.bounds.min.z
            ||
            viewZ >
                stockpile.bounds.max.z
        )
        {
            continue;
        }

        for (
            int y =
                stockpile.bounds.min.y;
            y <=
                stockpile.bounds.max.y;
            ++y
        )
        {
            for (
                int x =
                    stockpile.bounds.min.x;
                x <=
                    stockpile.bounds.max.x;
                ++x
            )
            {
                const Position position{
                    x,
                    y,
                    viewZ
                };

                if (
                    !map.inBounds(
                        position
                    )
                )
                {
                    continue;
                }

                const Tile& tile =
                    map.at(
                        position
                    );

                if (
                    !tile.baseWalkable()
                    ||
                    tile.feature !=
                        TileFeature::None
                    ||
                    tile.liquid.depth > 0
                )
                {
                    continue;
                }

                setCell(
                    static_cast<std::size_t>(
                        x
                    ),
                    static_cast<std::size_t>(
                        y
                    ),
                    '=',
                    TerminalColor::BrightCyan
                );
            }
        }
    }

    // ==================================================
    // ECS entities
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

    std::sort(
        entities.begin(),
        entities.end(),
        [](
            entt::entity lhs,
            entt::entity rhs
        )
        {
            return
                entt::to_integral(
                    lhs
                )
                <
                entt::to_integral(
                    rhs
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

        if (
            position.z !=
                viewZ
        )
        {
            continue;
        }

        if (
            !map.inBounds(
                position
            )
        )
        {
            continue;
        }

        const auto& glyph =
            registry.get<
                Glyph
            >(entity);

        setCell(
            static_cast<std::size_t>(
                position.x
            ),
            static_cast<std::size_t>(
                position.y
            ),
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
                TerminalColor::BrightWhite
            );
        }
    }

    // ==================================================
    // Dirty-cell renderer
    //
    // Only redraw characters that changed from the
    // previous frame.
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
                nextFrame[
                    index
                ];

            const Cell& previous =
                previousFrame_[
                    index
                ];

            if (
                next == previous
            )
            {
                continue;
            }

            // ANSI coordinates are 1-based.
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

    // Restore default terminal styling.
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
    if (
        finished_
    )
    {
        return;
    }

    if (
        initialized_
    )
    {
        // Restore styles.
        std::cout
            << "\033[0m"

            // Show cursor.
            << "\033[?25h"

            // Leave alternate screen buffer.
            //
            // This restores the terminal exactly as
            // it looked before ASCII Universe started.
            << "\033[?1049l";
    }
    else
    {
        std::cout
            << "\033[0m"
            << "\033[?25h";
    }

    std::cout.flush();

    initialized_ =
        false;

    finished_ =
        true;
}

}

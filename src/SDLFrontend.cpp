#include "ascii/SDLFrontend.hpp"

#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Material.hpp"
#include "ascii/Position.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace ascii
{

namespace
{

constexpr int CellSize =
    SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;

constexpr int Margin =
    8;

struct GlyphVisual
{
    char character{' '};

    SDL_Color color{
        255,
        255,
        255,
        255
    };
};

// ==================================================
// Color conversion
// ==================================================

SDL_Color toSDLColor(
    TerminalColor color
)
{
    switch (color)
    {
        case TerminalColor::Black:
            return {
                20,
                22,
                26,
                255
            };

        case TerminalColor::Red:
            return {
                205,
                70,
                70,
                255
            };

        case TerminalColor::Green:
            return {
                80,
                180,
                100,
                255
            };

        case TerminalColor::Yellow:
            return {
                205,
                170,
                80,
                255
            };

        case TerminalColor::Blue:
            return {
                70,
                110,
                210,
                255
            };

        case TerminalColor::Magenta:
            return {
                175,
                90,
                205,
                255
            };

        case TerminalColor::Cyan:
            return {
                70,
                185,
                195,
                255
            };

        case TerminalColor::White:
            return {
                200,
                205,
                215,
                255
            };

        case TerminalColor::BrightBlack:
            return {
                90,
                95,
                105,
                255
            };

        case TerminalColor::BrightRed:
            return {
                255,
                90,
                90,
                255
            };

        case TerminalColor::BrightGreen:
            return {
                105,
                235,
                125,
                255
            };

        case TerminalColor::BrightYellow:
            return {
                245,
                210,
                90,
                255
            };

        case TerminalColor::BrightBlue:
            return {
                90,
                145,
                255,
                255
            };

        case TerminalColor::BrightMagenta:
            return {
                220,
                115,
                255,
                255
            };

        case TerminalColor::BrightCyan:
            return {
                95,
                230,
                240,
                255
            };

        case TerminalColor::BrightWhite:
            return {
                245,
                245,
                250,
                255
            };

        case TerminalColor::Default:
        default:
            return {
                200,
                205,
                215,
                255
            };
    }
}

SDL_Color dimColor(
    SDL_Color color
)
{
    constexpr float Factor =
        0.32F;

    color.r =
        static_cast<std::uint8_t>(
            static_cast<float>(
                color.r
            )
            *
            Factor
        );

    color.g =
        static_cast<std::uint8_t>(
            static_cast<float>(
                color.g
            )
            *
            Factor
        );

    color.b =
        static_cast<std::uint8_t>(
            static_cast<float>(
                color.b
            )
            *
            Factor
        );

    return color;
}

// ==================================================
// Tile appearance
// ==================================================

GlyphVisual visualForTile(
    const Tile& tile
)
{
    GlyphVisual visual;

    visual.color =
        toSDLColor(
            materialColor(
                tile.material
            )
        );

    // ----------------------------------------------
    // Water
    // ----------------------------------------------

    if (
        tile.liquid.type ==
            LiquidType::Water
        &&
        tile.liquid.depth > 0
    )
    {
        visual.character =
            static_cast<char>(
                '0'
                +
                tile.liquid.depth
            );

        visual.color =
            tile.liquid.depth >= 5
            ?
            toSDLColor(
                TerminalColor::
                    BrightBlue
            )
            :
            toSDLColor(
                TerminalColor::
                    Cyan
            );

        return visual;
    }

    // ----------------------------------------------
    // Trees
    // ----------------------------------------------

    if (
        tile.feature ==
            TileFeature::Tree
    )
    {
        visual.character =
            'T';

        visual.color =
            toSDLColor(
                materialColor(
                    tile.featureMaterial
                )
            );

        return visual;
    }

    // ----------------------------------------------
    // Geometry
    // ----------------------------------------------

    switch (
        tile.shape
    )
    {
        case TileShape::Open:
            visual.character =
                ' ';

            visual.color =
                toSDLColor(
                    TerminalColor::
                        Default
                );

            break;

        case TileShape::Floor:
            visual.character =
                '.';
            break;

        case TileShape::Wall:
            visual.character =
                '#';
            break;

        case TileShape::Ramp:
            visual.character =
                '^';
            break;

        case TileShape::UpStair:
            visual.character =
                '<';
            break;

        case TileShape::DownStair:
            visual.character =
                '>';
            break;

        case TileShape::UpDownStair:
            visual.character =
                'X';
            break;
    }

    return visual;
}

// ==================================================
// Dwarf-Fortress-style "look through open air"
//
// When the current Z cell is open, faintly show the
// immediately lower Z-level.
//
// This makes slopes/cliffs much easier to understand
// than having large completely black gaps.
// ==================================================

GlyphVisual visualAt(
    const GameMap& map,
    int x,
    int y,
    int z
)
{
    const Tile& tile =
        map.at(
            x,
            y,
            z
        );

    if (
        tile.shape !=
            TileShape::Open
    )
    {
        return visualForTile(
            tile
        );
    }

    if (
        z <= 0
    )
    {
        return GlyphVisual{};
    }

    const Tile& below =
        map.at(
            x,
            y,
            z - 1
        );

    if (
        below.shape ==
            TileShape::Open
    )
    {
        return GlyphVisual{};
    }

    GlyphVisual visual =
        visualForTile(
            below
        );

    visual.color =
        dimColor(
            visual.color
        );

    return visual;
}

// ==================================================
// SDL drawing helpers
// ==================================================

void setDrawColor(
    SDL_Renderer* renderer,
    SDL_Color color
)
{
    SDL_SetRenderDrawColor(
        renderer,
        color.r,
        color.g,
        color.b,
        color.a
    );
}

void drawGlyph(
    SDL_Renderer* renderer,
    float x,
    float y,
    char character,
    SDL_Color color
)
{
    if (
        character == ' '
    )
    {
        return;
    }

    setDrawColor(
        renderer,
        color
    );

    const char text[]{
        character,
        '\0'
    };

    SDL_RenderDebugText(
        renderer,
        x,
        y,
        text
    );
}

void drawText(
    SDL_Renderer* renderer,
    float x,
    float y,
    const std::string& text,
    SDL_Color color
)
{
    setDrawColor(
        renderer,
        color
    );

    SDL_RenderDebugText(
        renderer,
        x,
        y,
        text.c_str()
    );
}

}

// ==================================================
// Construction
// ==================================================

SDLFrontend::SDLFrontend(
    int windowWidth,
    int windowHeight
)
{
    if (
        !SDL_Init(
            SDL_INIT_VIDEO
        )
    )
    {
        throw std::runtime_error(
            std::string{
                "SDL_Init failed: "
            }
            +
            SDL_GetError()
        );
    }

    const SDL_WindowFlags flags =
        SDL_WINDOW_RESIZABLE
        |
        SDL_WINDOW_HIGH_PIXEL_DENSITY;

    if (
        !SDL_CreateWindowAndRenderer(
            "ASCII Universe",
            windowWidth,
            windowHeight,
            flags,
            &window_,
            &renderer_
        )
    )
    {
        const std::string error =
            SDL_GetError();

        SDL_Quit();

        throw std::runtime_error(
            "SDL window creation failed: "
            +
            error
        );
    }

    // Try to synchronize rendering to the display.
    // Not all render backends guarantee support, so
    // failure here is harmless.
    SDL_SetRenderVSync(
        renderer_,
        1
    );

    // Keep our 8x8 pixel glyphs sharp when the
    // logical canvas is enlarged.
    SDL_SetDefaultTextureScaleMode(
        renderer_,
        SDL_SCALEMODE_NEAREST
    );

    SDL_SetRenderDrawBlendMode(
        renderer_,
        SDL_BLENDMODE_BLEND
    );
}

// ==================================================
// Destruction
// ==================================================

SDLFrontend::~SDLFrontend()
{
    if (
        renderer_ != nullptr
    )
    {
        SDL_DestroyRenderer(
            renderer_
        );

        renderer_ =
            nullptr;
    }

    if (
        window_ != nullptr
    )
    {
        SDL_DestroyWindow(
            window_
        );

        window_ =
            nullptr;
    }

    SDL_Quit();
}

// ==================================================
// Input
// ==================================================

FrontendActions
SDLFrontend::pollActions()
{
    FrontendActions actions;

    SDL_Event event;

    while (
        SDL_PollEvent(
            &event
        )
    )
    {
        switch (
            event.type
        )
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                actions.quit =
                    true;

                break;

            case SDL_EVENT_WINDOW_EXPOSED:
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                actions.redraw =
                    true;

                break;

            case SDL_EVENT_KEY_DOWN:
            {
                if (
                    event.key.repeat
                )
                {
                    break;
                }

                switch (
                    event.key.scancode
                )
                {
                    case SDL_SCANCODE_ESCAPE:
                    case SDL_SCANCODE_Q:
                        actions.quit =
                            true;

                        break;

                    case SDL_SCANCODE_S:
                        actions.save =
                            true;

                        break;

                    case SDL_SCANCODE_SPACE:
                        actions.togglePause =
                            true;

                        break;

                    case SDL_SCANCODE_LEFTBRACKET:
                    case SDL_SCANCODE_COMMA:
                    case SDL_SCANCODE_PAGEDOWN:
                        actions.viewZDown =
                            true;

                        break;

                    case SDL_SCANCODE_RIGHTBRACKET:
                    case SDL_SCANCODE_PERIOD:
                    case SDL_SCANCODE_PAGEUP:
                        actions.viewZUp =
                            true;

                        break;

                    default:
                        break;
                }

                break;
            }

            default:
                break;
        }
    }

    return actions;
}

// ==================================================
// Rendering
// ==================================================

void SDLFrontend::render(
    const GameMap& map,
    entt::registry& registry,
    int viewZ,
    const std::vector<std::string>&
        hudLines
)
{
    if (
        viewZ < 0
        ||
        viewZ >=
            map.depth()
    )
    {
        return;
    }

    // ==================================================
    // Logical canvas dimensions
    //
    // SDL scales this entire fixed logical canvas into
    // the user's actual resizable window.
    // ==================================================

    const int mapPixelWidth =
        map.width()
        *
        CellSize;

    const int mapPixelHeight =
        map.height()
        *
        CellSize;

    std::size_t longestHudLine =
        0;

    for (
        const auto& line :
        hudLines
    )
    {
        longestHudLine =
            std::max(
                longestHudLine,
                line.size()
            );
    }

    const int hudPixelWidth =
        static_cast<int>(
            longestHudLine
        )
        *
        CellSize;

    const int hudPixelHeight =
        static_cast<int>(
            hudLines.size()
        )
        *
        CellSize;

    const int requiredWidth =
        std::max(
            mapPixelWidth,
            hudPixelWidth
        )
        +
        Margin
        *
        2;

    const int requiredHeight =
        Margin
        +
        mapPixelHeight
        +
        CellSize
        +
        hudPixelHeight
        +
        Margin;

    const int newLogicalWidth =
        std::max(
            640,
            requiredWidth
        );

    const int newLogicalHeight =
        std::max(
            360,
            requiredHeight
        );

    if (
        newLogicalWidth !=
            logicalWidth_
        ||
        newLogicalHeight !=
            logicalHeight_
    )
    {
        logicalWidth_ =
            newLogicalWidth;

        logicalHeight_ =
            newLogicalHeight;

        if (
            !SDL_SetRenderLogicalPresentation(
                renderer_,
                logicalWidth_,
                logicalHeight_,
                SDL_LOGICAL_PRESENTATION_LETTERBOX
            )
        )
        {
            throw std::runtime_error(
                std::string{
                    "SDL logical presentation failed: "
                }
                +
                SDL_GetError()
            );
        }
    }

    // ==================================================
    // Background
    // ==================================================

    SDL_SetRenderDrawColor(
        renderer_,
        10,
        12,
        16,
        255
    );

    SDL_RenderClear(
        renderer_
    );

    // Center map horizontally.
    const float mapOriginX =
        static_cast<float>(
            (
                logicalWidth_
                -
                mapPixelWidth
            )
            /
            2
        );

    const float mapOriginY =
        static_cast<float>(
            Margin
        );

    // Slightly darker map panel.
    SDL_FRect mapBackground{
        mapOriginX,
        mapOriginY,
        static_cast<float>(
            mapPixelWidth
        ),
        static_cast<float>(
            mapPixelHeight
        )
    };

    SDL_SetRenderDrawColor(
        renderer_,
        5,
        7,
        10,
        255
    );

    SDL_RenderFillRect(
        renderer_,
        &mapBackground
    );

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
            const GlyphVisual visual =
                visualAt(
                    map,
                    x,
                    y,
                    viewZ
                );

            drawGlyph(
                renderer_,
                mapOriginX
                    +
                    static_cast<float>(
                        x
                        *
                        CellSize
                    ),
                mapOriginY
                    +
                    static_cast<float>(
                        y
                        *
                        CellSize
                    ),
                visual.character,
                visual.color
            );
        }
    }

    // ==================================================
    // Stockpile overlay
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

    const SDL_Color stockpileColor =
        toSDLColor(
            TerminalColor::
                BrightCyan
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

                drawGlyph(
                    renderer_,
                    mapOriginX
                        +
                        static_cast<float>(
                            x
                            *
                            CellSize
                        ),
                    mapOriginY
                        +
                        static_cast<float>(
                            y
                            *
                            CellSize
                        ),
                    '=',
                    stockpileColor
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

        drawGlyph(
            renderer_,
            mapOriginX
                +
                static_cast<float>(
                    position.x
                    *
                    CellSize
                ),
            mapOriginY
                +
                static_cast<float>(
                    position.y
                    *
                    CellSize
                ),
            glyph.character,
            toSDLColor(
                glyph.color
            )
        );
    }

    // ==================================================
    // Map border
    // ==================================================

    SDL_SetRenderDrawColor(
        renderer_,
        55,
        60,
        70,
        255
    );

    SDL_RenderRect(
        renderer_,
        &mapBackground
    );

    // ==================================================
    // HUD separator
    // ==================================================

    const float separatorY =
        mapOriginY
        +
        static_cast<float>(
            mapPixelHeight
        )
        +
        4.0F;

    SDL_SetRenderDrawColor(
        renderer_,
        55,
        60,
        70,
        255
    );

    SDL_RenderLine(
        renderer_,
        static_cast<float>(
            Margin
        ),
        separatorY,
        static_cast<float>(
            logicalWidth_
            -
            Margin
        ),
        separatorY
    );

    // ==================================================
    // HUD
    // ==================================================

    const float hudOriginX =
        static_cast<float>(
            Margin
        );

    const float hudOriginY =
        separatorY
        +
        8.0F;

    const SDL_Color hudColor =
        toSDLColor(
            TerminalColor::
                BrightWhite
        );

    for (
        std::size_t i = 0;
        i < hudLines.size();
        ++i
    )
    {
        drawText(
            renderer_,
            hudOriginX,
            hudOriginY
                +
                static_cast<float>(
                    i
                    *
                    CellSize
                ),
            hudLines[i],
            hudColor
        );
    }

    // ==================================================
    // Present
    // ==================================================

    SDL_RenderPresent(
        renderer_
    );
}

}

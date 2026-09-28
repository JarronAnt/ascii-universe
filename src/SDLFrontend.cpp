#include "ascii/SDLFrontend.hpp"

#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Designations.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Material.hpp"
#include "ascii/Simulation.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"
#include "ascii/Simulation.hpp"
#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ascii
{

namespace
{

constexpr int CellSize =
    SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;

constexpr int LogicalWidth =
    800;

constexpr int LogicalHeight =
    480;

constexpr float MapOriginX =
    16.0F;

constexpr float MapOriginY =
    16.0F;

constexpr float SidebarX =
    416.0F;

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

SDL_Color toSDLColor(
    TerminalColor color
)
{
    switch (color)
    {
        case TerminalColor::Black:
            return {20, 22, 26, 255};

        case TerminalColor::Red:
            return {205, 70, 70, 255};

        case TerminalColor::Green:
            return {80, 180, 100, 255};

        case TerminalColor::Yellow:
            return {205, 170, 80, 255};

        case TerminalColor::Blue:
            return {70, 110, 210, 255};

        case TerminalColor::Magenta:
            return {175, 90, 205, 255};

        case TerminalColor::Cyan:
            return {70, 185, 195, 255};

        case TerminalColor::White:
            return {200, 205, 215, 255};

        case TerminalColor::BrightBlack:
            return {90, 95, 105, 255};

        case TerminalColor::BrightRed:
            return {255, 90, 90, 255};

        case TerminalColor::BrightGreen:
            return {105, 235, 125, 255};

        case TerminalColor::BrightYellow:
            return {245, 210, 90, 255};

        case TerminalColor::BrightBlue:
            return {90, 145, 255, 255};

        case TerminalColor::BrightMagenta:
            return {220, 115, 255, 255};

        case TerminalColor::BrightCyan:
            return {95, 230, 240, 255};

        case TerminalColor::BrightWhite:
            return {245, 245, 250, 255};

        case TerminalColor::Default:
        default:
            return {200, 205, 215, 255};
    }
}

SDL_Color dimColor(
    SDL_Color color
)
{
    constexpr float Factor =
        0.30F;

    color.r =
        static_cast<std::uint8_t>(
            static_cast<float>(color.r)
            *
            Factor
        );

    color.g =
        static_cast<std::uint8_t>(
            static_cast<float>(color.g)
            *
            Factor
        );

    color.b =
        static_cast<std::uint8_t>(
            static_cast<float>(color.b)
            *
            Factor
        );

    return color;
}

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
    if (character == ' ')
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
                TerminalColor::BrightBlue
            )
            :
            toSDLColor(
                TerminalColor::Cyan
            );

        return visual;
    }

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

    switch (tile.shape)
    {
        case TileShape::Open:
            visual.character = ' ';
            break;

        case TileShape::Floor:
            visual.character = '.';
            break;

        case TileShape::Wall:
            visual.character = '#';
            break;

        case TileShape::Ramp:
            visual.character = '^';
            break;

        case TileShape::UpStair:
            visual.character = '<';
            break;

        case TileShape::DownStair:
            visual.character = '>';
            break;

        case TileShape::UpDownStair:
            visual.character = 'X';
            break;
    }

    return visual;
}

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

    if (z <= 0)
    {
        return {};
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
        return {};
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

std::string tileShapeName(
    TileShape shape
)
{
    switch (shape)
    {
        case TileShape::Open:
            return "Open";

        case TileShape::Floor:
            return "Floor";

        case TileShape::Wall:
            return "Wall";

        case TileShape::Ramp:
            return "Ramp";

        case TileShape::UpStair:
            return "Up Stair";

        case TileShape::DownStair:
            return "Down Stair";

        case TileShape::UpDownStair:
            return "Up/Down Stair";
    }

    return "Unknown";
}

std::string itemTypeName(
    ItemType type
)
{
    switch (type)
    {
        case ItemType::Stone:
            return "Stone";

        case ItemType::Ore:
            return "Ore";

        case ItemType::Soil:
            return "Soil";

        case ItemType::Log:
            return "Log";
    }

    return "Unknown";
}

std::string designationTypeName(
    DesignationType type
)
{
    switch (type)
    {
        case DesignationType::Mine:
            return "Mine";

        case DesignationType::DigDown:
            return "Dig Down";

        case DesignationType::DigUp:
            return "Dig Up";

        case DesignationType::FellTree:
            return "Fell Tree";
    }

    return "Unknown";
}

std::string designationStateName(
    DesignationState state
)
{
    switch (state)
    {
        case DesignationState::Active:
            return "Active";

        case DesignationState::Ignored:
            return "Ignored";

        case DesignationState::Consumed:
            return "Job Created";

        case DesignationState::Cancelled:
            return "Cancelled";
    }

    return "Unknown";
}

std::string jobTypeName(
    JobType type
)
{
    switch (type)
    {
        case JobType::Mine:
            return "Mine";

        case JobType::DigDown:
            return "Dig Down";

        case JobType::DigUp:
            return "Dig Up";

        case JobType::FellTree:
            return "Fell Tree";

        case JobType::Haul:
            return "Haul";
    }

    return "Unknown";
}

std::string professionName(
    const entt::registry& registry,
    entt::entity entity
)
{
    if (
        registry.all_of<
            Miner
        >(entity)
    )
    {
        return "Miner";
    }

    if (
        registry.all_of<
            Hauler
        >(entity)
    )
    {
        return "Hauler";
    }

    if (
        registry.all_of<
            Woodcutter
        >(entity)
    )
    {
        return "Woodcutter";
    }

    return "Goblin";
}

SDL_Color modeColor(
    PlayerMode mode
)
{
    switch (mode)
    {
        case PlayerMode::Mine:
            return {
                255,
                70,
                70,
                80
            };

        case PlayerMode::FellTree:
            return {
                240,
                210,
                60,
                80
            };

        case PlayerMode::DigDown:
        case PlayerMode::DigUp:
            return {
                210,
                90,
                255,
                80
            };

        case PlayerMode::Stockpile:
            return {
                70,
                220,
                235,
                80
            };

        case PlayerMode::Cancel:
            return {
                255,
                60,
                60,
                100
            };

        case PlayerMode::Inspect:
        default:
            return {
                220,
                220,
                230,
                60
            };
    }
}

std::string stockpileAcceptsText(
    const Stockpile& stockpile
)
{
    std::string result;

    for (
        std::size_t i = 0;
        i <
            stockpile.accepts.size();
        ++i
    )
    {
        if (i > 0)
        {
            result += ", ";
        }

        result +=
            itemTypeName(
                stockpile.accepts[i]
            );
    }

    if (result.empty())
    {
        return "Nothing";
    }

    return result;
}

}

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

    SDL_SetRenderVSync(
        renderer_,
        1
    );

    SDL_SetDefaultTextureScaleMode(
        renderer_,
        SDL_SCALEMODE_NEAREST
    );

    SDL_SetRenderDrawBlendMode(
        renderer_,
        SDL_BLENDMODE_BLEND
    );

    if (
        !SDL_SetRenderLogicalPresentation(
            renderer_,
            LogicalWidth,
            LogicalHeight,
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

SDLFrontend::~SDLFrontend()
{
    if (renderer_ != nullptr)
    {
        SDL_DestroyRenderer(
            renderer_
        );

        renderer_ =
            nullptr;
    }

    if (window_ != nullptr)
    {
        SDL_DestroyWindow(
            window_
        );

        window_ =
            nullptr;
    }

    SDL_Quit();
}

std::optional<Position>
SDLFrontend::windowPointToTile(
    float windowX,
    float windowY,
    const PlayerViewState& view,
    const GameMap& map
) const
{
    float renderX = 0.0F;
    float renderY = 0.0F;

    if (
        !SDL_RenderCoordinatesFromWindow(
            renderer_,
            windowX,
            windowY,
            &renderX,
            &renderY
        )
    )
    {
        return std::nullopt;
    }

    const float mapWidth =
        static_cast<float>(
            FortressViewportColumns
            *
            CellSize
        );

    const float mapHeight =
        static_cast<float>(
            FortressViewportRows
            *
            CellSize
        );

    if (
        renderX <
            MapOriginX
        ||
        renderY <
            MapOriginY
        ||
        renderX >=
            MapOriginX
            +
            mapWidth
        ||
        renderY >=
            MapOriginY
            +
            mapHeight
    )
    {
        return std::nullopt;
    }

    const int screenX =
        static_cast<int>(
            (
                renderX
                -
                MapOriginX
            )
            /
            static_cast<float>(
                CellSize
            )
        );

    const int screenY =
        static_cast<int>(
            (
                renderY
                -
                MapOriginY
            )
            /
            static_cast<float>(
                CellSize
            )
        );

    const Position position{
        view.cameraX
        +
        screenX,
        view.cameraY
        +
        screenY,
        view.viewZ
    };

    if (
        !map.inBounds(
            position
        )
    )
    {
        return std::nullopt;
    }

    return position;
}

PlayerInput
SDLFrontend::pollInput(
    const PlayerViewState& view,
    const GameMap& map
)
{
    PlayerInput input;

    SDL_Event event;

    while (
        SDL_PollEvent(
            &event
        )
    )
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                input.quit =
                    true;

                break;

            case SDL_EVENT_MOUSE_MOTION:
            {
                const auto tile =
                    windowPointToTile(
                        event.motion.x,
                        event.motion.y,
                        view,
                        map
                    );

                if (tile)
                {
                    input.pointerEvents.
                        push_back(
                            PointerInput{
                                PointerInputType::Move,
                                *tile
                            }
                        );
                }

                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                const auto tile =
                    windowPointToTile(
                        event.button.x,
                        event.button.y,
                        view,
                        map
                    );

                if (!tile)
                {
                    break;
                }

                if (
                    event.button.button ==
                    SDL_BUTTON_LEFT
                )
                {
                    input.pointerEvents.
                        push_back(
                            PointerInput{
                                PointerInputType::
                                    PrimaryDown,
                                *tile
                            }
                        );
                }
                else if (
                    event.button.button ==
                    SDL_BUTTON_RIGHT
                )
                {
                    input.pointerEvents.
                        push_back(
                            PointerInput{
                                PointerInputType::
                                    SecondaryDown,
                                *tile
                            }
                        );
                }

                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                if (
                    event.button.button !=
                    SDL_BUTTON_LEFT
                )
                {
                    break;
                }

                const auto tile =
                    windowPointToTile(
                        event.button.x,
                        event.button.y,
                        view,
                        map
                    );

                if (tile)
                {
                    input.pointerEvents.
                        push_back(
                            PointerInput{
                                PointerInputType::
                                    PrimaryUp,
                                *tile
                            }
                        );
                }

                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                if (
                    event.wheel.y > 0.0F
                )
                {
                    ++input.zDelta;
                }
                else if (
                    event.wheel.y < 0.0F
                )
                {
                    --input.zDelta;
                }

                break;
            }

            case SDL_EVENT_KEY_DOWN:
            {
                const bool repeat =
                    event.key.repeat;

                switch (
                    event.key.scancode
                )
                {
                    // Camera.
                    case SDL_SCANCODE_W:
                        --input.cameraDy;
                        break;

                    case SDL_SCANCODE_S:
                        ++input.cameraDy;
                        break;

                    case SDL_SCANCODE_A:
                        --input.cameraDx;
                        break;

                    case SDL_SCANCODE_D:
                        ++input.cameraDx;
                        break;

                    // Cursor.
                    case SDL_SCANCODE_UP:
                        --input.cursorDy;
                        break;

                    case SDL_SCANCODE_DOWN:
                        ++input.cursorDy;
                        break;

                    case SDL_SCANCODE_LEFT:
                        --input.cursorDx;
                        break;

                    case SDL_SCANCODE_RIGHT:
                        ++input.cursorDx;
                        break;

                    // Z-levels.
                    case SDL_SCANCODE_LEFTBRACKET:
                    case SDL_SCANCODE_COMMA:
                    case SDL_SCANCODE_PAGEDOWN:
                        --input.zDelta;
                        break;

                    case SDL_SCANCODE_RIGHTBRACKET:
                    case SDL_SCANCODE_PERIOD:
                    case SDL_SCANCODE_PAGEUP:
                        ++input.zDelta;
                        break;

                    default:
                        break;
                }

                if (repeat)
                {
                    break;
                }

                switch (
                    event.key.scancode
                )
                {
                    case SDL_SCANCODE_Q:
                        input.quit =
                            true;

                        break;

                    case SDL_SCANCODE_ESCAPE:
                        if (
                            view.mode ==
                            PlayerMode::Inspect
                        )
                        {
                            input.quit =
                                true;
                        }
                        else
                        {
                            input.abortCommand =
                                true;
                        }

                        break;

                    case SDL_SCANCODE_F5:
                        input.save =
                            true;

                        break;

                    case SDL_SCANCODE_SPACE:
                        input.togglePause =
                            true;

                        break;

                    case SDL_SCANCODE_1:
                        input.requestedSpeed =
                            1;

                        break;

                    case SDL_SCANCODE_2:
                        input.requestedSpeed =
                            2;

                        break;

                    case SDL_SCANCODE_3:
                        input.requestedSpeed =
                            4;

                        break;

                    case SDL_SCANCODE_4:
                        input.requestedSpeed =
                            8;

                        break;

                    case SDL_SCANCODE_I:
                        input.requestedMode =
                            PlayerMode::Inspect;

                        break;

                    case SDL_SCANCODE_M:
                        input.requestedMode =
                            PlayerMode::Mine;

                        break;

                    case SDL_SCANCODE_T:
                        input.requestedMode =
                            PlayerMode::FellTree;

                        break;

                    case SDL_SCANCODE_V:
                        input.requestedMode =
                            PlayerMode::DigDown;

                        break;

                    case SDL_SCANCODE_U:
                        input.requestedMode =
                            PlayerMode::DigUp;

                        break;

                    case SDL_SCANCODE_P:
                        input.requestedMode =
                            PlayerMode::Stockpile;

                        break;

                    case SDL_SCANCODE_X:
                        input.requestedMode =
                            PlayerMode::Cancel;

                        break;

                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_KP_ENTER:
                        input.confirmCursor =
                            true;

                        break;

                    case SDL_SCANCODE_R:
                        input.cycleStockpilePreset =
                            true;

                        break;

                    case SDL_SCANCODE_C:
                        input.configureStockpile =
                            true;

                        break;

                    case SDL_SCANCODE_TAB:
                        input.cycleGoblin =
                            true;

                        break;

                    case SDL_SCANCODE_G:
                        input.selectGoblinAtCursor =
                            true;

                        break;

                    case SDL_SCANCODE_F:
                        input.toggleFollow =
                            true;

                        break;

                    case SDL_SCANCODE_HOME:
                        input.centerCamera =
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

    return input;
}

void SDLFrontend::render(
    const Simulation& simulation,
    const PlayerViewState& view
)
{
    const GameMap& map =
        simulation.map();

    const auto& registry =
        simulation.registry();

    SDL_SetRenderDrawColor(
        renderer_,
        9,
        11,
        15,
        255
    );

    SDL_RenderClear(
        renderer_
    );

    // ==================================================
    // Map panel
    // ==================================================

    SDL_FRect mapPanel{
        MapOriginX,
        MapOriginY,
        static_cast<float>(
            FortressViewportColumns
            *
            CellSize
        ),
        static_cast<float>(
            FortressViewportRows
            *
            CellSize
        )
    };

    SDL_SetRenderDrawColor(
        renderer_,
        4,
        6,
        9,
        255
    );

    SDL_RenderFillRect(
        renderer_,
        &mapPanel
    );

    // ==================================================
    // Terrain
    // ==================================================

    for (
        int screenY = 0;
        screenY <
            FortressViewportRows;
        ++screenY
    )
    {
        for (
            int screenX = 0;
            screenX <
                FortressViewportColumns;
            ++screenX
        )
        {
            const int worldX =
                view.cameraX
                +
                screenX;

            const int worldY =
                view.cameraY
                +
                screenY;

            const Position position{
                worldX,
                worldY,
                view.viewZ
            };

            if (
                !map.inBounds(
                    position
                )
            )
            {
                continue;
            }

            const GlyphVisual visual =
                visualAt(
                    map,
                    worldX,
                    worldY,
                    view.viewZ
                );

            drawGlyph(
                renderer_,
                MapOriginX
                +
                static_cast<float>(
                    screenX
                    *
                    CellSize
                ),
                MapOriginY
                +
                static_cast<float>(
                    screenY
                    *
                    CellSize
                ),
                visual.character,
                visual.color
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

    const SDL_Color stockpileColor =
        toSDLColor(
            TerminalColor::BrightCyan
        );

    for (
        auto entity :
        stockpileView
    )
    {
        const auto& stockpile =
            stockpileView.get<
                Stockpile
            >(entity);

        if (
            view.viewZ <
                stockpile.bounds.min.z
            ||
            view.viewZ >
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
                const int screenX =
                    x
                    -
                    view.cameraX;

                const int screenY =
                    y
                    -
                    view.cameraY;

                if (
                    screenX < 0
                    ||
                    screenY < 0
                    ||
                    screenX >=
                        FortressViewportColumns
                    ||
                    screenY >=
                        FortressViewportRows
                )
                {
                    continue;
                }

                drawGlyph(
                    renderer_,
                    MapOriginX
                    +
                    static_cast<float>(
                        screenX
                        *
                        CellSize
                    ),
                    MapOriginY
                    +
                    static_cast<float>(
                        screenY
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
    // Selection rectangle
    // ==================================================

    if (view.dragging)
    {
        const int minX =
            std::min(
                view.dragStart.x,
                view.dragCurrent.x
            );

        const int maxX =
            std::max(
                view.dragStart.x,
                view.dragCurrent.x
            );

        const int minY =
            std::min(
                view.dragStart.y,
                view.dragCurrent.y
            );

        const int maxY =
            std::max(
                view.dragStart.y,
                view.dragCurrent.y
            );

        const int clippedMinX =
            std::max(
                minX,
                view.cameraX
            );

        const int clippedMaxX =
            std::min(
                maxX,
                view.cameraX
                +
                FortressViewportColumns
                -
                1
            );

        const int clippedMinY =
            std::max(
                minY,
                view.cameraY
            );

        const int clippedMaxY =
            std::min(
                maxY,
                view.cameraY
                +
                FortressViewportRows
                -
                1
            );

        if (
            clippedMinX <=
                clippedMaxX
            &&
            clippedMinY <=
                clippedMaxY
        )
        {
            const SDL_Color highlight =
                modeColor(
                    view.mode
                );

            setDrawColor(
                renderer_,
                highlight
            );

            SDL_FRect rectangle{
                MapOriginX
                +
                static_cast<float>(
                    (
                        clippedMinX
                        -
                        view.cameraX
                    )
                    *
                    CellSize
                ),
                MapOriginY
                +
                static_cast<float>(
                    (
                        clippedMinY
                        -
                        view.cameraY
                    )
                    *
                    CellSize
                ),
                static_cast<float>(
                    (
                        clippedMaxX
                        -
                        clippedMinX
                        +
                        1
                    )
                    *
                    CellSize
                ),
                static_cast<float>(
                    (
                        clippedMaxY
                        -
                        clippedMinY
                        +
                        1
                    )
                    *
                    CellSize
                )
            };

            SDL_RenderFillRect(
                renderer_,
                &rectangle
            );
        }
    }

    // ==================================================
    // Entities
    // ==================================================

    auto entityView =
        registry.view<
            Position,
            Glyph
        >();

    std::vector<entt::entity>
        entities;

    for (auto entity : entityView)
    {
        entities.push_back(
            entity
        );
    }

    std::sort(
        entities.begin(),
        entities.end(),
        [](
            entt::entity first,
            entt::entity second
        )
        {
            return
                entt::to_integral(first)
                <
                entt::to_integral(second);
        }
    );

    for (auto entity : entities)
    {
        const Position position =
            registry.get<
                Position
            >(entity);

        if (
            position.z !=
            view.viewZ
        )
        {
            continue;
        }

        const int screenX =
            position.x
            -
            view.cameraX;

        const int screenY =
            position.y
            -
            view.cameraY;

        if (
            screenX < 0
            ||
            screenY < 0
            ||
            screenX >=
                FortressViewportColumns
            ||
            screenY >=
                FortressViewportRows
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
            MapOriginX
            +
            static_cast<float>(
                screenX
                *
                CellSize
            ),
            MapOriginY
            +
            static_cast<float>(
                screenY
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
    // Selected goblin outline
    // ==================================================

    if (
        view.selectedGoblin !=
            entt::null
        &&
        registry.valid(
            view.selectedGoblin
        )
        &&
        registry.all_of<
            Position
        >(
            view.selectedGoblin
        )
    )
    {
        const Position selected =
            registry.get<
                Position
            >(
                view.selectedGoblin
            );

        if (
            selected.z ==
            view.viewZ
        )
        {
            const int screenX =
                selected.x
                -
                view.cameraX;

            const int screenY =
                selected.y
                -
                view.cameraY;

            if (
                screenX >= 0
                &&
                screenY >= 0
                &&
                screenX <
                    FortressViewportColumns
                &&
                screenY <
                    FortressViewportRows
            )
            {
                SDL_SetRenderDrawColor(
                    renderer_,
                    255,
                    0,
                    255,
                    255
                );

                SDL_FRect selectedRect{
                    MapOriginX
                    +
                    static_cast<float>(
                        screenX
                        *
                        CellSize
                    ),
                    MapOriginY
                    +
                    static_cast<float>(
                        screenY
                        *
                        CellSize
                    ),
                    static_cast<float>(
                        CellSize
                    ),
                    static_cast<float>(
                        CellSize
                    )
                };

                SDL_RenderRect(
                    renderer_,
                    &selectedRect
                );
            }
        }
    }

    // ==================================================
    // Cursor
    // ==================================================

    if (
        view.cursor.z ==
        view.viewZ
    )
    {
        const int cursorScreenX =
            view.cursor.x
            -
            view.cameraX;

        const int cursorScreenY =
            view.cursor.y
            -
            view.cameraY;

        if (
            cursorScreenX >= 0
            &&
            cursorScreenY >= 0
            &&
            cursorScreenX <
                FortressViewportColumns
            &&
            cursorScreenY <
                FortressViewportRows
        )
        {
            const SDL_Color cursorColor =
                modeColor(
                    view.mode
                );

            setDrawColor(
                renderer_,
                SDL_Color{
                    cursorColor.r,
                    cursorColor.g,
                    cursorColor.b,
                    255
                }
            );

            SDL_FRect cursorRect{
                MapOriginX
                +
                static_cast<float>(
                    cursorScreenX
                    *
                    CellSize
                ),
                MapOriginY
                +
                static_cast<float>(
                    cursorScreenY
                    *
                    CellSize
                ),
                static_cast<float>(
                    CellSize
                ),
                static_cast<float>(
                    CellSize
                )
            };

            SDL_RenderRect(
                renderer_,
                &cursorRect
            );
        }
    }

    // Map border.
    SDL_SetRenderDrawColor(
        renderer_,
        70,
        75,
        85,
        255
    );

    SDL_RenderRect(
        renderer_,
        &mapPanel
    );

    // ==================================================
    // Sidebar
    // ==================================================

    std::vector<std::string>
        lines;

    lines.push_back(
        "ASCII UNIVERSE"
    );

    lines.push_back(
        "--------------"
    );

    lines.push_back(
        "Tick: "
        +
        std::to_string(
            simulation.time().tick
        )
    );

    lines.push_back(
        "Z: "
        +
        std::to_string(
            view.viewZ
        )
        +
        "/"
        +
        std::to_string(
            map.depth() - 1
        )
    );

    lines.push_back(
    "Landform: "
    +
    std::string(
        landformName(
            simulation.landform()
        )
    )
);

    lines.push_back(
        "Climate: "
        +
        std::string(
            climateName(
                simulation.climate()
            )
        )
    );

    lines.push_back(
        std::string{
            "State: "
        }
        +
        (
            view.paused
            ?
            "PAUSED"
            :
            "RUNNING"
        )
    );

    lines.push_back(
        "Speed: "
        +
        std::to_string(
            view.speedMultiplier
        )
        +
        "x"
    );

    lines.push_back(
        "Mode: "
        +
        std::string(
            playerModeName(
                view.mode
            )
        )
    );

    if (
        view.mode ==
        PlayerMode::Stockpile
    )
    {
        lines.push_back(
            "Pile: "
            +
            std::string(
                stockpilePresetName(
                    view.stockpilePreset
                )
            )
        );
    }

    lines.push_back("");

    lines.push_back(
        "CURSOR"
    );

    lines.push_back(
        "("
        +
        std::to_string(
            view.cursor.x
        )
        +
        ","
        +
        std::to_string(
            view.cursor.y
        )
        +
        ","
        +
        std::to_string(
            view.cursor.z
        )
        +
        ")"
    );

    if (
        map.inBounds(
            view.cursor
        )
    )
    {
        const Tile& tile =
            map.at(
                view.cursor
            );

        lines.push_back(
            "Shape: "
            +
            tileShapeName(
                tile.shape
            )
        );

        lines.push_back(
            "Material: "
            +
            std::string(
                materialName(
                    tile.material
                )
            )
        );

        if (
            tile.feature ==
            TileFeature::Tree
        )
        {
            lines.push_back(
                "Tree: "
                +
                std::string(
                    materialName(
                        tile.featureMaterial
                    )
                )
            );
        }

        if (
            tile.liquid.depth > 0
        )
        {
            lines.push_back(
                "Water: "
                +
                std::to_string(
                    tile.liquid.depth
                )
                +
                "/7"
            );
        }

        auto designationView =
            registry.view<
                Designation,
                DesignationLifecycle,
                Position
            >();

        for (
            auto entity :
            designationView
        )
        {
            if (
                designationView.get<
                    Position
                >(entity)
                !=
                view.cursor
            )
            {
                continue;
            }

            const auto& designation =
                designationView.get<
                    Designation
                >(entity);

            const auto& lifecycle =
                designationView.get<
                    DesignationLifecycle
                >(entity);

            if (
                lifecycle.state ==
                DesignationState::Ignored
                ||
                lifecycle.state ==
                DesignationState::Cancelled
            )
            {
                continue;
            }

            lines.push_back(
                "Order: "
                +
                designationTypeName(
                    designation.type
                )
            );

            lines.push_back(
                "Order state: "
                +
                designationStateName(
                    lifecycle.state
                )
            );

            break;
        }

        auto itemView =
            registry.view<
                Item,
                Position
            >();

        for (auto entity : itemView)
        {
            if (
                itemView.get<
                    Position
                >(entity)
                !=
                view.cursor
            )
            {
                continue;
            }

            const auto& item =
                itemView.get<
                    Item
                >(entity);

            lines.push_back(
                "Item: "
                +
                itemTypeName(
                    item.type
                )
            );

            lines.push_back(
                "Item mat: "
                +
                std::string(
                    materialName(
                        item.material
                    )
                )
            );

            break;
        }

        for (
            auto entity :
            stockpileView
        )
        {
            const auto& stockpile =
                stockpileView.get<
                    Stockpile
                >(entity);

            if (
                !stockpile.bounds.contains(
                    view.cursor
                )
            )
            {
                continue;
            }

            lines.push_back(
                "Stockpile: "
                +
                stockpileAcceptsText(
                    stockpile
                )
            );

            break;
        }
    }

    lines.push_back("");

    lines.push_back(
        "SELECTED GOBLIN"
    );

    if (
        view.selectedGoblin ==
            entt::null
        ||
        !registry.valid(
            view.selectedGoblin
        )
        ||
        !registry.all_of<
            Goblin,
            Position
        >(
            view.selectedGoblin
        )
    )
    {
        lines.push_back(
            "None"
        );
    }
    else
    {
        std::string name =
            "Goblin";

        if (
            registry.all_of<
                Name
            >(
                view.selectedGoblin
            )
        )
        {
            name =
                registry.get<
                    Name
                >(
                    view.selectedGoblin
                ).value;
        }

        const Position position =
            registry.get<
                Position
            >(
                view.selectedGoblin
            );

        lines.push_back(
            name
        );

        lines.push_back(
            professionName(
                registry,
                view.selectedGoblin
            )
        );

        lines.push_back(
            "Pos: "
            +
            std::to_string(
                position.x
            )
            +
            ","
            +
            std::to_string(
                position.y
            )
            +
            ","
            +
            std::to_string(
                position.z
            )
        );

        lines.push_back(
            std::string{
                "Follow: "
            }
            +
            (
                view.followSelected
                ?
                "YES"
                :
                "NO"
            )
        );

        if (
            registry.all_of<
                AssignedJob
            >(
                view.selectedGoblin
            )
        )
        {
            const JobId jobId =
                registry.get<
                    AssignedJob
                >(
                    view.selectedGoblin
                ).id;

            const Job* job =
                simulation.jobBoard().
                    find(
                        jobId
                    );

            if (job != nullptr)
            {
                lines.push_back(
                    "Job: "
                    +
                    jobTypeName(
                        job->type
                    )
                );
            }
        }
        else
        {
            lines.push_back(
                "Job: Idle"
            );
        }
    }

    lines.push_back("");

    lines.push_back(
        "CONTROLS"
    );

    lines.push_back(
        "WASD       pan camera"
    );

    lines.push_back(
        "Arrows     move cursor"
    );

    lines.push_back(
        "[ ]        change Z"
    );

    lines.push_back(
        "Mouse      inspect/select"
    );

    lines.push_back(
        "LMB drag   paint area"
    );

    lines.push_back(
        "RMB/Esc    cancel mode"
    );

    lines.push_back(
        "M          mine"
    );

    lines.push_back(
        "T          fell trees"
    );

    lines.push_back(
        "V          dig down"
    );

    lines.push_back(
        "U          dig up"
    );

    lines.push_back(
        "P          stockpile"
    );

    lines.push_back(
        "X          cancel order"
    );

    lines.push_back(
        "I          inspect"
    );

    lines.push_back(
        "R          pile filter"
    );

    lines.push_back(
        "C          configure pile"
    );

    lines.push_back(
        "Enter      apply cursor"
    );

    lines.push_back(
        "Tab        next goblin"
    );

    lines.push_back(
        "G          select goblin"
    );

    lines.push_back(
        "F          follow goblin"
    );

    lines.push_back(
        "1/2/3/4    1x/2x/4x/8x"
    );

    lines.push_back(
        "Space      pause"
    );

    lines.push_back(
        "F5         save"
    );

    lines.push_back("");

    lines.push_back(
        "STATUS"
    );

    lines.push_back(
        view.statusMessage
    );

    const SDL_Color normalText =
        toSDLColor(
            TerminalColor::BrightWhite
        );

    const SDL_Color headingText =
        toSDLColor(
            TerminalColor::BrightCyan
        );

    float textY =
        16.0F;

    for (
        const std::string& line :
        lines
    )
    {
        const bool heading =
            line ==
                "ASCII UNIVERSE"
            ||
            line ==
                "CURSOR"
            ||
            line ==
                "SELECTED GOBLIN"
            ||
            line ==
                "CONTROLS"
            ||
            line ==
                "STATUS";

        drawText(
            renderer_,
            SidebarX,
            textY,
            line,
            heading
            ?
            headingText
            :
            normalText
        );

        textY +=
            static_cast<float>(
                CellSize
            );

        if (
            textY >
            LogicalHeight
            -
            CellSize
        )
        {
            break;
        }
    }

    SDL_RenderPresent(
        renderer_
    );
}

}

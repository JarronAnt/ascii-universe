#include "ascii/PlayerController.hpp"

#include "ascii/Components.hpp"
#include "ascii/GameMap.hpp"
#include "ascii/Simulation.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <utility>
#include <vector>

namespace ascii
{

namespace
{

Position normalizedMin(
    Position a,
    Position b
)
{
    return Position{
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        a.z
    };
}

Position normalizedMax(
    Position a,
    Position b
)
{
    return Position{
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        a.z
    };
}

}

PlayerController::PlayerController(
    const GameMap& map,
    Position initialFocus
)
{
    state_.viewZ =
        std::clamp(
            initialFocus.z,
            0,
            map.depth() - 1
        );

    state_.cursor =
        initialFocus;

    state_.cursor.z =
        state_.viewZ;

    clampCursor(map);

    centerCameraOn(
        map,
        state_.cursor
    );
}

const PlayerViewState&
PlayerController::state() const
{
    return state_;
}

PlayerViewState&
PlayerController::state()
{
    return state_;
}

void PlayerController::setStatus(
    std::string message
)
{
    state_.statusMessage =
        std::move(message);
}

std::chrono::nanoseconds
PlayerController::scaleElapsed(
    std::chrono::nanoseconds elapsed
) const
{
    if (state_.paused)
    {
        return std::chrono::nanoseconds{0};
    }

    return std::chrono::nanoseconds{
        elapsed.count()
        *
        state_.speedMultiplier
    };
}

void PlayerController::setMode(
    PlayerMode mode
)
{
    state_.mode =
        mode;

    state_.dragging =
        false;

    state_.statusMessage =
        "Mode: "
        +
        std::string(
            playerModeName(mode)
        );
}

void PlayerController::clampCamera(
    const GameMap& map
)
{
    const int maxX =
        std::max(
            0,
            map.width()
            -
            FortressViewportColumns
        );

    const int maxY =
        std::max(
            0,
            map.height()
            -
            FortressViewportRows
        );

    state_.cameraX =
        std::clamp(
            state_.cameraX,
            0,
            maxX
        );

    state_.cameraY =
        std::clamp(
            state_.cameraY,
            0,
            maxY
        );
}

void PlayerController::clampCursor(
    const GameMap& map
)
{
    state_.cursor.x =
        std::clamp(
            state_.cursor.x,
            0,
            map.width() - 1
        );

    state_.cursor.y =
        std::clamp(
            state_.cursor.y,
            0,
            map.height() - 1
        );

    state_.cursor.z =
        std::clamp(
            state_.viewZ,
            0,
            map.depth() - 1
        );
}

void PlayerController::centerCameraOn(
    const GameMap& map,
    Position position
)
{
    state_.cameraX =
        position.x
        -
        FortressViewportColumns / 2;

    state_.cameraY =
        position.y
        -
        FortressViewportRows / 2;

    clampCamera(map);
}

void PlayerController::keepCursorVisible(
    const GameMap& map
)
{
    if (
        state_.cursor.x <
        state_.cameraX
    )
    {
        state_.cameraX =
            state_.cursor.x;
    }

    if (
        state_.cursor.x >=
        state_.cameraX
        +
        FortressViewportColumns
    )
    {
        state_.cameraX =
            state_.cursor.x
            -
            FortressViewportColumns
            +
            1;
    }

    if (
        state_.cursor.y <
        state_.cameraY
    )
    {
        state_.cameraY =
            state_.cursor.y;
    }

    if (
        state_.cursor.y >=
        state_.cameraY
        +
        FortressViewportRows
    )
    {
        state_.cameraY =
            state_.cursor.y
            -
            FortressViewportRows
            +
            1;
    }

    clampCamera(map);
}

void PlayerController::clampCursorToViewport(
    const GameMap& map
)
{
    const int right =
        std::min(
            map.width() - 1,
            state_.cameraX
            +
            FortressViewportColumns
            -
            1
        );

    const int bottom =
        std::min(
            map.height() - 1,
            state_.cameraY
            +
            FortressViewportRows
            -
            1
        );

    state_.cursor.x =
        std::clamp(
            state_.cursor.x,
            state_.cameraX,
            right
        );

    state_.cursor.y =
        std::clamp(
            state_.cursor.y,
            state_.cameraY,
            bottom
        );
}

void PlayerController::moveCamera(
    const GameMap& map,
    int dx,
    int dy
)
{
    if (
        dx == 0
        &&
        dy == 0
    )
    {
        return;
    }

    state_.followSelected =
        false;

    state_.cameraX += dx;
    state_.cameraY += dy;

    clampCamera(map);

    clampCursorToViewport(map);

    state_.dragging =
        false;
}

void PlayerController::moveCursor(
    const GameMap& map,
    int dx,
    int dy
)
{
    if (
        dx == 0
        &&
        dy == 0
    )
    {
        return;
    }

    state_.followSelected =
        false;

    state_.cursor.x += dx;
    state_.cursor.y += dy;

    clampCursor(map);

    keepCursorVisible(map);

    if (state_.dragging)
    {
        state_.dragCurrent =
            state_.cursor;
    }
}

void PlayerController::changeZ(
    const GameMap& map,
    int delta
)
{
    if (delta == 0)
    {
        return;
    }

    state_.followSelected =
        false;

    state_.viewZ =
        std::clamp(
            state_.viewZ + delta,
            0,
            map.depth() - 1
        );

    state_.cursor.z =
        state_.viewZ;

    state_.dragging =
        false;

    state_.statusMessage =
        "Viewing Z "
        +
        std::to_string(
            state_.viewZ
        );
}

void PlayerController::beginSelection(
    Position position
)
{
    state_.followSelected =
        false;

    state_.cursor =
        position;

    state_.dragging =
        true;

    state_.dragStart =
        position;

    state_.dragCurrent =
        position;
}

void PlayerController::updateSelection(
    Position position
)
{
    if (!state_.dragging)
    {
        return;
    }

    if (
        position.z !=
        state_.dragStart.z
    )
    {
        return;
    }

    state_.dragCurrent =
        position;

    state_.cursor =
        position;
}

void PlayerController::abortSelection()
{
    state_.dragging =
        false;
}

void PlayerController::applyArea(
    Simulation& simulation,
    Position first,
    Position second
)
{
    const Position minimum =
        normalizedMin(
            first,
            second
        );

    const Position maximum =
        normalizedMax(
            first,
            second
        );

    if (
        state_.mode ==
        PlayerMode::Stockpile
    )
    {
        const auto entity =
            simulation.createStockpile(
                minimum,
                maximum,
                stockpileTypesForPreset(
                    state_.stockpilePreset
                )
            );

        if (
            entity ==
            entt::null
        )
        {
            state_.statusMessage =
                "Stockpile placement invalid.";
        }
        else
        {
            state_.statusMessage =
                "Created "
                +
                std::string(
                    stockpilePresetName(
                        state_.stockpilePreset
                    )
                )
                +
                " stockpile.";
        }

        return;
    }

    std::size_t affected =
        0;

    for (
        int y = minimum.y;
        y <= maximum.y;
        ++y
    )
    {
        for (
            int x = minimum.x;
            x <= maximum.x;
            ++x
        )
        {
            const Position position{
                x,
                y,
                minimum.z
            };

            switch (state_.mode)
            {
                case PlayerMode::Mine:
                {
                    if (
                        simulation.designateMine(
                            position
                        )
                        !=
                        entt::null
                    )
                    {
                        ++affected;
                    }

                    break;
                }

                case PlayerMode::FellTree:
                {
                    if (
                        simulation.designateFellTree(
                            position
                        )
                        !=
                        entt::null
                    )
                    {
                        ++affected;
                    }

                    break;
                }

                case PlayerMode::DigDown:
                {
                    if (
                        simulation.designateDigDown(
                            position
                        )
                        !=
                        entt::null
                    )
                    {
                        ++affected;
                    }

                    break;
                }

                case PlayerMode::DigUp:
                {
                    if (
                        simulation.designateDigUp(
                            position
                        )
                        !=
                        entt::null
                    )
                    {
                        ++affected;
                    }

                    break;
                }

                case PlayerMode::Cancel:
                {
                    if (
                        simulation.cancelDesignationAt(
                            position
                        )
                    )
                    {
                        ++affected;
                    }

                    break;
                }

                case PlayerMode::Inspect:
                case PlayerMode::Stockpile:
                    break;
            }
        }
    }

    std::ostringstream message;

    if (
        state_.mode ==
        PlayerMode::Cancel
    )
    {
        message
            << "Cancelled "
            << affected
            << " designation";

        if (affected != 1)
        {
            message << 's';
        }

        message << '.';
    }
    else
    {
        message
            << "Designated "
            << affected
            << " tile";

        if (affected != 1)
        {
            message << 's';
        }

        message
            << " for "
            << playerModeName(
                state_.mode
            )
            << '.';
    }

    state_.statusMessage =
        message.str();
}

void PlayerController::commitSelection(
    Simulation& simulation
)
{
    if (!state_.dragging)
    {
        return;
    }

    const Position first =
        state_.dragStart;

    const Position second =
        state_.dragCurrent;

    state_.dragging =
        false;

    applyArea(
        simulation,
        first,
        second
    );
}

void PlayerController::cycleStockpilePreset()
{
    int value =
        static_cast<int>(
            state_.stockpilePreset
        );

    value =
        (
            value + 1
        )
        %
        6;

    state_.stockpilePreset =
        static_cast<
            StockpilePreset
        >(value);

    state_.statusMessage =
        "Stockpile filter: "
        +
        std::string(
            stockpilePresetName(
                state_.stockpilePreset
            )
        );
}

void PlayerController::configureStockpile(
    Simulation& simulation
)
{
    if (
        simulation.configureStockpileAt(
            state_.cursor,
            stockpileTypesForPreset(
                state_.stockpilePreset
            )
        )
    )
    {
        state_.statusMessage =
            "Stockpile changed to "
            +
            std::string(
                stockpilePresetName(
                    state_.stockpilePreset
                )
            )
            +
            ".";
    }
    else
    {
        state_.statusMessage =
            "No stockpile under cursor.";
    }
}

void PlayerController::selectGoblinAt(
    Simulation& simulation,
    Position position
)
{
    auto& registry =
        simulation.registry();

    auto view =
        registry.view<
            Goblin,
            Position
        >();

    std::vector<entt::entity>
        matches;

    for (auto entity : view)
    {
        if (
            view.get<
                Position
            >(entity)
            ==
            position
        )
        {
            matches.push_back(
                entity
            );
        }
    }

    std::sort(
        matches.begin(),
        matches.end(),
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

    if (matches.empty())
    {
        state_.selectedGoblin =
            entt::null;

        state_.followSelected =
            false;

        state_.statusMessage =
            "No goblin on selected tile.";

        return;
    }

    state_.selectedGoblin =
        matches.front();

    state_.statusMessage =
        "Goblin selected.";
}

void PlayerController::cycleGoblin(
    Simulation& simulation
)
{
    auto& registry =
        simulation.registry();

    auto view =
        registry.view<
            Goblin
        >();

    std::vector<entt::entity>
        goblins;

    for (auto entity : view)
    {
        goblins.push_back(entity);
    }

    std::sort(
        goblins.begin(),
        goblins.end(),
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

    if (goblins.empty())
    {
        state_.selectedGoblin =
            entt::null;

        state_.followSelected =
            false;

        state_.statusMessage =
            "No goblins exist.";

        return;
    }

    auto iterator =
        std::find(
            goblins.begin(),
            goblins.end(),
            state_.selectedGoblin
        );

    if (
        iterator ==
        goblins.end()
        ||
        ++iterator ==
        goblins.end()
    )
    {
        state_.selectedGoblin =
            goblins.front();
    }
    else
    {
        state_.selectedGoblin =
            *iterator;
    }

    focusSelectedGoblin(
        simulation
    );

    state_.statusMessage =
        "Selected next goblin.";
}

void PlayerController::focusSelectedGoblin(
    Simulation& simulation
)
{
    auto& registry =
        simulation.registry();

    if (
        state_.selectedGoblin ==
        entt::null
        ||
        !registry.valid(
            state_.selectedGoblin
        )
        ||
        !registry.all_of<
            Position
        >(
            state_.selectedGoblin
        )
    )
    {
        return;
    }

    const Position position =
        registry.get<
            Position
        >(
            state_.selectedGoblin
        );

    state_.cursor =
        position;

    state_.viewZ =
        position.z;

    centerCameraOn(
        simulation.map(),
        position
    );
}

void PlayerController::toggleFollow(
    Simulation& simulation
)
{
    if (
        state_.selectedGoblin ==
        entt::null
    )
    {
        cycleGoblin(
            simulation
        );
    }

    if (
        state_.selectedGoblin ==
        entt::null
    )
    {
        return;
    }

    state_.followSelected =
        !state_.followSelected;

    if (
        state_.followSelected
    )
    {
        focusSelectedGoblin(
            simulation
        );

        state_.statusMessage =
            "Following selected goblin.";
    }
    else
    {
        state_.statusMessage =
            "Stopped following goblin.";
    }
}

void PlayerController::updateFollow(
    Simulation& simulation
)
{
    if (
        !state_.followSelected
    )
    {
        return;
    }

    focusSelectedGoblin(
        simulation
    );
}

void PlayerController::handleInput(
    const PlayerInput& input,
    Simulation& simulation
)
{
    const GameMap& map =
        simulation.map();

    if (
        input.requestedMode
    )
    {
        setMode(
            *input.requestedMode
        );
    }

    if (
        input.togglePause
    )
    {
        state_.paused =
            !state_.paused;

        state_.statusMessage =
            state_.paused
            ?
            "Simulation paused."
            :
            "Simulation resumed.";
    }

    if (
        input.requestedSpeed
    )
    {
        state_.speedMultiplier =
            std::clamp(
                *input.requestedSpeed,
                1,
                8
            );

        state_.statusMessage =
            "Simulation speed: "
            +
            std::to_string(
                state_.speedMultiplier
            )
            +
            "x";
    }

    if (
        input.abortCommand
    )
    {
        abortSelection();

        setMode(
            PlayerMode::Inspect
        );
    }

    moveCamera(
        map,
        input.cameraDx,
        input.cameraDy
    );

    moveCursor(
        map,
        input.cursorDx,
        input.cursorDy
    );

    changeZ(
        map,
        input.zDelta
    );

    if (
        input.centerCamera
    )
    {
        centerCameraOn(
            map,
            state_.cursor
        );

        state_.followSelected =
            false;
    }

    if (
        input.cycleStockpilePreset
    )
    {
        cycleStockpilePreset();
    }

    if (
        input.configureStockpile
    )
    {
        configureStockpile(
            simulation
        );
    }

    if (
        input.cycleGoblin
    )
    {
        cycleGoblin(
            simulation
        );
    }

    if (
        input.selectGoblinAtCursor
    )
    {
        selectGoblinAt(
            simulation,
            state_.cursor
        );
    }

    if (
        input.toggleFollow
    )
    {
        toggleFollow(
            simulation
        );
    }

    if (
        input.confirmCursor
    )
    {
        if (
            state_.mode ==
            PlayerMode::Inspect
        )
        {
            selectGoblinAt(
                simulation,
                state_.cursor
            );
        }
        else
        {
            applyArea(
                simulation,
                state_.cursor,
                state_.cursor
            );
        }
    }

    for (
        const PointerInput& pointer :
        input.pointerEvents
    )
    {
        state_.followSelected =
            false;

        state_.cursor =
            pointer.tile;

        state_.viewZ =
            pointer.tile.z;

        switch (pointer.type)
        {
            case PointerInputType::Move:
            {
                if (state_.dragging)
                {
                    updateSelection(
                        pointer.tile
                    );
                }

                break;
            }

            case PointerInputType::PrimaryDown:
            {
                if (
                    state_.mode ==
                    PlayerMode::Inspect
                )
                {
                    selectGoblinAt(
                        simulation,
                        pointer.tile
                    );
                }
                else
                {
                    beginSelection(
                        pointer.tile
                    );
                }

                break;
            }

            case PointerInputType::PrimaryUp:
            {
                if (state_.dragging)
                {
                    updateSelection(
                        pointer.tile
                    );

                    commitSelection(
                        simulation
                    );
                }

                break;
            }

            case PointerInputType::SecondaryDown:
            {
                abortSelection();

                setMode(
                    PlayerMode::Inspect
                );

                break;
            }
        }
    }

    clampCursor(map);
    clampCamera(map);
}

}

#pragma once

#include "PlayerControls.hpp"

#include <chrono>

namespace ascii
{

class GameMap;
class Simulation;

class PlayerController
{
public:
    PlayerController(
        const GameMap& map,
        Position initialFocus
    );

    void handleInput(
        const PlayerInput& input,
        Simulation& simulation
    );

    void updateFollow(
        Simulation& simulation
    );

    [[nodiscard]]
    const PlayerViewState&
    state() const;

    [[nodiscard]]
    PlayerViewState&
    state();

    void setStatus(
        std::string message
    );

    [[nodiscard]]
    std::chrono::nanoseconds
    scaleElapsed(
        std::chrono::nanoseconds elapsed
    ) const;

private:
    void setMode(
        PlayerMode mode
    );

    void moveCamera(
        const GameMap& map,
        int dx,
        int dy
    );

    void moveCursor(
        const GameMap& map,
        int dx,
        int dy
    );

    void changeZ(
        const GameMap& map,
        int delta
    );

    void centerCameraOn(
        const GameMap& map,
        Position position
    );

    void clampCamera(
        const GameMap& map
    );

    void clampCursor(
        const GameMap& map
    );

    void keepCursorVisible(
        const GameMap& map
    );

    void clampCursorToViewport(
        const GameMap& map
    );

    void beginSelection(
        Position position
    );

    void updateSelection(
        Position position
    );

    void commitSelection(
        Simulation& simulation
    );

    void abortSelection();

    void applyArea(
        Simulation& simulation,
        Position first,
        Position second
    );

    void cycleStockpilePreset();

    void configureStockpile(
        Simulation& simulation
    );

    void selectGoblinAt(
        Simulation& simulation,
        Position position
    );

    void cycleGoblin(
        Simulation& simulation
    );

    void toggleFollow(
        Simulation& simulation
    );

    void focusSelectedGoblin(
        Simulation& simulation
    );

    PlayerViewState state_;
};

}

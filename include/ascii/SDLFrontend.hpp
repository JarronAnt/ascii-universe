#pragma once

#include "GameMap.hpp"
#include "PlayerControls.hpp"
#include <optional>

struct SDL_Window;
struct SDL_Renderer;

namespace ascii
{

class Simulation;

class SDLFrontend
{
public:
    SDLFrontend(
        int windowWidth = 1440,
        int windowHeight = 900
    );

    ~SDLFrontend();

    SDLFrontend(
        const SDLFrontend&
    ) = delete;

    SDLFrontend&
    operator=(
        const SDLFrontend&
    ) = delete;

    [[nodiscard]]
    PlayerInput pollInput(
        const PlayerViewState& view,
        const GameMap& map
    );

    void render(
        const Simulation& simulation,
        const PlayerViewState& view
    );

private:
    [[nodiscard]]
    std::optional<Position>
    windowPointToTile(
        float windowX,
        float windowY,
        const PlayerViewState& view,
        const GameMap& map
    ) const;

    SDL_Window* window_{
        nullptr
    };

    SDL_Renderer* renderer_{
        nullptr
    };
};

}

#pragma once

#include "GameMap.hpp"

#include <entt/entt.hpp>

#include <string>
#include <vector>

// Avoid pulling SDL headers into everything that
// includes this file.
struct SDL_Window;
struct SDL_Renderer;

namespace ascii
{

struct FrontendActions
{
    bool quit{false};

    bool save{false};

    bool togglePause{false};

    bool viewZDown{false};

    bool viewZUp{false};

    bool redraw{false};
};

class SDLFrontend
{
public:
    SDLFrontend(
        int windowWidth = 1280,
        int windowHeight = 720
    );

    ~SDLFrontend();

    SDLFrontend(
        const SDLFrontend&
    ) = delete;

    SDLFrontend&
    operator=(
        const SDLFrontend&
    ) = delete;

    SDLFrontend(
        SDLFrontend&&
    ) = delete;

    SDLFrontend&
    operator=(
        SDLFrontend&&
    ) = delete;

    [[nodiscard]]
    FrontendActions pollActions();

    void render(
        const GameMap& map,
        entt::registry& registry,
        int viewZ,
        const std::vector<std::string>&
            hudLines
    );

private:
    SDL_Window* window_{
        nullptr
    };

    SDL_Renderer* renderer_{
        nullptr
    };

    int logicalWidth_{0};

    int logicalHeight_{0};
};

}

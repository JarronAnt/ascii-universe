#pragma once

#include "Items.hpp"
#include "Position.hpp"

#include <entt/entt.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ascii
{

inline constexpr int FortressViewportColumns = 48;
inline constexpr int FortressViewportRows = 24;

enum class PlayerMode
{
    Inspect,
    Mine,
    FellTree,
    DigDown,
    DigUp,
    Stockpile,
    Cancel
};

[[nodiscard]]
inline std::string_view playerModeName(
    PlayerMode mode
)
{
    switch (mode)
    {
        case PlayerMode::Inspect:
            return "INSPECT";

        case PlayerMode::Mine:
            return "MINE";

        case PlayerMode::FellTree:
            return "FELL TREES";

        case PlayerMode::DigDown:
            return "DIG DOWN";

        case PlayerMode::DigUp:
            return "DIG UP";

        case PlayerMode::Stockpile:
            return "STOCKPILE";

        case PlayerMode::Cancel:
            return "CANCEL";
    }

    return "UNKNOWN";
}

enum class StockpilePreset
{
    All,
    StoneAndOre,
    Stone,
    Ore,
    Soil,
    Logs
};

[[nodiscard]]
inline std::string_view stockpilePresetName(
    StockpilePreset preset
)
{
    switch (preset)
    {
        case StockpilePreset::All:
            return "All";

        case StockpilePreset::StoneAndOre:
            return "Stone + Ore";

        case StockpilePreset::Stone:
            return "Stone";

        case StockpilePreset::Ore:
            return "Ore";

        case StockpilePreset::Soil:
            return "Soil";

        case StockpilePreset::Logs:
            return "Logs";
    }

    return "Unknown";
}

[[nodiscard]]
inline std::vector<ItemType>
stockpileTypesForPreset(
    StockpilePreset preset
)
{
    switch (preset)
    {
        case StockpilePreset::All:
            return {
                ItemType::Stone,
                ItemType::Ore,
                ItemType::Soil,
                ItemType::Log
            };

        case StockpilePreset::StoneAndOre:
            return {
                ItemType::Stone,
                ItemType::Ore
            };

        case StockpilePreset::Stone:
            return {
                ItemType::Stone
            };

        case StockpilePreset::Ore:
            return {
                ItemType::Ore
            };

        case StockpilePreset::Soil:
            return {
                ItemType::Soil
            };

        case StockpilePreset::Logs:
            return {
                ItemType::Log
            };
    }

    return {};
}

enum class PointerInputType
{
    Move,
    PrimaryDown,
    PrimaryUp,
    SecondaryDown
};

struct PointerInput
{
    PointerInputType type{
        PointerInputType::Move
    };

    Position tile{};
};

struct PlayerInput
{
    bool quit{false};
    bool save{false};

    bool togglePause{false};

    int cameraDx{0};
    int cameraDy{0};

    int cursorDx{0};
    int cursorDy{0};

    int zDelta{0};

    std::optional<int>
        requestedSpeed;

    std::optional<PlayerMode>
        requestedMode;

    bool confirmCursor{false};

    bool abortCommand{false};

    bool cycleStockpilePreset{false};

    bool configureStockpile{false};

    bool cycleGoblin{false};

    bool selectGoblinAtCursor{false};

    bool toggleFollow{false};

    bool centerCamera{false};

    std::vector<PointerInput>
        pointerEvents;
};

struct PlayerViewState
{
    int cameraX{0};
    int cameraY{0};
    int viewZ{0};

    Position cursor{};

    PlayerMode mode{
        PlayerMode::Inspect
    };

    bool dragging{false};

    Position dragStart{};
    Position dragCurrent{};

    StockpilePreset stockpilePreset{
        StockpilePreset::All
    };

    bool paused{false};

    int speedMultiplier{1};

    entt::entity selectedGoblin{
        entt::null
    };

    bool followSelected{false};

    std::string statusMessage{
        "Ready."
    };
};

}

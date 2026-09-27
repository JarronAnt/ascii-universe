#pragma once

namespace ascii
{

struct Position
{
    int x{};
    int y{};
    int z{};

    friend bool operator==(
        const Position&,
        const Position&
    ) = default;
};

}

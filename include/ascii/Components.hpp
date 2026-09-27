#pragma once

#include "Color.hpp"
#include "Position.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ascii
{

struct Name
{
    std::string value;
};

struct Goblin
{
};

// ==================================================
// Rendering
// ==================================================

struct Glyph
{
    char character{'?'};

    TerminalColor color{
        TerminalColor::White
    };
};

// ==================================================
// Professions
// ==================================================

struct Miner
{
};

struct Hauler
{
};

// Future:
//
// struct Builder {};
// struct Farmer {};
// struct Smith {};
// struct Carpenter {};
// struct Cook {};
// struct Doctor {};
// struct Soldier {};

// ==================================================
// Movement
// ==================================================

struct MovementPath
{
    std::vector<Position> nodes;

    std::size_t nextStep{0};

    [[nodiscard]]
    bool finished() const
    {
        return
            nextStep >=
            nodes.size();
    }
};

}

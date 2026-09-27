#pragma once

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

struct Glyph
{
    char character{'?'};
};

// --------------------------------------------------
// Professions
// --------------------------------------------------

struct Miner
{
};

struct Hauler
{
};

// Later:
//
// struct Builder {};
// struct Farmer {};
// struct Smith {};
// struct Carpenter {};
// struct Doctor {};
// struct Soldier {};

// --------------------------------------------------
// Movement
// --------------------------------------------------

struct MovementPath
{
    std::vector<Position> nodes;

    std::size_t nextStep{0};

    [[nodiscard]]
    bool finished() const
    {
        return nextStep >= nodes.size();
    }
};

}

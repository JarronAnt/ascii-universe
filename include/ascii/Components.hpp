#pragma once

#include "Position.hpp"

#include <string>

namespace ascii
{

    struct Name
    {
        std::string value;
    };

    struct Goblin
    {};

    struct Glyph
    {
        char character{'?'};
    };

}

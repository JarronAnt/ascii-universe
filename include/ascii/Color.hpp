#pragma once

namespace ascii
{

enum class TerminalColor
{
    Default,

    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,

    BrightBlack,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightCyan,
    BrightWhite
};

[[nodiscard]]
constexpr int ansiForegroundCode(
    TerminalColor color
)
{
    switch (color)
    {
        case TerminalColor::Black:
            return 30;

        case TerminalColor::Red:
            return 31;

        case TerminalColor::Green:
            return 32;

        case TerminalColor::Yellow:
            return 33;

        case TerminalColor::Blue:
            return 34;

        case TerminalColor::Magenta:
            return 35;

        case TerminalColor::Cyan:
            return 36;

        case TerminalColor::White:
            return 37;

        case TerminalColor::BrightBlack:
            return 90;

        case TerminalColor::BrightRed:
            return 91;

        case TerminalColor::BrightGreen:
            return 92;

        case TerminalColor::BrightYellow:
            return 93;

        case TerminalColor::BrightBlue:
            return 94;

        case TerminalColor::BrightMagenta:
            return 95;

        case TerminalColor::BrightCyan:
            return 96;

        case TerminalColor::BrightWhite:
            return 97;

        case TerminalColor::Default:
        default:
            return 39;
    }
}

}

#include "ascii/TerminalInput.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace ascii
{

struct TerminalInput::State
{
    termios original{};
};

TerminalInput::TerminalInput()
{
    if (
        !isatty(
            STDIN_FILENO
        )
    )
    {
        return;
    }

    state_ =
        new State{};

    if (
        tcgetattr(
            STDIN_FILENO,
            &state_->original
        )
        != 0
    )
    {
        delete state_;

        state_ =
            nullptr;

        return;
    }

    termios raw =
        state_->original;

    raw.c_lflag &=
        static_cast<
            tcflag_t
        >(
            ~(
                ICANON |
                ECHO
            )
        );

    raw.c_cc[VMIN] =
        0;

    raw.c_cc[VTIME] =
        0;

    if (
        tcsetattr(
            STDIN_FILENO,
            TCSANOW,
            &raw
        )
        != 0
    )
    {
        delete state_;

        state_ =
            nullptr;

        return;
    }

    originalFlags_ =
        fcntl(
            STDIN_FILENO,
            F_GETFL,
            0
        );

    fcntl(
        STDIN_FILENO,
        F_SETFL,
        originalFlags_ |
            O_NONBLOCK
    );

    active_ =
        true;
}

TerminalInput::~TerminalInput()
{
    if (
        active_ &&
        state_ != nullptr
    )
    {
        tcsetattr(
            STDIN_FILENO,
            TCSANOW,
            &state_->original
        );

        fcntl(
            STDIN_FILENO,
            F_SETFL,
            originalFlags_
        );
    }

    delete state_;
}

std::optional<char>
TerminalInput::poll()
{
    if (!active_)
    {
        return std::nullopt;
    }

    char value{};

    const auto result =
        read(
            STDIN_FILENO,
            &value,
            1
        );

    if (
        result == 1
    )
    {
        return value;
    }

    return std::nullopt;
}

}

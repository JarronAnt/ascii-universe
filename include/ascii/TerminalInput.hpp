#pragma once

#include <optional>

namespace ascii
{

class TerminalInput
{
public:
    TerminalInput();

    ~TerminalInput();

    TerminalInput(
        const TerminalInput&
    ) = delete;

    TerminalInput&
    operator=(
        const TerminalInput&
    ) = delete;

    [[nodiscard]]
    std::optional<char>
    poll();

private:
    struct State;

    State* state_{nullptr};

    int originalFlags_{0};

    bool active_{false};
};

}

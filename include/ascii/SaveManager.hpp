#pragma once

#include "Simulation.hpp"

#include <filesystem>
#include <memory>

namespace ascii
{

class SaveManager
{
public:
    static constexpr int
        CurrentVersion =
            1;

    static void save(
        const Simulation& simulation,
        const std::filesystem::path&
            path
    );

    [[nodiscard]]
    static std::unique_ptr<Simulation>
    load(
        const std::filesystem::path&
            path
    );
};

}

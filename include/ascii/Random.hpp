#pragma once

#include <cstdint>
#include <random>

namespace ascii
{
//random number gen
    class Random
    {
    public:
        explicit Random(std::uint64_t seed)
            : engine_(seed)
        {}

        int integer(int min, int max)
        {
            std::uniform_int_distribution<int>
                distribution(min, max);

            return distribution(engine_);
        }

    private:
        std::mt19937_64 engine_;
    };

}

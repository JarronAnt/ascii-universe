#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ascii
{

// Deterministic pseudo-random number generator.
//
// SplitMix64 is simple, fast, and most importantly for us,
// produces the exact same sequence for the same seed.
//
// Later we can save state() into the world save file and
// resume the random sequence exactly where we left off.
class Random
{
public:
    explicit Random(std::uint64_t seed)
        : state_(seed)
    {
    }

    // Reset the RNG to a specific seed.
    void seed(std::uint64_t value)
    {
        state_ = value;
    }

    // Current internal RNG state.
    //
    // Useful later for save/load.
    [[nodiscard]]
    std::uint64_t state() const
    {
        return state_;
    }

    // Restore an RNG state.
    void setState(std::uint64_t value)
    {
        state_ = value;
    }

    // Generate a raw 64-bit random value.
    std::uint64_t nextU64()
    {
        std::uint64_t z =
            (state_ += 0x9E3779B97F4A7C15ULL);

        z = (z ^ (z >> 30))
            * 0xBF58476D1CE4E5B9ULL;

        z = (z ^ (z >> 27))
            * 0x94D049BB133111EBULL;

        return z ^ (z >> 31);
    }

    // Generate an integer in the inclusive range:
    //
    //     [min, max]
    //
    // Rejection sampling avoids modulo bias.
    int integer(int min, int max)
    {
        assert(min <= max);

        const std::uint64_t range =
            static_cast<std::uint64_t>(
                static_cast<std::int64_t>(max)
                -
                static_cast<std::int64_t>(min)
            )
            + 1ULL;

        // Values below this threshold would create modulo bias.
        const std::uint64_t threshold =
            (0ULL - range) % range;

        std::uint64_t value;

        do
        {
            value = nextU64();
        }
        while (value < threshold);

        return static_cast<int>(
            static_cast<std::int64_t>(min)
            +
            static_cast<std::int64_t>(
                value % range
            )
        );
    }

    // Pick a random index:
    //
    //     [0, count - 1]
    std::size_t index(std::size_t count)
    {
        assert(count > 0);

        const std::uint64_t range =
            static_cast<std::uint64_t>(count);

        const std::uint64_t threshold =
            (0ULL - range) % range;

        std::uint64_t value;

        do
        {
            value = nextU64();
        }
        while (value < threshold);

        return static_cast<std::size_t>(
            value % range
        );
    }

    // Random double:
    //
    //     0.0 <= result < 1.0
    double unit()
    {
        constexpr double scale =
            1.0 / 9007199254740992.0;

        return static_cast<double>(
            nextU64() >> 11
        ) * scale;
    }

    bool chance(double probability)
    {
        assert(
            probability >= 0.0 &&
            probability <= 1.0
        );

        return unit() < probability;
    }

private:
    std::uint64_t state_;
};

}

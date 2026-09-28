#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ascii::worldgen
{

// ==================================================
// Stateless deterministic hashing
//
// These functions allow world generation to sample
// randomness at arbitrary coordinates without consuming
// the Simulation RNG.
//
// This is useful because:
//
//     same seed + same coordinate = same value
//
// regardless of generation traversal order.
// ==================================================

[[nodiscard]]
inline std::uint64_t mix64(
    std::uint64_t value
)
{
    value ^=
        value >> 30;

    value *=
        0xBF58476D1CE4E5B9ULL;

    value ^=
        value >> 27;

    value *=
        0x94D049BB133111EBULL;

    value ^=
        value >> 31;

    return value;
}

[[nodiscard]]
inline std::uint64_t coordinateHash(
    std::uint64_t seed,
    int x,
    int y,
    int z = 0
)
{
    std::uint64_t value =
        mix64(
            seed
            ^
            0x9E3779B97F4A7C15ULL
        );

    value ^=
        mix64(
            static_cast<std::uint64_t>(
                static_cast<std::int64_t>(
                    x
                )
            )
            +
            0x632BE59BD9B4E019ULL
        );

    value ^=
        mix64(
            static_cast<std::uint64_t>(
                static_cast<std::int64_t>(
                    y
                )
            )
            +
            0x8CB92BA72F3D8DD7ULL
        );

    value ^=
        mix64(
            static_cast<std::uint64_t>(
                static_cast<std::int64_t>(
                    z
                )
            )
            +
            0xA24BAED4963EE407ULL
        );

    return mix64(
        value
    );
}

[[nodiscard]]
inline double unitHash(
    std::uint64_t seed,
    int x,
    int y,
    int z = 0
)
{
    constexpr double Scale =
        1.0
        /
        9007199254740992.0;

    return
        static_cast<double>(
            coordinateHash(
                seed,
                x,
                y,
                z
            )
            >>
            11
        )
        *
        Scale;
}

[[nodiscard]]
inline double signedHash(
    std::uint64_t seed,
    int x,
    int y,
    int z = 0
)
{
    return
        unitHash(
            seed,
            x,
            y,
            z
        )
        *
        2.0
        -
        1.0;
}

// ==================================================
// Noise interpolation
// ==================================================

[[nodiscard]]
inline double fade(
    double value
)
{
    // Smoothstep curve.
    return
        value
        *
        value
        *
        (
            3.0
            -
            2.0
            *
            value
        );
}

[[nodiscard]]
inline double interpolate(
    double a,
    double b,
    double t
)
{
    return
        a
        +
        (
            b
            -
            a
        )
        *
        t;
}

// ==================================================
// 2D value noise
// ==================================================

[[nodiscard]]
inline double valueNoise2D(
    std::uint64_t seed,
    double x,
    double y
)
{
    const int x0 =
        static_cast<int>(
            std::floor(
                x
            )
        );

    const int y0 =
        static_cast<int>(
            std::floor(
                y
            )
        );

    const int x1 =
        x0 + 1;

    const int y1 =
        y0 + 1;

    const double tx =
        fade(
            x
            -
            static_cast<double>(
                x0
            )
        );

    const double ty =
        fade(
            y
            -
            static_cast<double>(
                y0
            )
        );

    const double a =
        interpolate(
            signedHash(
                seed,
                x0,
                y0
            ),
            signedHash(
                seed,
                x1,
                y0
            ),
            tx
        );

    const double b =
        interpolate(
            signedHash(
                seed,
                x0,
                y1
            ),
            signedHash(
                seed,
                x1,
                y1
            ),
            tx
        );

    return
        interpolate(
            a,
            b,
            ty
        );
}

// ==================================================
// 3D value noise
//
// Not heavily used yet, but worth having because
// future caves, aquifers and temperature fields can
// use continuous three-dimensional noise.
// ==================================================

[[nodiscard]]
inline double valueNoise3D(
    std::uint64_t seed,
    double x,
    double y,
    double z
)
{
    const int x0 =
        static_cast<int>(
            std::floor(x)
        );

    const int y0 =
        static_cast<int>(
            std::floor(y)
        );

    const int z0 =
        static_cast<int>(
            std::floor(z)
        );

    const int x1 =
        x0 + 1;

    const int y1 =
        y0 + 1;

    const int z1 =
        z0 + 1;

    const double tx =
        fade(
            x
            -
            static_cast<double>(
                x0
            )
        );

    const double ty =
        fade(
            y
            -
            static_cast<double>(
                y0
            )
        );

    const double tz =
        fade(
            z
            -
            static_cast<double>(
                z0
            )
        );

    const double c000 =
        signedHash(
            seed,
            x0,
            y0,
            z0
        );

    const double c100 =
        signedHash(
            seed,
            x1,
            y0,
            z0
        );

    const double c010 =
        signedHash(
            seed,
            x0,
            y1,
            z0
        );

    const double c110 =
        signedHash(
            seed,
            x1,
            y1,
            z0
        );

    const double c001 =
        signedHash(
            seed,
            x0,
            y0,
            z1
        );

    const double c101 =
        signedHash(
            seed,
            x1,
            y0,
            z1
        );

    const double c011 =
        signedHash(
            seed,
            x0,
            y1,
            z1
        );

    const double c111 =
        signedHash(
            seed,
            x1,
            y1,
            z1
        );

    const double x00 =
        interpolate(
            c000,
            c100,
            tx
        );

    const double x10 =
        interpolate(
            c010,
            c110,
            tx
        );

    const double x01 =
        interpolate(
            c001,
            c101,
            tx
        );

    const double x11 =
        interpolate(
            c011,
            c111,
            tx
        );

    const double y0v =
        interpolate(
            x00,
            x10,
            ty
        );

    const double y1v =
        interpolate(
            x01,
            x11,
            ty
        );

    return
        interpolate(
            y0v,
            y1v,
            tz
        );
}

// ==================================================
// Fractional Brownian motion
//
// Multiple differently-sized layers of value noise
// combine into terrain containing broad regions plus
// progressively smaller detail.
// ==================================================

[[nodiscard]]
inline double fbm2D(
    std::uint64_t seed,
    double x,
    double y,
    double baseScale,
    int octaves,
    double persistence = 0.5
)
{
    double amplitude =
        1.0;

    double scale =
        std::max(
            0.5,
            baseScale
        );

    double total =
        0.0;

    double normalization =
        0.0;

    for (
        int octave = 0;
        octave < octaves;
        ++octave
    )
    {
        total +=
            valueNoise2D(
                seed
                +
                static_cast<std::uint64_t>(
                    octave
                )
                *
                0x9E3779B97F4A7C15ULL,
                x / scale,
                y / scale
            )
            *
            amplitude;

        normalization +=
            amplitude;

        amplitude *=
            persistence;

        scale =
            std::max(
                0.5,
                scale * 0.5
            );
    }

    if (
        normalization ==
        0.0
    )
    {
        return 0.0;
    }

    return
        total
        /
        normalization;
}

// ==================================================
// Ridged noise
//
// Taking 1 - abs(noise) produces sharp ridge-like
// structures, which are useful for mountain chains.
// ==================================================

[[nodiscard]]
inline double ridged2D(
    std::uint64_t seed,
    double x,
    double y,
    double baseScale,
    int octaves,
    double persistence = 0.5
)
{
    double amplitude =
        1.0;

    double scale =
        std::max(
            0.5,
            baseScale
        );

    double total =
        0.0;

    double normalization =
        0.0;

    for (
        int octave = 0;
        octave < octaves;
        ++octave
    )
    {
        const double sample =
            valueNoise2D(
                seed
                +
                static_cast<std::uint64_t>(
                    octave
                )
                *
                0xD1B54A32D192ED03ULL,
                x / scale,
                y / scale
            );

        const double ridge =
            1.0
            -
            std::abs(
                sample
            );

        total +=
            ridge
            *
            amplitude;

        normalization +=
            amplitude;

        amplitude *=
            persistence;

        scale =
            std::max(
                0.5,
                scale * 0.5
            );
    }

    if (
        normalization ==
        0.0
    )
    {
        return 0.0;
    }

    return
        std::clamp(
            total
            /
            normalization,
            0.0,
            1.0
        );
}

[[nodiscard]]
inline double fbm3D(
    std::uint64_t seed,
    double x,
    double y,
    double z,
    double baseScale,
    int octaves,
    double persistence = 0.5
)
{
    double amplitude =
        1.0;

    double scale =
        std::max(
            0.5,
            baseScale
        );

    double total =
        0.0;

    double normalization =
        0.0;

    for (
        int octave = 0;
        octave < octaves;
        ++octave
    )
    {
        total +=
            valueNoise3D(
                seed
                +
                static_cast<std::uint64_t>(
                    octave
                )
                *
                0x94D049BB133111EBULL,
                x / scale,
                y / scale,
                z / scale
            )
            *
            amplitude;

        normalization +=
            amplitude;

        amplitude *=
            persistence;

        scale =
            std::max(
                0.5,
                scale * 0.5
            );
    }

    if (
        normalization ==
        0.0
    )
    {
        return 0.0;
    }

    return
        total
        /
        normalization;
}

}

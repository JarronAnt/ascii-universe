#pragma once

namespace ascii
{

enum class DesignationState
{
    Active,
    Ignored,
    Consumed
};

// Marker component.
//
// An entity with:
//     Position
//     MineDesignation
//     DesignationLifecycle
//
// represents a player mining designation.
struct MineDesignation
{
};

struct DesignationLifecycle
{
    DesignationState state{
        DesignationState::Active
    };
};

}

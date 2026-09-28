#pragma once

namespace ascii
{

enum class DesignationType
{
    Mine,
    DigDown,
    DigUp,
    FellTree
};

enum class DesignationState
{
    Active,
    Ignored,
    Consumed,
    Cancelled
};

struct Designation
{
    DesignationType type{
        DesignationType::Mine
    };
};

struct DesignationLifecycle
{
    DesignationState state{
        DesignationState::Active
    };
};

}

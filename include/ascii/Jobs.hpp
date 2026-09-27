#pragma once

#include "Position.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ascii
{

using JobId = std::uint64_t;

enum class JobType
{
    Mine,
    Haul
};

enum class JobState
{
    Available,
    Assigned,
    Complete,
    Cancelled
};

struct Job
{
    JobId id{};

    JobType type{
        JobType::Mine
    };

    // The thing being acted upon.
    //
    // For mining:
    // this is the WALL tile.
    Position target{};

    // Where the worker actually stands.
    //
    // Mining targets are walls and therefore aren't
    // walkable. The worker paths to a floor tile
    // adjacent to the wall.
    Position workPosition{};

    JobState state{
        JobState::Available
    };

    entt::entity worker{
        entt::null
    };

    // Designation that originally created this job.
    entt::entity sourceDesignation{
        entt::null
    };
};

// ECS component placed on a worker.
struct AssignedJob
{
    JobId id{};
};

class JobBoard
{
public:
    JobId add(
        JobType type,
        Position target,
        entt::entity sourceDesignation
    )
    {
        Job job;

        job.id =
            nextId_++;

        job.type =
            type;

        job.target =
            target;

        job.sourceDesignation =
            sourceDesignation;

        jobs_.push_back(job);

        return job.id;
    }

    Job* find(JobId id)
    {
        for (auto& job : jobs_)
        {
            if (job.id == id)
            {
                return &job;
            }
        }

        return nullptr;
    }

    const Job* find(JobId id) const
    {
        for (const auto& job : jobs_)
        {
            if (job.id == id)
            {
                return &job;
            }
        }

        return nullptr;
    }

    std::vector<Job>& jobs()
    {
        return jobs_;
    }

    const std::vector<Job>& jobs() const
    {
        return jobs_;
    }

    [[nodiscard]]
    bool hasUnfinished() const
    {
        for (const auto& job : jobs_)
        {
            if (
                job.state ==
                    JobState::Available
                ||
                job.state ==
                    JobState::Assigned
            )
            {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]]
    std::size_t count(
        JobState state
    ) const
    {
        std::size_t result = 0;

        for (const auto& job : jobs_)
        {
            if (job.state == state)
            {
                ++result;
            }
        }

        return result;
    }

private:
    JobId nextId_{1};

    std::vector<Job> jobs_;
};

}

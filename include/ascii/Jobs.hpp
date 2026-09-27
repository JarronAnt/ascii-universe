#pragma once

#include "Position.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ascii
{

using JobId =
    std::uint64_t;

enum class JobType
{
    Mine,

    DigDown,
    DigUp,

    FellTree,

    Haul
};

enum class JobState
{
    Available,
    Assigned,
    Complete,
    Cancelled
};

enum class HaulStage
{
    ToItem,
    ToStockpile
};

struct Job
{
    JobId id{};

    JobType type{
        JobType::Mine
    };

    Position target{};

    Position workPosition{};

    JobState state{
        JobState::Available
    };

    entt::entity worker{
        entt::null
    };

    entt::entity sourceDesignation{
        entt::null
    };

    // Hauling
    entt::entity item{
        entt::null
    };

    entt::entity destinationStockpile{
        entt::null
    };

    Position destination{};

    HaulStage haulStage{
        HaulStage::ToItem
    };
};

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

        jobs_.push_back(
            job
        );

        return job.id;
    }

    JobId addHaul(
        entt::entity item,
        entt::entity stockpile,
        Position destination
    )
    {
        Job job;

        job.id =
            nextId_++;

        job.type =
            JobType::Haul;

        job.item =
            item;

        job.destinationStockpile =
            stockpile;

        job.destination =
            destination;

        job.haulStage =
            HaulStage::ToItem;

        jobs_.push_back(
            job
        );

        return job.id;
    }

    Job* find(
        JobId id
    )
    {
        for (auto& job : jobs_)
        {
            if (
                job.id == id
            )
            {
                return &job;
            }
        }

        return nullptr;
    }

    const Job* find(
        JobId id
    ) const
    {
        for (
            const auto& job :
            jobs_
        )
        {
            if (
                job.id == id
            )
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

    const std::vector<Job>&
    jobs() const
    {
        return jobs_;
    }

    [[nodiscard]]
    bool hasUnfinished() const
    {
        for (
            const auto& job :
            jobs_
        )
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
        std::size_t result =
            0;

        for (
            const auto& job :
            jobs_
        )
        {
            if (
                job.state ==
                    state
            )
            {
                ++result;
            }
        }

        return result;
    }

    void restore(
        std::vector<Job> jobs
    )
    {
        jobs_ =
            std::move(jobs);

        nextId_ =
            1;

        for (
            const auto& job :
            jobs_
        )
        {
            if (
                job.id >=
                nextId_
            )
            {
                nextId_ =
                    job.id + 1;
            }
        }
    }

private:
    JobId nextId_{1};

    std::vector<Job>
        jobs_;
};

}

#include "ascii/systems/ExcavationSystems.hpp"

#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Tile.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace ascii::systems
{

namespace
{

bool isExcavationJob(
    JobType type
)
{
    return
        type == JobType::Mine
        ||
        type == JobType::DigDown
        ||
        type == JobType::DigUp;
}

void addUpConnection(
    Tile& tile
)
{
    switch (tile.shape)
    {
        case TileShape::DownStair:
            tile.shape =
                TileShape::UpDownStair;
            break;

        case TileShape::UpStair:
        case TileShape::UpDownStair:
            break;

        default:
            tile.shape =
                TileShape::UpStair;
            break;
    }
}

void addDownConnection(
    Tile& tile
)
{
    switch (tile.shape)
    {
        case TileShape::UpStair:
            tile.shape =
                TileShape::UpDownStair;
            break;

        case TileShape::DownStair:
        case TileShape::UpDownStair:
            break;

        default:
            tile.shape =
                TileShape::DownStair;
            break;
    }
}

void emitExcavatedMaterial(
    EventQueue<ItemSpawnEvent>&
        events,
    MaterialType material,
    Position position
)
{
    const auto itemType =
        itemTypeForMaterial(
            material
        );

    if (!itemType)
    {
        return;
    }

    events.emit(
        ItemSpawnEvent{
            *itemType,
            material,
            position,
            ItemSource::Digging
        }
    );
}

}

void assignExcavationJobs(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard
)
{
    auto view =
        registry.view<
            Goblin,
            Miner,
            Position
        >();

    std::vector<entt::entity>
        miners;

    for (
        auto entity :
        view
    )
    {
        miners.push_back(
            entity
        );
    }

    sortEntities(
        miners
    );

    for (
        auto miner :
        miners
    )
    {
        if (
            registry.all_of<
                AssignedJob
            >(miner)
        )
        {
            continue;
        }

        const Position start =
            registry.get<
                Position
            >(miner);

        for (
            auto& job :
            jobBoard.jobs()
        )
        {
            if (
                job.state !=
                    JobState::Available
                ||
                !isExcavationJob(
                    job.type
                )
            )
            {
                continue;
            }

            std::optional<
                std::vector<Position>
            > path;

            Position workPosition{};

            if (
                job.type ==
                    JobType::Mine
            )
            {
                auto approach =
                    findAdjacentApproach(
                        map,
                        pathfinder,
                        start,
                        job.target
                    );

                if (!approach)
                {
                    continue;
                }

                workPosition =
                    approach->
                        workPosition;

                path =
                    std::move(
                        approach->path
                    );
            }
            else
            {
                workPosition =
                    job.target;

                path =
                    pathfinder.findPath(
                        map,
                        start,
                        workPosition
                    );

                if (!path)
                {
                    continue;
                }
            }

            job.state =
                JobState::Assigned;

            job.worker =
                miner;

            job.workPosition =
                workPosition;

            auto& assigned =
                registry.emplace<
                    AssignedJob
                >(miner);

            assigned.id =
                job.id;

            setMovementPath(
                registry,
                miner,
                std::move(
                    *path
                )
            );

            break;
        }
    }
}

void executeExcavation(
    entt::registry& registry,
    GameMap& map,
    JobBoard& jobBoard,
    EventQueue<ItemSpawnEvent>&
        itemSpawnEvents
)
{
    auto view =
        registry.view<
            Goblin,
            Miner,
            Position,
            AssignedJob
        >();

    std::vector<entt::entity>
        workers;

    for (
        auto entity :
        view
    )
    {
        workers.push_back(
            entity
        );
    }

    sortEntities(
        workers
    );

    std::vector<entt::entity>
        clear;

    for (
        auto worker :
        workers
    )
    {
        const auto assigned =
            registry.get<
                AssignedJob
            >(worker);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (
            job == nullptr
            ||
            !isExcavationJob(
                job->type
            )
        )
        {
            continue;
        }

        if (
            job->state !=
                JobState::Assigned
            ||
            job->worker !=
                worker
        )
        {
            clear.push_back(
                worker
            );

            continue;
        }

        if (
            registry.all_of<
                MovementPath
            >(worker)
            &&
            !registry.get<
                MovementPath
            >(worker).finished()
        )
        {
            continue;
        }

        const Position workerPosition =
            registry.get<
                Position
            >(worker);

        bool success =
            false;

        // ==========================================
        // Horizontal mining
        // ==========================================

        if (
            job->type ==
                JobType::Mine
        )
        {
            if (
                workerPosition ==
                    job->workPosition
                &&
                isAdjacentSameLevel(
                    workerPosition,
                    job->target
                )
                &&
                map.inBounds(
                    job->target
                )
            )
            {
                Tile& target =
                    map.at(
                        job->target
                    );

                if (
                    target.shape ==
                        TileShape::Wall
                )
                {
                    const MaterialType
                        material =
                            target.material;

                    target.shape =
                        TileShape::Floor;

                    target.feature =
                        TileFeature::None;

                    emitExcavatedMaterial(
                        itemSpawnEvents,
                        material,
                        job->target
                    );

                    success =
                        true;
                }
            }
        }

        // ==========================================
        // Dig downward
        // ==========================================

        else if (
            job->type ==
                JobType::DigDown
        )
        {
            if (
                workerPosition ==
                    job->target
                &&
                job->target.z > 0
            )
            {
                const Position below{
                    job->target.x,
                    job->target.y,
                    job->target.z - 1
                };

                if (
                    map.inBounds(
                        below
                    )
                    &&
                    map.at(
                        below
                    ).shape ==
                        TileShape::Wall
                )
                {
                    Tile& current =
                        map.at(
                            job->target
                        );

                    Tile& lower =
                        map.at(
                            below
                        );

                    const MaterialType
                        material =
                            lower.material;

                    addDownConnection(
                        current
                    );

                    addUpConnection(
                        lower
                    );

                    emitExcavatedMaterial(
                        itemSpawnEvents,
                        material,
                        job->target
                    );

                    success =
                        true;
                }
            }
        }

        // ==========================================
        // Dig upward
        // ==========================================

        else if (
            job->type ==
                JobType::DigUp
        )
        {
            if (
                workerPosition ==
                    job->target
                &&
                job->target.z <
                    map.depth() - 1
            )
            {
                const Position above{
                    job->target.x,
                    job->target.y,
                    job->target.z + 1
                };

                if (
                    map.inBounds(
                        above
                    )
                    &&
                    map.at(
                        above
                    ).shape ==
                        TileShape::Wall
                )
                {
                    Tile& current =
                        map.at(
                            job->target
                        );

                    Tile& upper =
                        map.at(
                            above
                        );

                    const MaterialType
                        material =
                            upper.material;

                    addUpConnection(
                        current
                    );

                    addDownConnection(
                        upper
                    );

                    emitExcavatedMaterial(
                        itemSpawnEvents,
                        material,
                        job->target
                    );

                    success =
                        true;
                }
            }
        }

        if (success)
        {
            job->state =
                JobState::Complete;
        }
        else
        {
            job->state =
                JobState::Cancelled;
        }

        job->worker =
            entt::null;

        hideDesignation(
            registry,
            job->
                sourceDesignation
        );

        clear.push_back(
            worker
        );
    }

    for (
        auto worker :
        clear
    )
    {
        clearWorkerJob(
            registry,
            worker
        );
    }
}

}

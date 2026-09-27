#include "ascii/SaveManager.hpp"

#include "ascii/Color.hpp"
#include "ascii/Components.hpp"
#include "ascii/Designations.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ascii
{

namespace
{

using json =
    nlohmann::json;

// ==================================================
// Basic conversions
// ==================================================

json positionToJson(
    Position position
)
{
    return {
        {"x", position.x},
        {"y", position.y}
    };
}

Position positionFromJson(
    const json& value
)
{
    return Position{
        value.at("x").
            get<int>(),

        value.at("y").
            get<int>()
    };
}

json entityReference(
    entt::entity entity
)
{
    if (
        entity ==
        entt::null
    )
    {
        return nullptr;
    }

    return static_cast<
        std::uint32_t
    >(
        entt::to_integral(
            entity
        )
    );
}

entt::entity
entityReferenceFromJson(
    const json& value
)
{
    if (value.is_null())
    {
        return entt::null;
    }

    return static_cast<
        entt::entity
    >(
        value.get<
            std::uint32_t
        >()
    );
}

std::uint32_t entityId(
    entt::entity entity
)
{
    return static_cast<
        std::uint32_t
    >(
        entt::to_integral(
            entity
        )
    );
}

entt::entity createExactEntity(
    entt::registry& registry,
    std::uint32_t id
)
{
    const auto requested =
        static_cast<
            entt::entity
        >(id);

    const auto created =
        registry.create(
            requested
        );

    if (
        created !=
        requested
    )
    {
        throw std::runtime_error(
            "Failed to recreate saved "
            "EnTT entity identifier."
        );
    }

    return created;
}

// ==================================================
// Glyph
// ==================================================

json glyphToJson(
    const Glyph& glyph
)
{
    return {
        {
            "character",
            std::string(
                1,
                glyph.character
            )
        },
        {
            "color",
            static_cast<int>(
                glyph.color
            )
        }
    };
}

Glyph glyphFromJson(
    const json& value
)
{
    Glyph glyph;

    const std::string character =
        value.at(
            "character"
        ).get<
            std::string
        >();

    glyph.character =
        character.empty()
        ?
        '?'
        :
        character.front();

    glyph.color =
        static_cast<
            TerminalColor
        >(
            value.at(
                "color"
            ).get<int>()
        );

    return glyph;
}

// ==================================================
// Movement path
// ==================================================

json movementPathToJson(
    const MovementPath& path
)
{
    json result;

    result["next_step"] =
        path.nextStep;

    result["nodes"] =
        json::array();

    for (
        const auto& node :
        path.nodes
    )
    {
        result["nodes"].
            push_back(
                positionToJson(
                    node
                )
            );
    }

    return result;
}

MovementPath movementPathFromJson(
    const json& value
)
{
    MovementPath path;

    path.nextStep =
        value.at(
            "next_step"
        ).get<
            std::size_t
        >();

    for (
        const auto& node :
        value.at("nodes")
    )
    {
        path.nodes.push_back(
            positionFromJson(
                node
            )
        );
    }

    return path;
}

// ==================================================
// Save entities
// ==================================================

json saveGoblins(
    const entt::registry&
        registry
)
{
    json result =
        json::array();

    auto view =
        registry.view<
            Goblin
        >();

    for (auto entity : view)
    {
        json record;

        record["id"] =
            entityId(entity);

        record["miner"] =
            registry.all_of<
                Miner
            >(entity);

        record["hauler"] =
            registry.all_of<
                Hauler
            >(entity);

        if (
            registry.all_of<
                Name
            >(entity)
        )
        {
            record["name"] =
                registry.get<
                    Name
                >(entity).value;
        }
        else
        {
            record["name"] =
                nullptr;
        }

        if (
            registry.all_of<
                Position
            >(entity)
        )
        {
            record["position"] =
                positionToJson(
                    registry.get<
                        Position
                    >(entity)
                );
        }
        else
        {
            record["position"] =
                nullptr;
        }

        if (
            registry.all_of<
                Glyph
            >(entity)
        )
        {
            record["glyph"] =
                glyphToJson(
                    registry.get<
                        Glyph
                    >(entity)
                );
        }
        else
        {
            record["glyph"] =
                nullptr;
        }

        if (
            registry.all_of<
                AssignedJob
            >(entity)
        )
        {
            record[
                "assigned_job"
            ] =
                registry.get<
                    AssignedJob
                >(entity).id;
        }
        else
        {
            record[
                "assigned_job"
            ] =
                nullptr;
        }

        if (
            registry.all_of<
                MovementPath
            >(entity)
        )
        {
            record[
                "movement_path"
            ] =
                movementPathToJson(
                    registry.get<
                        MovementPath
                    >(entity)
                );
        }
        else
        {
            record[
                "movement_path"
            ] =
                nullptr;
        }

        if (
            registry.all_of<
                CarryingItem
            >(entity)
        )
        {
            record[
                "carrying_item"
            ] =
                entityReference(
                    registry.get<
                        CarryingItem
                    >(entity).item
                );
        }
        else
        {
            record[
                "carrying_item"
            ] =
                nullptr;
        }

        result.push_back(
            std::move(record)
        );
    }

    return result;
}

json saveDesignations(
    const entt::registry&
        registry
)
{
    json result =
        json::array();

    auto view =
        registry.view<
            MineDesignation
        >();

    for (auto entity : view)
    {
        json record;

        record["id"] =
            entityId(entity);

        if (
            registry.all_of<
                Position
            >(entity)
        )
        {
            record["position"] =
                positionToJson(
                    registry.get<
                        Position
                    >(entity)
                );
        }

        if (
            registry.all_of<
                DesignationLifecycle
            >(entity)
        )
        {
            record["state"] =
                static_cast<int>(
                    registry.get<
                        DesignationLifecycle
                    >(entity).state
                );
        }

        if (
            registry.all_of<
                Glyph
            >(entity)
        )
        {
            record["glyph"] =
                glyphToJson(
                    registry.get<
                        Glyph
                    >(entity)
                );
        }
        else
        {
            record["glyph"] =
                nullptr;
        }

        result.push_back(
            std::move(record)
        );
    }

    return result;
}

json saveItems(
    const entt::registry&
        registry
)
{
    json result =
        json::array();

    auto view =
        registry.view<
            Item
        >();

    for (auto entity : view)
    {
        const auto& item =
            registry.get<
                Item
            >(entity);

        json record;

        record["id"] =
            entityId(entity);

        record["type"] =
            static_cast<int>(
                item.type
            );

        record["weight"] =
            item.weight;

        record["carriable"] =
            registry.all_of<
                Carriable
            >(entity);

        if (
            registry.all_of<
                ItemState
            >(entity)
        )
        {
            const auto& state =
                registry.get<
                    ItemState
                >(entity);

            record["state"] = {
                {
                    "location",
                    static_cast<int>(
                        state.location
                    )
                },
                {
                    "carrier",
                    entityReference(
                        state.carrier
                    )
                },
                {
                    "stockpile",
                    entityReference(
                        state.stockpile
                    )
                }
            };
        }

        if (
            registry.all_of<
                Position
            >(entity)
        )
        {
            record["position"] =
                positionToJson(
                    registry.get<
                        Position
                    >(entity)
                );
        }
        else
        {
            record["position"] =
                nullptr;
        }

        if (
            registry.all_of<
                Glyph
            >(entity)
        )
        {
            record["glyph"] =
                glyphToJson(
                    registry.get<
                        Glyph
                    >(entity)
                );
        }
        else
        {
            record["glyph"] =
                nullptr;
        }

        result.push_back(
            std::move(record)
        );
    }

    return result;
}

json saveStockpiles(
    const entt::registry&
        registry
)
{
    json result =
        json::array();

    auto view =
        registry.view<
            Stockpile
        >();

    for (auto entity : view)
    {
        const auto& stockpile =
            registry.get<
                Stockpile
            >(entity);

        json record;

        record["id"] =
            entityId(entity);

        record["bounds"] = {
            {
                "top_left",
                positionToJson(
                    stockpile.
                        bounds.
                        topLeft
                )
            },
            {
                "bottom_right",
                positionToJson(
                    stockpile.
                        bounds.
                        bottomRight
                )
            }
        };

        record["accepts"] =
            json::array();

        for (
            const auto type :
            stockpile.accepts
        )
        {
            record[
                "accepts"
            ].push_back(
                static_cast<int>(
                    type
                )
            );
        }

        record["current_items"] =
            json::array();

        for (
            const auto item :
            stockpile.currentItems
        )
        {
            record[
                "current_items"
            ].push_back(
                entityReference(
                    item
                )
            );
        }

        record["reserved_cells"] =
            json::array();

        for (
            const auto position :
            stockpile.reservedCells
        )
        {
            record[
                "reserved_cells"
            ].push_back(
                positionToJson(
                    position
                )
            );
        }

        if (
            stockpile.maxItems
        )
        {
            record["max_items"] =
                *stockpile.maxItems;
        }
        else
        {
            record["max_items"] =
                nullptr;
        }

        result.push_back(
            std::move(record)
        );
    }

    return result;
}

// ==================================================
// Jobs
// ==================================================

json saveJobs(
    const JobBoard&
        jobBoard
)
{
    json result =
        json::array();

    for (
        const auto& job :
        jobBoard.jobs()
    )
    {
        result.push_back({
            {"id", job.id},

            {
                "type",
                static_cast<int>(
                    job.type
                )
            },

            {
                "target",
                positionToJson(
                    job.target
                )
            },

            {
                "work_position",
                positionToJson(
                    job.workPosition
                )
            },

            {
                "state",
                static_cast<int>(
                    job.state
                )
            },

            {
                "worker",
                entityReference(
                    job.worker
                )
            },

            {
                "source_designation",
                entityReference(
                    job.sourceDesignation
                )
            },

            {
                "item",
                entityReference(
                    job.item
                )
            },

            {
                "destination_stockpile",
                entityReference(
                    job.destinationStockpile
                )
            },

            {
                "destination",
                positionToJson(
                    job.destination
                )
            },

            {
                "haul_stage",
                static_cast<int>(
                    job.haulStage
                )
            }
        });
    }

    return result;
}

// ==================================================
// Loading entity categories
// ==================================================

void loadGoblins(
    entt::registry& registry,
    const json& records
)
{
    for (
        const auto& record :
        records
    )
    {
        const auto entity =
            createExactEntity(
                registry,
                record.at("id").
                    get<
                        std::uint32_t
                    >()
            );

        registry.emplace<
            Goblin
        >(entity);

        if (
            record.value(
                "miner",
                false
            )
        )
        {
            registry.emplace<
                Miner
            >(entity);
        }

        if (
            record.value(
                "hauler",
                false
            )
        )
        {
            registry.emplace<
                Hauler
            >(entity);
        }

        if (
            !record.at(
                "name"
            ).is_null()
        )
        {
            auto& name =
                registry.emplace<
                    Name
                >(entity);

            name.value =
                record.at(
                    "name"
                ).get<
                    std::string
                >();
        }

        if (
            !record.at(
                "position"
            ).is_null()
        )
        {
            auto& position =
                registry.emplace<
                    Position
                >(entity);

            position =
                positionFromJson(
                    record.at(
                        "position"
                    )
                );
        }

        if (
            !record.at(
                "glyph"
            ).is_null()
        )
        {
            auto& glyph =
                registry.emplace<
                    Glyph
                >(entity);

            glyph =
                glyphFromJson(
                    record.at(
                        "glyph"
                    )
                );
        }

        if (
            !record.at(
                "assigned_job"
            ).is_null()
        )
        {
            auto& assigned =
                registry.emplace<
                    AssignedJob
                >(entity);

            assigned.id =
                record.at(
                    "assigned_job"
                ).get<JobId>();
        }

        if (
            !record.at(
                "movement_path"
            ).is_null()
        )
        {
            auto& path =
                registry.emplace<
                    MovementPath
                >(entity);

            path =
                movementPathFromJson(
                    record.at(
                        "movement_path"
                    )
                );
        }

        if (
            !record.at(
                "carrying_item"
            ).is_null()
        )
        {
            auto& carrying =
                registry.emplace<
                    CarryingItem
                >(entity);

            carrying.item =
                entityReferenceFromJson(
                    record.at(
                        "carrying_item"
                    )
                );
        }
    }
}

void loadDesignations(
    entt::registry& registry,
    const json& records
)
{
    for (
        const auto& record :
        records
    )
    {
        const auto entity =
            createExactEntity(
                registry,
                record.at("id").
                    get<
                        std::uint32_t
                    >()
            );

        registry.emplace<
            MineDesignation
        >(entity);

        auto& lifecycle =
            registry.emplace<
                DesignationLifecycle
            >(entity);

        lifecycle.state =
            static_cast<
                DesignationState
            >(
                record.at(
                    "state"
                ).get<int>()
            );

        auto& position =
            registry.emplace<
                Position
            >(entity);

        position =
            positionFromJson(
                record.at(
                    "position"
                )
            );

        if (
            !record.at(
                "glyph"
            ).is_null()
        )
        {
            auto& glyph =
                registry.emplace<
                    Glyph
                >(entity);

            glyph =
                glyphFromJson(
                    record.at(
                        "glyph"
                    )
                );
        }
    }
}

void loadItems(
    entt::registry& registry,
    const json& records
)
{
    for (
        const auto& record :
        records
    )
    {
        const auto entity =
            createExactEntity(
                registry,
                record.at("id").
                    get<
                        std::uint32_t
                    >()
            );

        auto& item =
            registry.emplace<
                Item
            >(entity);

        item.type =
            static_cast<
                ItemType
            >(
                record.at(
                    "type"
                ).get<int>()
            );

        item.weight =
            record.at(
                "weight"
            ).get<
                std::uint32_t
            >();

        if (
            record.value(
                "carriable",
                false
            )
        )
        {
            registry.emplace<
                Carriable
            >(entity);
        }

        if (
            record.contains(
                "state"
            )
        )
        {
            auto& state =
                registry.emplace<
                    ItemState
                >(entity);

            const auto&
                stateJson =
                    record.at(
                        "state"
                    );

            state.location =
                static_cast<
                    ItemLocation
                >(
                    stateJson.at(
                        "location"
                    ).get<int>()
                );

            state.carrier =
                entityReferenceFromJson(
                    stateJson.at(
                        "carrier"
                    )
                );

            state.stockpile =
                entityReferenceFromJson(
                    stateJson.at(
                        "stockpile"
                    )
                );
        }

        if (
            !record.at(
                "position"
            ).is_null()
        )
        {
            auto& position =
                registry.emplace<
                    Position
                >(entity);

            position =
                positionFromJson(
                    record.at(
                        "position"
                    )
                );
        }

        if (
            !record.at(
                "glyph"
            ).is_null()
        )
        {
            auto& glyph =
                registry.emplace<
                    Glyph
                >(entity);

            glyph =
                glyphFromJson(
                    record.at(
                        "glyph"
                    )
                );
        }
    }
}

void loadStockpiles(
    entt::registry& registry,
    const json& records
)
{
    for (
        const auto& record :
        records
    )
    {
        const auto entity =
            createExactEntity(
                registry,
                record.at("id").
                    get<
                        std::uint32_t
                    >()
            );

        auto& stockpile =
            registry.emplace<
                Stockpile
            >(entity);

        stockpile.bounds.topLeft =
            positionFromJson(
                record.at(
                    "bounds"
                ).at(
                    "top_left"
                )
            );

        stockpile.bounds.bottomRight =
            positionFromJson(
                record.at(
                    "bounds"
                ).at(
                    "bottom_right"
                )
            );

        for (
            const auto& type :
            record.at(
                "accepts"
            )
        )
        {
            stockpile.accepts.push_back(
                static_cast<
                    ItemType
                >(
                    type.get<int>()
                )
            );
        }

        for (
            const auto& item :
            record.at(
                "current_items"
            )
        )
        {
            stockpile.
                currentItems.
                push_back(
                    entityReferenceFromJson(
                        item
                    )
                );
        }

        for (
            const auto& position :
            record.at(
                "reserved_cells"
            )
        )
        {
            stockpile.
                reservedCells.
                push_back(
                    positionFromJson(
                        position
                    )
                );
        }

        if (
            !record.at(
                "max_items"
            ).is_null()
        )
        {
            stockpile.maxItems =
                record.at(
                    "max_items"
                ).get<
                    std::size_t
                >();
        }
    }
}

std::vector<Job>
loadJobs(
    const json& records
)
{
    std::vector<Job>
        jobs;

    for (
        const auto& record :
        records
    )
    {
        Job job;

        job.id =
            record.at(
                "id"
            ).get<JobId>();

        job.type =
            static_cast<
                JobType
            >(
                record.at(
                    "type"
                ).get<int>()
            );

        job.target =
            positionFromJson(
                record.at(
                    "target"
                )
            );

        job.workPosition =
            positionFromJson(
                record.at(
                    "work_position"
                )
            );

        job.state =
            static_cast<
                JobState
            >(
                record.at(
                    "state"
                ).get<int>()
            );

        job.worker =
            entityReferenceFromJson(
                record.at(
                    "worker"
                )
            );

        job.sourceDesignation =
            entityReferenceFromJson(
                record.at(
                    "source_designation"
                )
            );

        job.item =
            entityReferenceFromJson(
                record.at(
                    "item"
                )
            );

        job.destinationStockpile =
            entityReferenceFromJson(
                record.at(
                    "destination_stockpile"
                )
            );

        job.destination =
            positionFromJson(
                record.at(
                    "destination"
                )
            );

        job.haulStage =
            static_cast<
                HaulStage
            >(
                record.at(
                    "haul_stage"
                ).get<int>()
            );

        jobs.push_back(
            std::move(job)
        );
    }

    return jobs;
}

}

void SaveManager::save(
    const Simulation& simulation,
    const std::filesystem::path&
        path
)
{
    json root;

    root["format"] =
        "ascii-universe-save";

    root["version"] =
        CurrentVersion;

    // ==================================================
    // World
    // ==================================================

    const GameMap& map =
        simulation.map();

    root["world"] = {
        {
            "seed",
            simulation.worldSeed()
        },
        {
            "width",
            map.width()
        },
        {
            "height",
            map.height()
        }
    };

    root["world"]["tiles"] =
        json::array();

    for (
        int y = 0;
        y < map.height();
        ++y
    )
    {
        for (
            int x = 0;
            x < map.width();
            ++x
        )
        {
            root[
                "world"
            ][
                "tiles"
            ].push_back(
                static_cast<int>(
                    map.at(
                        x,
                        y
                    ).type
                )
            );
        }
    }

    // ==================================================
    // Runtime state
    // ==================================================

    root["simulation"] = {
        {
            "tick",
            simulation.time().tick
        },
        {
            "rng_state",
            simulation.random().
                state()
        }
    };

    // ==================================================
    // ECS
    // ==================================================

    const auto& registry =
        simulation.registry();

    root["entities"] = {
        {
            "goblins",
            saveGoblins(
                registry
            )
        },
        {
            "designations",
            saveDesignations(
                registry
            )
        },
        {
            "items",
            saveItems(
                registry
            )
        },
        {
            "stockpiles",
            saveStockpiles(
                registry
            )
        }
    };

    root["jobs"] =
        saveJobs(
            simulation.jobBoard()
        );

    // ==================================================
    // Write atomically
    // ==================================================

    if (
        path.has_parent_path()
    )
    {
        std::filesystem::
            create_directories(
                path.parent_path()
            );
    }

    std::filesystem::path
        temporary =
            path;

    temporary +=
        ".tmp";

    {
        std::ofstream output(
            temporary
        );

        if (!output)
        {
            throw std::runtime_error(
                "Unable to open temporary "
                "save file."
            );
        }

        output
            << root.dump(2);

        output.flush();

        if (!output)
        {
            throw std::runtime_error(
                "Failed while writing "
                "save file."
            );
        }
    }

    std::filesystem::rename(
        temporary,
        path
    );
}

std::unique_ptr<Simulation>
SaveManager::load(
    const std::filesystem::path&
        path
)
{
    std::ifstream input(
        path
    );

    if (!input)
    {
        throw std::runtime_error(
            "Unable to open save file: "
            +
            path.string()
        );
    }

    json root;

    input >> root;

    if (
        root.at(
            "format"
        ).get<
            std::string
        >()
        !=
        "ascii-universe-save"
    )
    {
        throw std::runtime_error(
            "Not an ASCII Universe save."
        );
    }

    const int version =
        root.at(
            "version"
        ).get<int>();

    if (
        version !=
        CurrentVersion
    )
    {
        throw std::runtime_error(
            "Unsupported save version: "
            +
            std::to_string(
                version
            )
        );
    }

    const auto& world =
        root.at(
            "world"
        );

    const int width =
        world.at(
            "width"
        ).get<int>();

    const int height =
        world.at(
            "height"
        ).get<int>();

    const auto seed =
        world.at(
            "seed"
        ).get<
            std::uint64_t
        >();

    auto simulation =
        std::make_unique<
            Simulation
        >(
            width,
            height,
            seed
        );

    // ==================================================
    // Restore tiles
    // ==================================================

    const auto& tiles =
        world.at(
            "tiles"
        );

    const std::size_t expected =
        static_cast<
            std::size_t
        >(
            width * height
        );

    if (
        tiles.size() !=
        expected
    )
    {
        throw std::runtime_error(
            "Save file contains an "
            "invalid tile count."
        );
    }

    std::size_t tileIndex =
        0;

    for (
        int y = 0;
        y < height;
        ++y
    )
    {
        for (
            int x = 0;
            x < width;
            ++x
        )
        {
            simulation->
                map().
                at(
                    x,
                    y
                ).type =
                    static_cast<
                        TileType
                    >(
                        tiles[
                            tileIndex++
                        ].get<int>()
                    );
        }
    }

    // ==================================================
    // Restore entities
    // ==================================================

    auto& registry =
        simulation->
            registry();

    const auto& entities =
        root.at(
            "entities"
        );

    loadGoblins(
        registry,
        entities.at(
            "goblins"
        )
    );

    loadDesignations(
        registry,
        entities.at(
            "designations"
        )
    );

    loadItems(
        registry,
        entities.at(
            "items"
        )
    );

    loadStockpiles(
        registry,
        entities.at(
            "stockpiles"
        )
    );

    // ==================================================
    // Restore jobs
    // ==================================================

    simulation->
        jobBoard().
        restore(
            loadJobs(
                root.at(
                    "jobs"
                )
            )
        );

    // ==================================================
    // Restore simulation clock + RNG
    // ==================================================

    const auto& runtime =
        root.at(
            "simulation"
        );

    simulation->
        restoreRuntimeState(
            runtime.at(
                "tick"
            ).get<
                std::uint64_t
            >(),

            runtime.at(
                "rng_state"
            ).get<
                std::uint64_t
            >()
        );

    return simulation;
}

}

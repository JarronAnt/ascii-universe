#include "ascii/SaveManager.hpp"

#include "ascii/Components.hpp"
#include "ascii/Designations.hpp"
#include "ascii/Items.hpp"
#include "ascii/Jobs.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/Tile.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
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
// Basic helpers
// ==================================================

json positionToJson(
    Position position
)
{
    return {
        {"x", position.x},
        {"y", position.y},
        {"z", position.z}
    };
}

Position positionFromJson(
    const json& value
)
{
    return Position{
        value.at("x").get<int>(),
        value.at("y").get<int>(),
        value.at("z").get<int>()
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

    return
        static_cast<
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
    if (
        value.is_null()
    )
    {
        return entt::null;
    }

    return
        static_cast<
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
    return
        static_cast<
            std::uint32_t
        >(
            entt::to_integral(
                entity
            )
        );
}

entt::entity entityFromId(
    std::uint32_t id
)
{
    return
        static_cast<
            entt::entity
        >(id);
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

    const auto character =
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
        const auto position :
        path.nodes
    )
    {
        result[
            "nodes"
        ].push_back(
            positionToJson(
                position
            )
        );
    }

    return result;
}

MovementPath
movementPathFromJson(
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
        value.at(
            "nodes"
        )
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
// Entity collection
// ==================================================

std::vector<entt::entity>
collectEntities(
    const entt::registry& registry
)
{
    std::vector<entt::entity>
        entities;

    const auto collect =
        [&entities](auto view)
        {
            for (
                auto entity :
                view
            )
            {
                entities.push_back(
                    entity
                );
            }
        };

    collect(
        registry.view<
            Goblin
        >()
    );

    collect(
        registry.view<
            Designation
        >()
    );

    collect(
        registry.view<
            Item
        >()
    );

    collect(
        registry.view<
            Stockpile
        >()
    );

    std::sort(
        entities.begin(),
        entities.end(),
        [](
            entt::entity lhs,
            entt::entity rhs
        )
        {
            return
                entt::to_integral(lhs)
                <
                entt::to_integral(rhs);
        }
    );

    entities.erase(
        std::unique(
            entities.begin(),
            entities.end()
        ),
        entities.end()
    );

    return entities;
}

// ==================================================
// Entity serialization
// ==================================================

json serializeEntity(
    const entt::registry& registry,
    entt::entity entity
)
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

    record["tags"] = {
        {
            "goblin",
            registry.all_of<
                Goblin
            >(entity)
        },
        {
            "miner",
            registry.all_of<
                Miner
            >(entity)
        },
        {
            "hauler",
            registry.all_of<
                Hauler
            >(entity)
        },
        {
            "woodcutter",
            registry.all_of<
                Woodcutter
            >(entity)
        }
    };

    if (
        registry.all_of<
            AssignedJob
        >(entity)
    )
    {
        record["assigned_job"] =
            registry.get<
                AssignedJob
            >(entity).id;
    }

    if (
        registry.all_of<
            MovementPath
        >(entity)
    )
    {
        record["movement_path"] =
            movementPathToJson(
                registry.get<
                    MovementPath
                >(entity)
            );
    }

    if (
        registry.all_of<
            CarryingItem
        >(entity)
    )
    {
        record["carrying_item"] =
            entityReference(
                registry.get<
                    CarryingItem
                >(entity).item
            );
    }

    // ==================================================
    // Designation
    // ==================================================

    if (
        registry.all_of<
            Designation,
            DesignationLifecycle
        >(entity)
    )
    {
        const auto& designation =
            registry.get<
                Designation
            >(entity);

        const auto& lifecycle =
            registry.get<
                DesignationLifecycle
            >(entity);

        record["designation"] = {
            {
                "type",
                static_cast<int>(
                    designation.type
                )
            },
            {
                "state",
                static_cast<int>(
                    lifecycle.state
                )
            }
        };
    }

    // ==================================================
    // Item
    // ==================================================

    if (
        registry.all_of<
            Item
        >(entity)
    )
    {
        const auto& item =
            registry.get<
                Item
            >(entity);

        json itemJson{
            {
                "type",
                static_cast<int>(
                    item.type
                )
            },
            {
                "material",
                static_cast<int>(
                    item.material
                )
            },
            {
                "weight",
                item.weight
            },
            {
                "carriable",
                registry.all_of<
                    Carriable
                >(entity)
            }
        };

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

            itemJson["state"] = {
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

        record["item"] =
            std::move(
                itemJson
            );
    }

    // ==================================================
    // Stockpile
    // ==================================================

    if (
        registry.all_of<
            Stockpile
        >(entity)
    )
    {
        const auto& stockpile =
            registry.get<
                Stockpile
            >(entity);

        json stockpileJson;

        stockpileJson["bounds"] = {
            {
                "min",
                positionToJson(
                    stockpile.bounds.min
                )
            },
            {
                "max",
                positionToJson(
                    stockpile.bounds.max
                )
            }
        };

        stockpileJson["accepts"] =
            json::array();

        for (
            const auto type :
            stockpile.accepts
        )
        {
            stockpileJson[
                "accepts"
            ].push_back(
                static_cast<int>(
                    type
                )
            );
        }

        stockpileJson[
            "current_items"
        ] =
            json::array();

        for (
            const auto item :
            stockpile.currentItems
        )
        {
            stockpileJson[
                "current_items"
            ].push_back(
                entityReference(
                    item
                )
            );
        }

        stockpileJson[
            "reserved_cells"
        ] =
            json::array();

        for (
            const auto position :
            stockpile.reservedCells
        )
        {
            stockpileJson[
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
            stockpileJson[
                "max_items"
            ] =
                *stockpile.maxItems;
        }
        else
        {
            stockpileJson[
                "max_items"
            ] =
                nullptr;
        }

        record["stockpile"] =
            std::move(
                stockpileJson
            );
    }

    return record;
}

// ==================================================
// Entity loading
// ==================================================

void createEntityIds(
    entt::registry& registry,
    const json& entities
)
{
    for (
        const auto& record :
        entities
    )
    {
        const auto requested =
            entityFromId(
                record.at(
                    "id"
                ).get<
                    std::uint32_t
                >()
            );

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
                "Could not restore saved entity ID."
            );
        }
    }
}

void loadEntityComponents(
    entt::registry& registry,
    const json& entities
)
{
    for (
        const auto& record :
        entities
    )
    {
        const auto entity =
            entityFromId(
                record.at(
                    "id"
                ).get<
                    std::uint32_t
                >()
            );

        if (
            record.contains(
                "position"
            )
        )
        {
            registry.emplace<
                Position
            >(entity) =
                positionFromJson(
                    record.at(
                        "position"
                    )
                );
        }

        if (
            record.contains(
                "glyph"
            )
        )
        {
            registry.emplace<
                Glyph
            >(entity) =
                glyphFromJson(
                    record.at(
                        "glyph"
                    )
                );
        }

        if (
            record.contains(
                "name"
            )
        )
        {
            registry.emplace<
                Name
            >(entity).value =
                record.at(
                    "name"
                ).get<
                    std::string
                >();
        }

        const auto& tags =
            record.at(
                "tags"
            );

        if (
            tags.value(
                "goblin",
                false
            )
        )
        {
            registry.emplace<
                Goblin
            >(entity);
        }

        if (
            tags.value(
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
            tags.value(
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
            tags.value(
                "woodcutter",
                false
            )
        )
        {
            registry.emplace<
                Woodcutter
            >(entity);
        }

        if (
            record.contains(
                "assigned_job"
            )
        )
        {
            registry.emplace<
                AssignedJob
            >(entity).id =
                record.at(
                    "assigned_job"
                ).get<JobId>();
        }

        if (
            record.contains(
                "movement_path"
            )
        )
        {
            registry.emplace<
                MovementPath
            >(entity) =
                movementPathFromJson(
                    record.at(
                        "movement_path"
                    )
                );
        }

        if (
            record.contains(
                "carrying_item"
            )
        )
        {
            registry.emplace<
                CarryingItem
            >(entity).item =
                entityReferenceFromJson(
                    record.at(
                        "carrying_item"
                    )
                );
        }

        // ==========================================
        // Designation
        // ==========================================

        if (
            record.contains(
                "designation"
            )
        )
        {
            const auto& value =
                record.at(
                    "designation"
                );

            registry.emplace<
                Designation
            >(entity).type =
                static_cast<
                    DesignationType
                >(
                    value.at(
                        "type"
                    ).get<int>()
                );

            registry.emplace<
                DesignationLifecycle
            >(entity).state =
                static_cast<
                    DesignationState
                >(
                    value.at(
                        "state"
                    ).get<int>()
                );
        }

        // ==========================================
        // Item
        // ==========================================

        if (
            record.contains(
                "item"
            )
        )
        {
            const auto& value =
                record.at(
                    "item"
                );

            auto& item =
                registry.emplace<
                    Item
                >(entity);

            item.type =
                static_cast<
                    ItemType
                >(
                    value.at(
                        "type"
                    ).get<int>()
                );

            item.material =
                static_cast<
                    MaterialType
                >(
                    value.at(
                        "material"
                    ).get<int>()
                );

            item.weight =
                value.at(
                    "weight"
                ).get<
                    std::uint32_t
                >();

            if (
                value.value(
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
                value.contains(
                    "state"
                )
            )
            {
                const auto& stateJson =
                    value.at(
                        "state"
                    );

                auto& state =
                    registry.emplace<
                        ItemState
                    >(entity);

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
        }

        // ==========================================
        // Stockpile
        // ==========================================

        if (
            record.contains(
                "stockpile"
            )
        )
        {
            const auto& value =
                record.at(
                    "stockpile"
                );

            auto& stockpile =
                registry.emplace<
                    Stockpile
                >(entity);

            stockpile.bounds.min =
                positionFromJson(
                    value.at(
                        "bounds"
                    ).at(
                        "min"
                    )
                );

            stockpile.bounds.max =
                positionFromJson(
                    value.at(
                        "bounds"
                    ).at(
                        "max"
                    )
                );

            for (
                const auto& type :
                value.at(
                    "accepts"
                )
            )
            {
                stockpile.accepts.
                    push_back(
                        static_cast<
                            ItemType
                        >(
                            type.get<int>()
                        )
                    );
            }

            for (
                const auto& item :
                value.at(
                    "current_items"
                )
            )
            {
                stockpile.currentItems.
                    push_back(
                        entityReferenceFromJson(
                            item
                        )
                    );
            }

            for (
                const auto& position :
                value.at(
                    "reserved_cells"
                )
            )
            {
                stockpile.reservedCells.
                    push_back(
                        positionFromJson(
                            position
                        )
                    );
            }

            if (
                !value.at(
                    "max_items"
                ).is_null()
            )
            {
                stockpile.maxItems =
                    value.at(
                        "max_items"
                    ).get<
                        std::size_t
                    >();
            }
        }
    }
}

// ==================================================
// Jobs
// ==================================================

json jobsToJson(
    const JobBoard& jobBoard
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

std::vector<Job>
jobsFromJson(
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
        },
        {
            "depth",
            map.depth()
        }
    };

    root["world"]["tiles"] =
        json::array();

    for (
        int z = 0;
        z < map.depth();
        ++z
    )
    {
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
                const Tile& tile =
                    map.at(
                        x,
                        y,
                        z
                    );

                root[
                    "world"
                ][
                    "tiles"
                ].push_back({
                    {
                        "shape",
                        static_cast<int>(
                            tile.shape
                        )
                    },
                    {
                        "material",
                        static_cast<int>(
                            tile.material
                        )
                    },
                    {
                        "feature",
                        static_cast<int>(
                            tile.feature
                        )
                    },
                    {
                        "feature_material",
                        static_cast<int>(
                            tile.featureMaterial
                        )
                    },
                    {
                        "liquid_type",
                        static_cast<int>(
                            tile.liquid.type
                        )
                    },
                    {
                        "liquid_depth",
                        static_cast<int>(
                            tile.liquid.depth
                        )
                    }
                });
            }
        }
    }

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

    root["entities"] =
        json::array();

    const auto& registry =
        simulation.registry();

    for (
        const auto entity :
        collectEntities(
            registry
        )
    )
    {
        root[
            "entities"
        ].push_back(
            serializeEntity(
                registry,
                entity
            )
        );
    }

    root["jobs"] =
        jobsToJson(
            simulation.jobBoard()
        );

    if (
        path.has_parent_path()
    )
    {
        std::filesystem::
            create_directories(
                path.parent_path()
            );
    }

    auto temporary =
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
                "Could not open temporary save file."
            );
        }

        output
            << root.dump(2);

        output.flush();

        if (!output)
        {
            throw std::runtime_error(
                "Failed while writing save file."
            );
        }
    }

    std::error_code error;

    std::filesystem::rename(
        temporary,
        path,
        error
    );

    if (error)
    {
        throw std::runtime_error(
            "Could not replace save file: "
            +
            error.message()
        );
    }
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
            "Save version "
            +
            std::to_string(version)
            +
            " is incompatible. "
            "This build requires version "
            +
            std::to_string(
                CurrentVersion
            )
            +
            "."
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

    const int depth =
        world.at(
            "depth"
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
            depth,
            seed
        );

    const auto& tiles =
        world.at(
            "tiles"
        );

    const std::size_t expected =
        static_cast<std::size_t>(
            width
        )
        *
        static_cast<std::size_t>(
            height
        )
        *
        static_cast<std::size_t>(
            depth
        );

    if (
        tiles.size() !=
        expected
    )
    {
        throw std::runtime_error(
            "Save contains an invalid tile count."
        );
    }

    std::size_t tileIndex =
        0;

    for (
        int z = 0;
        z < depth;
        ++z
    )
    {
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
                const auto& value =
                    tiles[
                        tileIndex++
                    ];

                Tile& tile =
                    simulation->
                        map().
                        at(
                            x,
                            y,
                            z
                        );

                tile.shape =
                    static_cast<
                        TileShape
                    >(
                        value.at(
                            "shape"
                        ).get<int>()
                    );

                tile.material =
                    static_cast<
                        MaterialType
                    >(
                        value.at(
                            "material"
                        ).get<int>()
                    );

                tile.feature =
                    static_cast<
                        TileFeature
                    >(
                        value.at(
                            "feature"
                        ).get<int>()
                    );

                tile.featureMaterial =
                    static_cast<
                        MaterialType
                    >(
                        value.at(
                            "feature_material"
                        ).get<int>()
                    );

                tile.liquid.type =
                    static_cast<
                        LiquidType
                    >(
                        value.at(
                            "liquid_type"
                        ).get<int>()
                    );

                tile.liquid.depth =
                    static_cast<
                        std::uint8_t
                    >(
                        value.at(
                            "liquid_depth"
                        ).get<int>()
                    );
            }
        }
    }

    auto& registry =
        simulation->
            registry();

    const auto& entities =
        root.at(
            "entities"
        );

    createEntityIds(
        registry,
        entities
    );

    loadEntityComponents(
        registry,
        entities
    );

    simulation->
        jobBoard().
        restore(
            jobsFromJson(
                root.at(
                    "jobs"
                )
            )
        );

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

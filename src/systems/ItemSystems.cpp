#include "ascii/systems/ItemSystems.hpp"

#include "ascii/Components.hpp"
#include "ascii/Items.hpp"
#include "ascii/Material.hpp"
#include "ascii/Position.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/systems/StockpileSystems.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <algorithm>
#include <utility>

namespace ascii::systems
{

void processItemSpawns(
    entt::registry& registry,
    EventQueue<ItemSpawnEvent>& events
)
{
    const auto queued =
        events.take();

    for (
        const auto& event :
        queued
    )
    {
        const auto entity =
            registry.create();

        auto& item =
            registry.emplace<
                Item
            >(entity);

        item.type =
            event.itemType;

        item.material =
            event.material;

        item.weight =
            1;

        registry.emplace<
            Carriable
        >(entity);

        auto& state =
            registry.emplace<
                ItemState
            >(entity);

        state.location =
            ItemLocation::OnGround;

        state.carrier =
            entt::null;

        state.stockpile =
            entt::null;

        registry.emplace<
            Position
        >(entity) =
            event.position;

        auto& glyph =
            registry.emplace<
                Glyph
            >(entity);

        switch (event.itemType)
        {
            case ItemType::Stone:
                glyph.character =
                    's';
                break;

            case ItemType::Ore:
                glyph.character =
                    'o';
                break;

            case ItemType::Soil:
                glyph.character =
                    'd';
                break;

            case ItemType::Log:
                glyph.character =
                    'l';
                break;
        }

        glyph.color =
            materialColor(
                event.material
            );
    }
}

void processItemPickups(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    JobBoard& jobBoard,
    EventQueue<ItemPickupEvent>& events
)
{
    const auto queued =
        events.take();

    for (
        const auto& event :
        queued
    )
    {
        if (
            !registry.valid(
                event.item
            ) ||
            !registry.valid(
                event.agent
            ) ||
            !registry.all_of<
                ItemState,
                Position
            >(event.item) ||
            !registry.all_of<
                AssignedJob,
                Position
            >(event.agent)
        )
        {
            continue;
        }

        auto& state =
            registry.get<
                ItemState
            >(event.item);

        if (
            state.location !=
            ItemLocation::OnGround
        )
        {
            continue;
        }

        const Position itemPosition =
            registry.get<
                Position
            >(event.item);

        const Position agentPosition =
            registry.get<
                Position
            >(event.agent);

        if (
            itemPosition !=
            agentPosition
        )
        {
            continue;
        }

        const auto assigned =
            registry.get<
                AssignedJob
            >(event.agent);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (
            job == nullptr ||
            job->type !=
                JobType::Haul ||
            job->item !=
                event.item
        )
        {
            continue;
        }

        state.location =
            ItemLocation::Carried;

        state.carrier =
            event.agent;

        state.stockpile =
            entt::null;

        registry.remove<
            Position
        >(event.item);

        if (
            registry.all_of<
                CarryingItem
            >(event.agent)
        )
        {
            registry.get<
                CarryingItem
            >(event.agent).item =
                event.item;
        }
        else
        {
            registry.emplace<
                CarryingItem
            >(event.agent).item =
                event.item;
        }

        job->haulStage =
            HaulStage::ToStockpile;

        auto path =
            pathfinder.findPath(
                map,
                agentPosition,
                job->destination
            );

        if (path)
        {
            setMovementPath(
                registry,
                event.agent,
                std::move(*path)
            );
        }
        else if (
            registry.all_of<
                MovementPath
            >(event.agent)
        )
        {
            registry.remove<
                MovementPath
            >(event.agent);
        }
    }
}

void processItemDrops(
    entt::registry& registry,
    JobBoard& jobBoard,
    EventQueue<ItemDropEvent>& events
)
{
    const auto queued =
        events.take();

    for (
        const auto& event :
        queued
    )
    {
        if (
            !registry.valid(
                event.item
            ) ||
            !registry.valid(
                event.agent
            ) ||
            !registry.valid(
                event.stockpile
            ) ||
            !registry.all_of<
                ItemState
            >(event.item) ||
            !registry.all_of<
                Stockpile
            >(event.stockpile) ||
            !registry.all_of<
                AssignedJob
            >(event.agent)
        )
        {
            continue;
        }

        auto& state =
            registry.get<
                ItemState
            >(event.item);

        if (
            state.location !=
                ItemLocation::Carried ||
            state.carrier !=
                event.agent
        )
        {
            continue;
        }

        auto& stockpile =
            registry.get<
                Stockpile
            >(event.stockpile);

        if (
            !stockpile.bounds.contains(
                event.destination
            )
        )
        {
            continue;
        }

        const auto assigned =
            registry.get<
                AssignedJob
            >(event.agent);

        Job* job =
            jobBoard.find(
                assigned.id
            );

        if (
            job == nullptr ||
            job->type !=
                JobType::Haul ||
            job->item !=
                event.item
        )
        {
            continue;
        }

        if (
            registry.all_of<
                Position
            >(event.item)
        )
        {
            registry.get<
                Position
            >(event.item) =
                event.destination;
        }
        else
        {
            registry.emplace<
                Position
            >(event.item) =
                event.destination;
        }

        state.location =
            ItemLocation::Stockpiled;

        state.carrier =
            entt::null;

        state.stockpile =
            event.stockpile;

        if (
            std::find(
                stockpile.currentItems.begin(),
                stockpile.currentItems.end(),
                event.item
            )
            ==
            stockpile.currentItems.end()
        )
        {
            stockpile.currentItems.
                push_back(
                    event.item
                );
        }

        releaseStockpileCell(
            registry,
            event.stockpile,
            event.destination
        );

        job->state =
            JobState::Complete;

        job->worker =
            entt::null;

        if (
            registry.all_of<
                CarryingItem
            >(event.agent)
        )
        {
            registry.remove<
                CarryingItem
            >(event.agent);
        }

        clearWorkerJob(
            registry,
            event.agent
        );
    }
}

}

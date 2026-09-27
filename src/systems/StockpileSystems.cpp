#include "ascii/systems/StockpileSystems.hpp"

#include "ascii/Position.hpp"
#include "ascii/Stockpiles.hpp"
#include "ascii/systems/SystemUtils.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace ascii::systems
{

namespace
{

bool cellReserved(
    const Stockpile& stockpile,
    Position position
)
{
    return
        std::find(
            stockpile.reservedCells.begin(),
            stockpile.reservedCells.end(),
            position
        )
        !=
        stockpile.reservedCells.end();
}

bool cellOccupied(
    const entt::registry& registry,
    const Stockpile& stockpile,
    Position position
)
{
    for (
        auto item :
        stockpile.currentItems
    )
    {
        if (
            !registry.valid(item) ||
            !registry.all_of<
                Position
            >(item)
        )
        {
            continue;
        }

        if (
            registry.get<
                Position
            >(item)
            ==
            position
        )
        {
            return true;
        }
    }

    return false;
}

}

std::optional<StockpileDestination>
findStockpileDestination(
    entt::registry& registry,
    const GameMap& map,
    const Pathfinder& pathfinder,
    ItemType itemType,
    Position itemPosition
)
{
    auto view =
        registry.view<
            Stockpile
        >();

    std::vector<entt::entity>
        stockpiles;

    for (auto entity : view)
    {
        stockpiles.push_back(
            entity
        );
    }

    sortEntities(stockpiles);

    std::optional<
        StockpileDestination
    > best;

    for (
        auto stockpileEntity :
        stockpiles
    )
    {
        auto& stockpile =
            registry.get<
                Stockpile
            >(stockpileEntity);

        if (
            !stockpile.acceptsItem(
                itemType
            ) ||
            stockpile.full()
        )
        {
            continue;
        }

        for (
            int z =
                stockpile.bounds.min.z;
            z <=
                stockpile.bounds.max.z;
            ++z
        )
        {
            for (
                int y =
                    stockpile.bounds.min.y;
                y <=
                    stockpile.bounds.max.y;
                ++y
            )
            {
                for (
                    int x =
                        stockpile.bounds.min.x;
                    x <=
                        stockpile.bounds.max.x;
                    ++x
                )
                {
                    const Position candidate{
                        x,
                        y,
                        z
                    };

                    if (
                        !map.inBounds(candidate) ||
                        !map.at(candidate).
                            walkable()
                    )
                    {
                        continue;
                    }

                    if (
                        cellReserved(
                            stockpile,
                            candidate
                        ) ||
                        cellOccupied(
                            registry,
                            stockpile,
                            candidate
                        )
                    )
                    {
                        continue;
                    }

                    auto path =
                        pathfinder.findPath(
                            map,
                            itemPosition,
                            candidate
                        );

                    if (!path)
                    {
                        continue;
                    }

                    if (
                        !best ||
                        path->size() <
                            best->pathLength
                    )
                    {
                        best =
                            StockpileDestination{
                                stockpileEntity,
                                candidate,
                                path->size()
                            };
                    }
                }
            }
        }
    }

    return best;
}

void reserveStockpileCell(
    entt::registry& registry,
    entt::entity stockpileEntity,
    Position position
)
{
    if (
        stockpileEntity ==
            entt::null ||
        !registry.valid(
            stockpileEntity
        ) ||
        !registry.all_of<
            Stockpile
        >(stockpileEntity)
    )
    {
        return;
    }

    auto& stockpile =
        registry.get<
            Stockpile
        >(stockpileEntity);

    if (
        !cellReserved(
            stockpile,
            position
        )
    )
    {
        stockpile.reservedCells.
            push_back(
                position
            );
    }
}

void releaseStockpileCell(
    entt::registry& registry,
    entt::entity stockpileEntity,
    Position position
)
{
    if (
        stockpileEntity ==
            entt::null ||
        !registry.valid(
            stockpileEntity
        ) ||
        !registry.all_of<
            Stockpile
        >(stockpileEntity)
    )
    {
        return;
    }

    auto& stockpile =
        registry.get<
            Stockpile
        >(stockpileEntity);

    const auto iterator =
        std::find(
            stockpile.reservedCells.begin(),
            stockpile.reservedCells.end(),
            position
        );

    if (
        iterator !=
        stockpile.reservedCells.end()
    )
    {
        stockpile.reservedCells.erase(
            iterator
        );
    }
}

}

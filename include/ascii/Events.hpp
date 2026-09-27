#pragma once

#include "Items.hpp"
#include "Material.hpp"
#include "Position.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace ascii
{

enum class ItemSource
{
    Mining,
    Digging,
    Felling
};

struct ItemSpawnEvent
{
    ItemType itemType{
        ItemType::Stone
    };

    MaterialType material{
        MaterialType::Granite
    };

    Position position{};

    ItemSource source{
        ItemSource::Mining
    };
};

struct ItemPickupEvent
{
    entt::entity item{
        entt::null
    };

    entt::entity agent{
        entt::null
    };
};

struct ItemDropEvent
{
    entt::entity item{
        entt::null
    };

    entt::entity agent{
        entt::null
    };

    entt::entity stockpile{
        entt::null
    };

    Position destination{};
};

template<typename Event>
class EventQueue
{
public:
    void emit(
        Event event
    )
    {
        events_.push_back(
            std::move(event)
        );
    }

    [[nodiscard]]
    bool empty() const
    {
        return events_.empty();
    }

    [[nodiscard]]
    std::size_t size() const
    {
        return events_.size();
    }

    std::vector<Event> take()
    {
        auto result =
            std::move(events_);

        events_.clear();

        return result;
    }

private:
    std::vector<Event>
        events_;
};

}

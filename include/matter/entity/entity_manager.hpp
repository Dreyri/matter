#ifndef MATTER_ENTITY_ENTITY_MANAGER_HPP
#define MATTER_ENTITY_ENTITY_MANAGER_HPP

#pragma once

#include <cassert>
#include <queue>
#include <vector>

#include "matter/entity/entity_traits.hpp"

namespace matter
{
template<typename Entity>
struct entity_manager
{
    static_assert(matter::is_entity_v<Entity>, "Entity is not a valid entity");

    using entity_type  = Entity;
    using id_type      = typename entity_type::id_type;
    using version_type = typename entity_type::version_type;

private:
    std::vector<version_type> m_entities;
    std::queue<id_type>       m_free_entities;

public:
    entity_manager()  = default;
    ~entity_manager() = default;

    entity_type create()
    {
        id_type      id;
        version_type ver;
        if (!std::empty(m_free_entities))
        {
            id = std::move(m_free_entities.front());
            m_free_entities.pop();
            ver = current_version(id);
        }
        else
        {
            // initialize with version 0
            ver = 0;
            m_entities.push_back(ver);
            id = static_cast<id_type>(std::size(m_entities));
        }

        return entity_type{id, ver};
    }

    void destroy(const entity_type& ent)
    {
        // assert a valid context
        assert(ent.id() <= std::size(m_entities));
        // check we're trying to destroy a current entity
        assert(ent.version() == current_version(ent.id()));

        ++current_version(ent.id());
        m_free_entities.push(ent.id());
    }

    bool is_valid(const entity_type& ent) const
    {
        return current_version(ent.id()) == ent.version();
    }

    version_type& current_version(const id_type& id)
    {
        // id 0 is invalid so ids start at 1, to fill all space id 1 occupies
        // slot 0
        return m_entities[id - 1];
    }

    const version_type& current_version(const id_type& id) const
    {
        return m_entities[id - 1];
    }

    size_t size() const noexcept
    {
        return std::size(m_entities) - std::size(m_free_entities);
    }

    size_t capacity() const noexcept
    {
        return m_entities.capacity();
    }
};
} // namespace matter

#endif

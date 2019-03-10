#ifndef MATTER_COMPONENT_COMPONENT_IDENTIFIER_HPP
#define MATTER_COMPONENT_COMPONENT_IDENTIFIER_HPP

#pragma once

#include <algorithm>
#include <unordered_map>

#include "matter/component/identifier.hpp"
#include "matter/component/metadata.hpp"
#include "matter/component/traits.hpp"

namespace matter
{

/// \brief identifies components by an id
/// Each component must be assigned an id, there is the struct identifier which
/// assists in generating a `size_t` id for the component. We cannot use this
/// generated id directly because these generated ids are global, with them
/// being global if we use them directly there might be big gaps in ids in case
/// you use multiple instances of `component_identifier`. To avoid this we map
/// the global ids to a local id. This way we get a clean array of ids.
template<typename... Components>
class component_identifier {
    static_assert((matter::is_component_v<Components> && ...),
                  "All types must be valid components");

    struct component_identifier_tag
    {};

public:
    using id_type = std::size_t;

    static constexpr auto constexpr_components_size = sizeof...(Components);

    using identifier_type = identifier<component_identifier_tag>;

private:
    /// holds all the runtime ids, the key is the id generated by the
    /// identifier and the value is the id used by the local
    /// component_identifier
    std::unordered_map<std::size_t, std::size_t> runtime_ids_;
    std::size_t next_local_id_{constexpr_components_size};

    std::array<matter::component_metadata, constexpr_components_size>
                                            static_metadata_;
    std::vector<matter::component_metadata> runtime_metadata_;

public:
    constexpr component_identifier()
        : static_metadata_{
              matter::component_metadata{std::in_place_type_t<Components>{}}...}
    {}

    template<typename Component>
    constexpr bool is_constexpr() const noexcept
    {
        return detail::type_in_list_v<Component, Components...>;
    }

    /// \brief instructs the identifier to now identify this component
    template<typename Component>
    std::size_t register_type() noexcept
    {
        static_assert(matter::is_component_v<Component>,
                      "You may only register valid components");
        auto id = identifier_type::template get<Component>();
        assert(runtime_ids_.count(id) ==
               0); // if not 0 then we already identify this type
        auto local_id = next_local_id_++;
        runtime_ids_.emplace(id, local_id);
        // store all available metadata for id -> data relation
        runtime_metadata_.emplace_back(std::in_place_type_t<Component>{});
        return local_id;
    }

    const matter::component_metadata& metadata(id_type id) noexcept
    {
        assert((constexpr_components_size + runtime_metadata_.size()) > id);

        if (id >= constexpr_components_size)
        {
            auto runtime_id = id - constexpr_components_size;
            return runtime_metadata_[runtime_id];
        }
        else
        {
            return static_metadata_[id];
        }
    }

    template<typename Component>
    constexpr bool is_registered() const noexcept
    {
        if (is_constexpr<Component>())
        {
            return true;
        }

        auto id = identifier_type::template get<Component>();
        return runtime_ids_.count(id) == 1;
    }

    /// \brief gets the id of the constexpr component
    /// If the component was registered at compile time this will yield the id,
    template<typename Component>
    constexpr id_type id_constexpr() const noexcept
    {
        static_assert(
            detail::type_index<Component, Components...>().has_value(),
            "The Component wasn't defined at compile time");
        return detail::type_index<Component, Components...>().value();
    }

    /// \brief get the id for a runtime component
    /// Retrieves the id for a runtime component, components have to be
    /// registered with register<Component>() first, this function cannot do so
    /// automatically as it's const qualified.
    template<typename Component>
    id_type id_runtime() const noexcept
    {
        // is_constexpr is for some reason not constexpr, who knew
        static_assert(!detail::type_in_list_v<Component, Components...>,
                      "This is a compile time component, use id<Component>() "
                      "or id_constexpr<Component>().");
        auto id = identifier_type::template get<Component>();

        auto it = runtime_ids_.find(id);

        assert(it != runtime_ids_.end()); // must be registered otherwise die
        return it->second;
    }

    /// \brief retrieve the local id for a component
    template<typename Component>
    constexpr id_type id() const noexcept
    {
        // cannot use is_constexpr because of some weird error, but this works
        // anyway
        if constexpr (detail::type_index<Component, Components...>()
                          .has_value())
        {
            return id_constexpr<Component>();
        }
        else
        {
            return id_runtime<Component>();
        }
    }

    template<typename... Cs>
    constexpr std::array<id_type, sizeof...(Cs)> ids()
    {
        return std::array<id_type, sizeof...(Cs)>{id<Cs>()...};
    }

    template<typename... Cs>
    constexpr std::array<id_type, sizeof...(Cs)> sorted_ids()
    {
        auto sorted_ids = ids<Cs...>();

        std::sort(sorted_ids.begin(), sorted_ids.end());
        return sorted_ids;
    }
};
} // namespace matter

#endif

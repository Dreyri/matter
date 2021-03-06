#ifndef MATTER_COMPONENT_COMPONENT_IDENTIFIER_HPP
#define MATTER_COMPONENT_COMPONENT_IDENTIFIER_HPP

#pragma once

#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "matter/component/traits.hpp"
#include "matter/id/id.hpp"
#include "matter/id/identifier.hpp"
#include "matter/id/typed_id.hpp"

namespace matter
{
struct unregistered_component : std::logic_error
{
    template<typename Component>
    unregistered_component(std::in_place_type_t<Component>)
        : std::logic_error{[]() {
              std::stringstream ss;
              ss << "Component \"";
              if constexpr (matter::is_component_named_v<Component>)
              {
                  ss << matter::component_name_v<Component>;
              }
              else
              {
                  ss << "unknown";
              }
              ss << "\" was not registered";
              return std::move(ss).str();
          }()}
    {}
};

/// \brief identifies components by an id
/// Each component must be assigned an id, there is the struct identifier which
/// assists in generating a `size_t` id for the component. We cannot use this
/// generated id directly because these generated ids are global, with them
/// being global if we use them directly there might be big gaps in ids in case
/// you use multiple instances of `component_identifier`. To avoid this we map
/// the global ids to a local id. This way we get a clean array of ids.
template<typename Id, typename... Components>
class default_component_identifier {
    static_assert(matter::is_id_v<Id>, "Id must fulfil the is_id concept");
    static_assert((matter::is_component_v<Components> && ...),
                  "All types must be valid components");

    struct component_identifier_tag
    {};

public:
    using id_type       = Id;
    using id_value_type = typename Id::value_type;

    static constexpr id_value_type constexpr_components_size =
        sizeof...(Components);

    using identifier_type = identifier<id_value_type, component_identifier_tag>;

private:
    /// holds all the runtime ids, the index is the id generated by the
    /// identifier and the value is the id used by the local
    /// component_identifier
    // std::unordered_map<std::size_t, std::size_t> runtime_ids_;
    std::vector<id_type> runtime_ids_;
    id_value_type        next_local_id_{constexpr_components_size};

public:
    constexpr default_component_identifier() = default;

    template<typename Component>
    static constexpr bool is_static() noexcept
    {
        return detail::type_in_list_v<Component, Components...>;
    }

    /// \brief instructs the identifier to now identify this component
    template<typename Component>
    matter::typed_id<id_type, Component> register_component() noexcept
    {
        assert(!is_static<Component>());
        auto global_id = identifier_type::template get<Component>();
        assert(runtime_ids_.size() <= static_cast<std::size_t>(global_id) ||
               !bool(runtime_ids_[global_id]));
        auto local_id = next_local_id_++;

        if (runtime_ids_.size() <= static_cast<std::size_t>(global_id))
        {
            // initialize to invalid state by default
            runtime_ids_.resize(global_id + 1, {});
        }

        auto id = runtime_ids_[global_id] =
            id_type{static_cast<id_value_type>(local_id)};
        return matter::typed_id<id_type, Component>{id};
    }

    template<typename Component>
    constexpr bool contains_component() const noexcept
    {
        if constexpr (is_static<Component>())
        {
            return true;
        }

        auto id = identifier_type::template get<Component>();
        return static_cast<decltype(id)>(runtime_ids_.size()) > id &&
               bool(runtime_ids_[id]);
    }

    /// \brief retrieve the local id for a component
    template<typename Component>
    constexpr matter::typed_id<id_type, Component> component_id() const
        noexcept(is_static<Component>())
    {
        if constexpr (is_static<Component>())
        {
            return matter::typed_id<id_type, Component>{static_id<Component>()};
        }
        else
        {
            return matter::typed_id<id_type, Component>{
                runtime_id<Component>()};
        }
    }

    template<typename... Ts>
    constexpr matter::unordered_typed_ids<id_type, Ts...> component_ids() const
        noexcept((is_static<Ts>() && ...))
    {
        return matter::unordered_typed_ids{component_id<Ts>()...};
    }

    template<typename... Ts>
    constexpr matter::ordered_typed_ids<id_type, Ts...>
    ordered_component_ids() const noexcept((is_static<Ts>() && ...))
    {
        return matter::ordered_typed_ids{component_ids<Ts...>()};
    }

private:
    template<typename Component>
    constexpr auto static_id() const noexcept
    {
        static_assert(
            is_static<Component>(),
            "This component id should be retrieved using runtime_id() instead");
        constexpr auto res =
            detail::type_index<Component, Components...>().value();
        return id_type{static_cast<id_value_type>(res)};
    }

    template<typename Component>
    constexpr auto runtime_id() const
    {
        static_assert(
            !is_static<Component>(),
            "This component id should be retrieve using static_id() instead");
        auto id = identifier_type::template get<Component>();

        auto local_id = runtime_ids_[id];

        if (!bool(local_id))
        {
            throw matter::unregistered_component{
                std::in_place_type_t<Component>{}};
        }
        return local_id;
    }
};
} // namespace matter

#endif

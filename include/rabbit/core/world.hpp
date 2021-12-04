#pragma once 

#include "entity.hpp"
#include "reactive.hpp"
#include "../math/mat4.hpp"
#include "../components/transform.hpp"

#include <string_view>

namespace rb {
    class world_impl {
    public:
        world_impl();

        entity find_by_name(const std::string_view& name) const;

        registry& registry();

        sink<void(rb::registry&, entity, global_transform&)> on_global_transform_update();

    private:
        void _on_transform_construct(rb::registry& registry, entity entity);

        void _on_local_transform_update(rb::registry& registry, entity entity);

        void _on_global_transform_update(rb::registry& registry, entity entity);

    private:
        rb::registry _registry;
        sigh<void(rb::registry&, entity, global_transform&)> _global_transform_update;
    };

    class world {
    public:
        static void setup();

        static void release();

        static entity find_by_name(const std::string_view& name);

        static registry& registry();

        static sink<void(rb::registry&, entity, global_transform&)> on_global_transform_update();

    private:
        static std::shared_ptr<world_impl> _impl;
    };
}

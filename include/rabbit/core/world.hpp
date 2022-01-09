#pragma once 

#include "entity.hpp"
#include "../math/mat4.hpp"
#include "../components/transform.hpp"

namespace rb {
    class world {
    public:
        static void init();

        static void release();

        static entity find_by_name(const std::string& name);

        static const mat4f& get_world(entity entity, transform& transform);

        static registry& registry();

    private:
        static void _on_transform_construct(rb::registry& registry, entity entity);

        static void _on_transform_update(rb::registry& registry, entity entity);

    private:
        static rb::registry _registry;
    };
}

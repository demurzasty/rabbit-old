#pragma once 

#include "../core/system.hpp"
#include "../components/transform.hpp"

namespace rb {
    class renderer : public system {
    public:
        renderer();

        virtual void process() override;

    private:
        void _on_geometry_construct(registry& registry, entity entity);

        void _on_geometry_destroy(registry& registry, entity entity);

        void _on_geometry_update(registry& registry, entity entity);

        void _on_global_transform_update(registry& registry, entity entity, global_transform& transform);
    };
}

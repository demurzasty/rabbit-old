#pragma once 

#include <rabbit/engine/core/system.hpp>
#include <rabbit/runtime/components/transform.hpp>

namespace rb {
    class hierarchy : public rb::system {
    public:
        void initialize(registry& registry);

    private:
        void _on_transform_construct(registry& registry, entity entity);

        void _on_transform_update(registry& registry, entity entity);
    };
}
#include <rabbit/systems/hierarchy.hpp>
#include <rabbit/components/transform.hpp>

using namespace rb;

void hierarchy::initialize(registry& registry) {
    registry.on_construct<transform>().connect<&hierarchy::_on_transform_construct>(this);
    registry.on_update<transform>().connect<&hierarchy::_on_transform_update>(this);
}

void hierarchy::_on_transform_construct(registry& registry, entity entity) {
    registry.emplace_or_replace<cached_transform>(entity);
}

void hierarchy::_on_transform_update(registry& registry, entity entity) {
    auto& cached_transform = registry.get<rb::cached_transform>(entity);
    if (cached_transform.dirty) {
        // TODO: Not pretty sure.
        return;
    }

    cached_transform.dirty = true;
    for (const auto& [child, child_transform] : registry.view<rb::transform>().each()) {
        if (child_transform.parent == entity) {
            _on_transform_update(registry, child);
        }
    }
}

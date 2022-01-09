#include <rabbit/core/world.hpp>
#include <rabbit/components/identity.hpp>

using namespace rb;

registry world::_registry;

void world::init() {
    _registry.on_construct<transform>().connect<&world::_on_transform_construct>();
    _registry.on_update<transform>().connect<&world::_on_transform_update>();
}

void world::release() {
    _registry.clear();
}

entity world::find_by_name(const std::string& name) {
    for (auto entity : _registry.view<identity>()) {
        if (_registry.get<identity>(entity).name == name) {
            return entity;
        }
    }

    return null;
}

const mat4f& world::get_world(entity entity, transform& transform) {
    auto& cached_transform = _registry.get_or_emplace<rb::cached_transform>(entity);
    if (cached_transform.dirty) {
        if (_registry.valid(transform.parent) && _registry.all_of<rb::transform, rb::cached_transform>(transform.parent)) {
            auto& parent_transform = _registry.get<rb::transform>(transform.parent);

            cached_transform.world = get_world(transform.parent, parent_transform) *
                mat4f::translation(transform.position) *
                mat4f::rotation(transform.rotation) *
                mat4f::scaling(transform.scaling);
        } else {
            cached_transform.world = mat4f::translation(transform.position) *
                mat4f::rotation(transform.rotation) *
                mat4f::scaling(transform.scaling);
        }

        cached_transform.dirty = false;
    }

    return cached_transform.world;
}

registry& world::registry() {
    return _registry;
}

void world::_on_transform_construct(rb::registry& registry, entity entity) {
    registry.emplace_or_replace<cached_transform>(entity);
}

void world::_on_transform_update(rb::registry& registry, entity entity) {
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

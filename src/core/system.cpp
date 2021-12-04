#include <rabbit/core/system.hpp>
#include <rabbit/components/identity.hpp>
#include <rabbit/components/transform.hpp>

using namespace rb;

void system::initialize(registry& registry) {
}

void system::update(registry& registry, float elapsed_time) {
}

void system::draw(registry& registry) {
}

entity system::find_by_name(registry& registry, const std::string& name) {
    for (auto entity : registry.view<identity>()) {
        if (registry.get<identity>(entity).name == name) {
            return entity;
        }
    }

    return null;
}

const mat4f& system::get_world(registry& registry, entity entity, transform& transform) {
    auto& cached_transform = registry.get_or_emplace<rb::cached_transform>(entity);
    if (cached_transform.dirty) {
        if (registry.valid(transform.parent) && registry.all_of<rb::transform, rb::cached_transform>(transform.parent)) {
            auto& parent_transform = registry.get<rb::transform>(transform.parent);

            cached_transform.world = get_world(registry, transform.parent, parent_transform) *
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
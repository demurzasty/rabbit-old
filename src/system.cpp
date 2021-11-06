#include <rabbit/system.hpp>
#include <rabbit/identity.hpp>
#include <rabbit/transform.hpp>

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

mat4f system::calculate_world(rb::registry& registry, entity entity) {
    if (registry.valid(entity) && registry.all_of<transform>(entity)) {
        const auto& transform = registry.get<rb::transform>(entity);
        const auto world = mat4f::translation(transform.position) *
            mat4f::rotation(transform.rotation) *
            mat4f::scaling(transform.scaling);

        return calculate_world(registry, transform.parent) * world;
    } else {
        return mat4f::identity();
    }
}

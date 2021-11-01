#include <rabbit/system.hpp>
#include <rabbit/identity.hpp>

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

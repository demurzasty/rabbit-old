#include <rabbit/systems/renderer.hpp>
#include <rabbit/graphics/graphics.hpp>
#include <rabbit/components/light.hpp>
#include <rabbit/components/geometry.hpp>
#include <rabbit/components/camera.hpp>
#include <rabbit/platform/input.hpp>
#include <rabbit/core/settings.hpp>
#include <rabbit/core/format.hpp>
#include <rabbit/math/math.hpp>

using namespace rb;

void renderer::initialize() {
}

void renderer::update(float elapsed_time) {
}

void renderer::draw() {
    const auto camera_entity = _find_camera();
    if (!world::registry().valid(camera_entity)) {
        return;
    }

    auto& camera_transform = world::registry().get<transform>(camera_entity);
    auto& camera_camera = world::registry().get<camera>(camera_entity);
    auto& camera_world = world::get_world(camera_entity, camera_transform);
    graphics::set_camera(mat4f::perspective(deg2rad(camera_camera.field_of_view), 1280.0 / 720.0f, 0.5f, 100.0f), invert(camera_world), camera_world);

    for (auto&& [entity, transform, geometry] : world::registry().view<transform, geometry>().each()) {
        graphics::render(world::get_world(entity, transform), geometry.mesh, geometry.material);
    }
}

entity renderer::_find_camera() {
    for (auto&& [entity, transform, camera] : world::registry().view<transform, camera>().each()) {
        return entity;
    }

    return null;
}

#include <rabbit/render/renderer.hpp>
#include <rabbit/core/world.hpp>
#include <rabbit/components/transform.hpp>
#include <rabbit/components/geometry.hpp>
#include <rabbit/graphics/graphics.hpp>

using namespace rb;

renderer::renderer() {
    world::on_global_transform_update().connect<&renderer::_on_global_transform_update>(this);
    world::registry().on_construct<geometry>().connect<&renderer::_on_geometry_construct>(this);
    world::registry().on_destroy<geometry>().connect<&renderer::_on_geometry_destroy>(this);
    world::registry().on_update<geometry>().connect<&renderer::_on_geometry_update>(this);
}

void renderer::process() {

}

void renderer::_on_geometry_construct(registry& registry, entity entity) {
    registry.get<geometry>(entity).instance = graphics::make_instance();
}

void renderer::_on_geometry_destroy(registry& registry, entity entity) {
    graphics::destroy_instance(registry.get<geometry>(entity).instance);
}

void renderer::_on_geometry_update(registry& registry, entity entity) {
    auto& geometry = registry.get<rb::geometry>(entity);
    graphics::set_instance_mesh(geometry.instance, geometry.mesh);
    graphics::set_instance_material(geometry.instance, geometry.material);
}

void renderer::_on_global_transform_update(registry& registry, entity entity, global_transform& transform) {
    graphics::set_instance_world_matrix(registry.get<geometry>(entity).instance, transform.world_matrix());
}

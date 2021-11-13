#include <rabbit/renderer.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/light.hpp>
#include <rabbit/input.hpp>
#include <rabbit/settings.hpp>
#include <rabbit/format.hpp>

// TODO: Calculate objects world matrices and store it in buffer.
//       Do not recalculate these matrices every time we want to draw geometry.

using namespace rb;

void renderer::initialize(registry& registry) {
    _viewport = graphics::make_viewport({ settings::window_size });

    registry.on_construct<transform>().connect<&renderer::_on_transform_constructed>(this);
}

void renderer::update(registry& registry, float elapsed_time) {
    if (!registry.valid(_viewport->camera)) {
        for (auto entity : registry.view<camera>()) {
            _viewport->camera = entity;
            break;
        }
    }
    
    for (const auto& [entity, transform, cache] : registry.view<transform, cache>().each()) {
        if (transform.dirty()) {
            cache.world = _calculate_world(registry, entity);
            transform.mark_dirty(false);
        }
    }
}

void renderer::draw(registry& registry) {
    // With no active camera we can't draw scene properly. 
    if (!registry.valid(_viewport->camera)) {
        return;
    }

    // Be sure that camera entity has mandatory components attached.
    if (!registry.all_of<transform, camera>(_viewport->camera)) {
        return;
    }

    // Start drawing. It is basically stariting recording commands into primary frame command buffer.
    // Every frame command buffer should be swapped with next one.
    graphics::begin();

    // Set main camera information to graphics backend.
    const auto& [camera_transform, camera] = registry.get<transform, rb::camera>(_viewport->camera);
    graphics::set_camera(camera_transform, camera);

    // Begin depth pre pass. Using this pass we achive few goals:
    // 1. Store depth into depth buffer. We can reuse it later in postprocessing pass.
    // 2. Minimalize overdraw polygons in forward pass. 
    graphics::begin_depth_pass(_viewport);

    // Draw depth for every geometry in scene.
    // TODO: Entities that is not visible from camera perspective should be culled. 
    for (const auto& [entity, transform, geometry, cache] : registry.view<transform, geometry, cache>().each()) {
        if (!geometry.material || !geometry.material->translucent()) {
            graphics::draw_depth(_viewport, cache.world, geometry.mesh, 0);
        }
    }

    // End depth pass. We can now reuse depth buffer.
    graphics::end_depth_pass(_viewport);

    // Before we render scene directly to viewport we need to prepare data for shadow mapping.
    // Shadows working only for one directional light (for now).
    // We need to search for directional light with shadows enabled and try to render scene into shadow maps.
    // TODO: This pass can be parallel with depth pre pass.
    const auto directional_light_with_shadow = _find_directional_light_with_shadows(registry);
    if (registry.valid(directional_light_with_shadow)) {
        // Extract components that we need to draw shadows.
        const auto& [transform, light, directional_light] = registry.get<rb::transform, rb::light, rb::directional_light>(directional_light_with_shadow);

        // Render scene from every cascade perspective.
        // TODO: We can build command buffers in parallel.
        for (auto cascade = 0u; cascade < graphics_limits::max_shadow_cascades; ++cascade) {
            graphics::begin_shadow_pass(transform, light, directional_light, cascade);

            registry.view<rb::transform, geometry, cache>().each([this, cascade, &registry](entity entity, rb::transform& transform, geometry& geometry, cache& cache) {
                graphics::draw_shadow(cache.world, geometry, cascade);
            });

            graphics::end_shadow_pass();
        }
    }

    graphics::begin_light_pass(_viewport);

    for (const auto& [entity, transform, light, point_light] : registry.view<transform, light, point_light>().each()) {
        graphics::add_point_light(_viewport, transform, light, point_light);
    }

    for (const auto& [entity, transform, light, directional_light] : registry.view<transform, light, directional_light>().each()) {
        graphics::add_directional_light(_viewport, transform, light, directional_light, entity == directional_light_with_shadow);
    }

    graphics::end_light_pass(_viewport);

    // Begin primary geometry drawing. It reuses depth buffer from depth pre pass step.
    graphics::begin_forward_pass(_viewport);

    // Draw every geometry. 
    // TODO: Entities that is not visible from camera perspective should be culled. 
    for (const auto& [entity, transform, geometry, cache] : registry.view<transform, geometry, cache>().each()) {
        if (geometry.material && !geometry.material->translucent()) {
            graphics::draw_forward(_viewport, cache.world, geometry.mesh, geometry.material, 0);
        }
    }

    // Draw skybox between. Minimize overdraw using depth testing.
    graphics::draw_skybox(_viewport);

    for (const auto& [entity, transform, geometry, cache] : registry.view<transform, geometry, cache>().each()) {
        if (geometry.material && geometry.material->translucent()) {
            graphics::draw_forward(_viewport, cache.world, geometry.mesh, geometry.material, 0);
        }
    }

    // End primary geometry drawing.
    graphics::end_forward_pass(_viewport);

    // Begin postprocess pass. It copies color buffer from forward pass.
    graphics::begin_postprocess_pass(_viewport);

    if (_viewport->fxaa_enabled) {
        graphics::next_postprocess_pass(_viewport);

        graphics::draw_fxaa(_viewport);
    }

    if (_viewport->sharpen_enabled) {
        graphics::next_postprocess_pass(_viewport);

        graphics::draw_sharpen(_viewport, _viewport->sharpen_factor);
    }

    if (_viewport->motion_blur_enabled) {
        graphics::next_postprocess_pass(_viewport);

        graphics::draw_motion_blur(_viewport);
    }

    graphics::end_postprocess_pass(_viewport);

    graphics::present(_viewport);

    // End drawing. It basically submits primary command buffer to render queue.
    graphics::end();
}

entity renderer::_find_directional_light_with_shadows(registry& registry) const {
    for (const auto& [entity, light, directional_light] : registry.view<light, directional_light>().each()) {
        if (directional_light.shadow_enabled) {
            return entity;
        }
    }

    return null;
}

void renderer::_on_transform_constructed(registry& registry, entity entity) {
    registry.emplace_or_replace<cache>(entity);
}

mat4f renderer::_calculate_world(rb::registry& registry, entity entity) {
    if (registry.valid(entity) && registry.all_of<transform, cache>(entity)) {
        const auto& transform = registry.get<rb::transform>(entity);
        const auto& cache = registry.get<renderer::cache>(entity);

        if (transform.dirty()) {
            const auto world = mat4f::translation(transform.position) *
                mat4f::rotation(transform.rotation) *
                mat4f::scaling(transform.scaling);

            return _calculate_world(registry, transform.parent) * world;
        } else {
            return _calculate_world(registry, transform.parent) * cache.world;
        }
    } else {
        return mat4f::identity();
    }
}
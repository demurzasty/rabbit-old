#include <rabbit/core/world.hpp>
#include <rabbit/components/identity.hpp>
#include <rabbit/components/transform.hpp>

using namespace rb;

world_impl::world_impl() {
    _registry.on_construct<local_transform>().connect<&world_impl::_on_transform_construct>(this);
    _registry.on_construct<global_transform>().connect<&world_impl::_on_transform_construct>(this);

    _registry.on_update<local_transform>().connect<&world_impl::_on_local_transform_update>(this);
}

entity world_impl::find_by_name(const std::string_view& name) const {
    for (const auto& [entity, identity] : _registry.view<rb::identity>().each()) {
        if (identity.name == name) {
            return entity;
        }
    }

    return null;
}

registry& world_impl::registry() {
    return _registry;
}

sink<void(rb::registry&, entity, global_transform&)> world_impl::on_global_transform_update() {
    return { _global_transform_update };
}

void world_impl::_on_transform_construct(rb::registry& registry, entity entity) {
    if (!registry.all_of<global_transform>(entity)) {
        registry.emplace<global_transform>(entity);
    }

    if (!registry.all_of<local_transform>(entity)) {
        registry.emplace<local_transform>(entity);
    }
}

void world_impl::_on_local_transform_update(rb::registry& registry, entity entity) {
    auto& [local_transform, global_transform] = registry.get<rb::local_transform, rb::global_transform>(entity);

    if (local_transform.parent == entity) {
        local_transform.parent = null;
    }

    if (registry.valid(local_transform.parent)) {
        const auto& parent_global_transform = registry.get<rb::global_transform>(local_transform.parent);

        global_transform.position = parent_global_transform.position + rotate(local_transform.position, local_transform.rotation);
        global_transform.rotation = parent_global_transform.rotation + local_transform.rotation;
        global_transform.scale = parent_global_transform.scale * parent_global_transform.scale;
    } else {
        global_transform.position = local_transform.position;
        global_transform.rotation = local_transform.rotation;
        global_transform.scale = local_transform.scale;
    }

    // TODO: Optimize traversal. 
    for (const auto& [child, child_local_transform] : registry.view<rb::local_transform>().each()) {
        if (child_local_transform.parent == entity) {
            _on_local_transform_update(registry, child);
        }
    }

    global_transform.mark_as_dirty();
}

void world_impl::_on_global_transform_update(rb::registry& registry, entity entity) {
    auto& [local_transform, global_transform] = registry.get<rb::local_transform, rb::global_transform>(entity);

    if (registry.valid(local_transform.parent)) {
        const auto& parent_global_transform = registry.get<rb::global_transform>(local_transform.parent);

        // TODO: Fix this.
    } else {
        local_transform.position = global_transform.position;
        local_transform.rotation = global_transform.rotation;
        local_transform.scale = global_transform.scale;
    }

    // TODO: Optimize traversal. 
    for (const auto& [child, child_local_transform] : registry.view<rb::local_transform>().each()) {
        if (child_local_transform.parent == entity) {
            _on_local_transform_update(registry, child);
        }
    }

    global_transform.mark_as_dirty();

    _global_transform_update.publish(registry, entity, global_transform);
}

std::shared_ptr<world_impl> world::_impl;

void world::setup() {
    _impl = std::make_shared<world_impl>();
}

void world::release() {
    _impl.reset();
}

entity world::find_by_name(const std::string_view& name) {
    return _impl->find_by_name(name);
}

registry& world::registry() {
    return _impl->registry();
}

sink<void(rb::registry&, entity, global_transform&)> world::on_global_transform_update() {
    return _impl->on_global_transform_update();
}

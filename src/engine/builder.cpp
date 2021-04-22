#include <rabbit/engine/builder.hpp>
#include <rabbit/platform/window.hpp>
#include <rabbit/engine/application.hpp>
#include <rabbit/asset/asset_manager.hpp>
#include <rabbit/graphics/texture.hpp>
#include <rabbit/loaders/texture_loader.hpp>
#include <rabbit/graphics/mesh.hpp>
#include <rabbit/loaders/mesh_loader.hpp>
#include <rabbit/systems/renderer.hpp>

using namespace rb;

builder rb::make_builder() {
    return builder{}
        .singleton<application_config>()
        .singleton<application_state>()
        .singleton<window>(&window::install)
        .singleton<graphics_device>(&graphics_device::install)
        .singleton<asset_manager>()
        .loader<texture, texture_loader>()
        .loader<mesh, mesh_loader>()
        .system<renderer>();
}

application builder::build() const {
    return *this;
}

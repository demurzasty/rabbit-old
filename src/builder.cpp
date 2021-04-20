#include <rabbit/builder.hpp>
#include <rabbit/window.hpp>
#include <rabbit/application.hpp>
#include <rabbit/asset_manager.hpp>
#include <rabbit/texture.hpp>
#include <rabbit/texture_loader.hpp>
#include <rabbit/mesh.hpp>
#include <rabbit/mesh_loader.hpp>

using namespace rb;

builder rb::make_builder() {
    return builder{}
        .singleton<application_config>()
        .singleton<application_state>()
        .singleton<window>(&window::install)
        .singleton<graphics_device>(&graphics_device::install)
        .singleton<asset_manager>()
        .loader<texture, texture_loader>(".png", ".bmp", ".jpg")
        .loader<mesh, mesh_loader>(".obj");
}

application builder::build() const {
    return *this;
}

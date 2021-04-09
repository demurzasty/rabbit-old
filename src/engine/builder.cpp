#include <rabbit/rabbit.hpp>

using namespace rb;

builder rb::make_default_builder() {
    builder builder;
    builder.service<config>();
    builder.service<window>(&window::resolve);
    builder.service<keyboard>(&keyboard::resolve);
    builder.service<mouse>(&mouse::resolve);
    builder.service<gamepad>(&gamepad::resolve);
    builder.service<graphics_device>(&graphics_device::resolve);
    builder.service<asset_manager>();
    builder.initialize([](asset_manager& asset_manager) {
        asset_manager.add_loader<texture, texture_loader>();
        asset_manager.add_loader<mesh, mesh_loader>();
        asset_manager.add_loader<texture_cube, texture_cube_loader>();
    });
    builder.system<renderer>();
    return builder;
}

game builder::build() {
    return *this;
}
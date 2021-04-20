#include <rabbit/builder.hpp>
#include <rabbit/window.hpp>
#include <rabbit/application.hpp>

using namespace rb;

builder rb::make_builder() {
    return builder{}
        .singleton<application_config>()
        .singleton<application_state>()
        .singleton<window>(&window::install)
        .singleton<graphics_device>(&graphics_device::install);
}

application builder::build() const {
    return *this;
}

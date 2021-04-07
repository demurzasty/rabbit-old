#include <rabbit/renderer.hpp>
#include <rabbit/builtin_shaders.hpp>

using namespace rb;

renderer::renderer(graphics_device& graphics_device)
    : _graphics_device(graphics_device) {
    shader_desc desc;
    desc.vertex_bytecode = builtin_shaders::get(builtin_shader::forward_vert);
    desc.fragment_bytecode = builtin_shaders::get(builtin_shader::forward_frag);
    _forward = graphics_device.make_shader(desc);
}

void renderer::draw(registry& registry, graphics_device& graphics_device) {
    graphics_device.clear(color::cornflower_blue());
}

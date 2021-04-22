#include <rabbit/loaders/texture_loader.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace rb;

texture_loader::texture_loader(graphics_device& graphics_device)
    : _graphics_device(graphics_device) {
}

std::shared_ptr<void> texture_loader::load(const std::string& filename, const json& json) {
    int width, height, components;
    std::unique_ptr<stbi_uc[], decltype(&stbi_image_free)> data{ stbi_load(filename.c_str(), &width, &height, &components, STBI_rgb_alpha), &stbi_image_free };
    RB_ASSERT(data, "Failed to load image");

    texture_desc desc;
    desc.data = data.get();
    desc.size = { static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height) };
    desc.format = texture_format::rgba8;

    return _graphics_device.make_texture(desc);
}

#include <rabbit/texture_loader.hpp>
#include <rabbit/image.hpp>
#include <rabbit/json.hpp>

#include <fstream>
#include <cassert>

using namespace rb;

texture_loader::texture_loader(std::shared_ptr<graphics_device> graphics_device)
    : _graphics_device(graphics_device) {
}

std::shared_ptr<void> texture_loader::load(const std::string& filename, const json& metadata) {
    auto image = image::from_file(filename);

    texture_desc desc;
    desc.data = image.pixels();
    desc.size = image.size();

    if (metadata.contains("filter")) {
        desc.filter = metadata["filter"] == "linear" ? texture_filter::linear : texture_filter::nearest;
    }
    
    return _graphics_device->make_texture(desc);
}

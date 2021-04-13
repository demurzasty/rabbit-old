#include <rabbit/loaders/texture_loader.hpp>
#include <rabbit/graphics/image.hpp>
#include <rabbit/core/json.hpp>

#include <map>
#include <string>
#include <fstream>
#include <cassert>

using namespace rb;

texture_loader::texture_loader(graphics_device& graphics_device)
    : _graphics_device(graphics_device) {
}

std::shared_ptr<void> texture_loader::load(const std::string& filename, const json& metadata) {
    auto image = image::from_file(filename);

    texture_desc desc;
    desc.data = image.pixels();
    desc.size = image.size();

    desc.filter = json_utils::member_or(metadata, "filter", texture_filter::linear);
    desc.wrap = json_utils::member_or(metadata, "wrap", texture_wrap::repeat);
    desc.generate_mipmaps = true;
    
    return _graphics_device.make_texture(desc);
}

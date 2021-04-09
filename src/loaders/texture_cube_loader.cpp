#include <rabbit/loaders/texture_cube_loader.hpp>
#include <rabbit/graphics/image.hpp>
#include <rabbit/core/json.hpp>
#include <rabbit/core/exception.hpp>

#include <map>
#include <string>
#include <fstream>
#include <cassert>

using namespace rb;

namespace {
    std::map<texture_cube_face, std::string> face_names = {
        { texture_cube_face::positive_x, "right" },
        { texture_cube_face::negative_x, "left" },
        { texture_cube_face::positive_y, "top" },
        { texture_cube_face::negative_y, "bottom" },
        { texture_cube_face::positive_z, "front" },
        { texture_cube_face::negative_z, "back" },
    };
}

texture_cube_loader::texture_cube_loader(graphics_device& graphics_device)
    : _graphics_device(graphics_device) {
}

std::shared_ptr<void> texture_cube_loader::load(const std::string& filename, const json& metadata) {
    std::ifstream stream{ filename, std::ios::in };
    if (!stream.is_open()) {
        throw make_exception("Cannot open file: {}", filename);
    }

    json json;
    stream >> json;

    texture_cube_desc desc;
    
    std::map<texture_cube_face, std::unique_ptr<image>> images;
    for (const auto& [face, name] : face_names) {
        images[face] = std::make_unique<image>(image::from_file(json[name]));
    }

    for (const auto& [face, image] : images) {
        desc.data[face] = image->pixels();
        desc.size = image->size();
    }

    desc.filter = json_utils::member_or(metadata, "filter", texture_filter::nearest);
    desc.wrap = json_utils::member_or(metadata, "wrap", texture_wrap::clamp);

    return _graphics_device.make_texture(desc);
}

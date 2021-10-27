#include <rabbit/material.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/assets.hpp>
#include <rabbit/config.hpp>

#include <array>
#include <fstream>

using namespace rb;

std::shared_ptr<material> material::load(const std::string& filename, json& metadata) {
    std::ifstream stream{ filename, std::ios::in };
    RB_ASSERT(stream.is_open(), "Cannot open file");

    json json;
    stream >> json;
    stream.close();
    material_desc desc;

    if (json.contains("base_color")) {
        auto& base_color = json["base_color"];
        desc.base_color = { base_color[0], base_color[1], base_color[2] };
    }

    if (json.contains("roughness")) {
        desc.roughness = json["roughness"];
    }

    if (json.contains("metallic")) {
        desc.metallic = json["metallic"];
    }

    if (json.contains("albedo_map")) {
        desc.albedo_map = assets::load<texture>(json["albedo_map"]);
    } else {
        desc.albedo_map = texture::make_one_color(color::white(), { 2, 2 });
    }

    if (json.contains("normal_map")) {
        desc.normal_map = assets::load<texture>(json["normal_map"]);
    } else {
        desc.normal_map = texture::make_one_color({ 127, 127, 255, 255 }, { 2, 2 });
    }

    if (json.contains("roughness_map")) {
        desc.roughness_map = assets::load<texture>(json["roughness_map"]);
    } else {
        desc.roughness_map = texture::make_one_color(color::white(), { 2, 2 });
    }

    if (json.contains("metallic_map")) {
        desc.metallic_map = assets::load<texture>(json["metallic_map"]);
    } else {
        desc.metallic_map = texture::make_one_color(color::white(), { 2, 2 });
    }

    if (json.contains("emissive_map")) {
        desc.emissive_map = assets::load<texture>(json["emissive_map"]);
    } else {
        desc.emissive_map = texture::make_one_color(color::black(), { 2, 2 });
    }

    return graphics::make_material(desc);
}

void material::import(const std::string& input, const std::string& output, const json& metadata) {

}

const vec3f& material::base_color() const {
    return _base_color;
}

float material::roughness() const {
    return _roughness;
}

float material::metallic() const {
    return _metallic;
}

const std::shared_ptr<texture>& material::albedo_map() const {
    return _albedo_map;
}

const std::shared_ptr<texture>& material::normal_map() const {
    return _normal_map;
}

const std::shared_ptr<texture>& material::roughness_map() const {
    return _roughness_map;
}

const std::shared_ptr<texture>& material::metallic_map() const {
    return _metallic_map;
}

const std::shared_ptr<texture>& material::emissive_map() const {
    return _emissive_map;
}

material::material(const material_desc& desc)
    : _base_color(desc.base_color)
    , _roughness(desc.roughness)
    , _metallic(desc.metallic)
    , _albedo_map(desc.albedo_map)
    , _normal_map(desc.normal_map)
    , _roughness_map(desc.roughness_map)
    , _metallic_map(desc.metallic_map)
    , _emissive_map(desc.emissive_map) {
}

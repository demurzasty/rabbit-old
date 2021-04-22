#include <rabbit/graphics/material.hpp>

using namespace rb;

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

const vec3f& material::base_color() const RB_NOEXCEPT {
    return _base_color;
}

float material::roughness() const RB_NOEXCEPT {
    return _roughness;
}

float material::metallic() const RB_NOEXCEPT {
    return _metallic;
}

const std::shared_ptr<texture>& material::albedo_map() const RB_NOEXCEPT {
    return _albedo_map;
}

const std::shared_ptr<texture>& material::normal_map() const RB_NOEXCEPT {
    return _normal_map;
}

const std::shared_ptr<texture>& material::roughness_map() const RB_NOEXCEPT {
    return _roughness_map;
}

const std::shared_ptr<texture>& material::metallic_map() const RB_NOEXCEPT {
    return _metallic_map;
}

const std::shared_ptr<texture>& material::emissive_map() const RB_NOEXCEPT {
    return _emissive_map;
}

#include <rabbit/graphics/material.hpp>

using namespace rb;

color material::base_color() const noexcept {
    return _base_color;
}

float material::metallic() const noexcept {
    return _metallic;
}

float material::roughness() const noexcept {
    return _roughness;
}

color material::emissive() const noexcept {
    return _emissive;
}

material_alpha_mode material::alpha_mode() const noexcept {
    return _alpha_mode;
}

float material::alpha_cutoff() const noexcept {
    return _alpha_cutoff;
}

bool material::double_sided() const noexcept {
    return _double_sided;
}

bool material::clearcoat() const noexcept {
    return _clearcoat;
}

bool material::sheen() const noexcept {
    return _sheen;
}

bool material::transmission() const noexcept {
    return _transmission;
}

bool material::ior() const noexcept {
    return _ior;
}

bool material::volume() const noexcept {
    return _volume;
}

bool material::unlit() const noexcept {
    return _unlit;
}

const asset<texture>& material::albedo_map() const noexcept {
    return _albedo_map;
}

const asset<texture>& material::metallic_roughness_map() const noexcept {
    return _metallic_roughness_map;
}

const asset<texture>& material::normal_map() const noexcept {
    return _normal_map;
}

const asset<texture>& material::occlusion_map() const noexcept {
    return _occlusion_map;
}

const asset<texture>& material::emissive_map() const noexcept {
    return _emissive_map;
}

material::material(const material_desc& desc)
    : _base_color(desc.base_color)
    , _metallic(desc.metallic)
    , _roughness(desc.roughness)
    , _emissive(desc.emissive)
    , _alpha_mode(desc.alpha_mode)
    , _alpha_cutoff(desc.alpha_cutoff)
    , _double_sided(desc.double_sided)
    , _clearcoat(desc.clearcoat)
    , _sheen(desc.sheen)
    , _transmission(desc.transmission)
    , _ior(desc.ior)
    , _volume(desc.volume)
    , _unlit(desc.unlit)
    , _albedo_map(desc.albedo_map)
    , _metallic_roughness_map(desc.metallic_roughness_map)
    , _normal_map(desc.normal_map)
    , _occlusion_map(desc.occlusion_map)
    , _emissive_map(desc.emissive_map) {
}

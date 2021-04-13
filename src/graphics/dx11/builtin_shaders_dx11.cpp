#include <rabbit/graphics/builtin_shaders.hpp>

#include <shaders/forward.vert.comp.h>
#include <shaders/forward.frag.comp.h>
#include <shaders/irradiance.vert.comp.h>
#include <shaders/irradiance.frag.comp.h>
#include <shaders/brdf.vert.comp.h>
#include <shaders/brdf.frag.comp.h>
#include <shaders/prefilter.vert.comp.h>
#include <shaders/prefilter.frag.comp.h>
#include <shaders/skybox.vert.comp.h>
#include <shaders/skybox.frag.comp.h>

using namespace rb;

span<const std::uint8_t> builtin_shaders::get(builtin_shader shader) {
    switch (shader) {
        case builtin_shader::forward_vert:
            return forward_vert_comp;
        case builtin_shader::forward_frag:
            return forward_frag_comp;
        case builtin_shader::irradiance_vert:
            return irradiance_vert_comp;
        case builtin_shader::irradiance_frag:
            return irradiance_frag_comp;
        case builtin_shader::brdf_vert:
            return brdf_vert_comp;
        case builtin_shader::brdf_frag:
            return brdf_frag_comp;
        case builtin_shader::prefilter_vert:
            return prefilter_vert_comp;
        case builtin_shader::prefilter_frag:
            return prefilter_frag_comp;
        case builtin_shader::skybox_vert:
            return skybox_vert_comp;
        case builtin_shader::skybox_frag:
            return skybox_frag_comp;
    }

    return {};
}

#include <rabbit/graphics/builtin_shaders.hpp>

#include <shaders/forward.vert.glsl.h>
#include <shaders/forward.frag.glsl.h>
#include <shaders/irradiance.vert.glsl.h>
#include <shaders/irradiance.frag.glsl.h>
#include <shaders/brdf.vert.glsl.h>
#include <shaders/brdf.frag.glsl.h>
#include <shaders/prefilter.vert.glsl.h>
#include <shaders/prefilter.frag.glsl.h>
#include <shaders/skybox.vert.glsl.h>
#include <shaders/skybox.frag.glsl.h>

using namespace rb;

span<const std::uint8_t> builtin_shaders::get(builtin_shader shader) {
    switch (shader) {
        case builtin_shader::forward_vert:
            return forward_vert_glsl;
        case builtin_shader::forward_frag:
            return forward_frag_glsl;
        case builtin_shader::irradiance_vert:
            return irradiance_vert_glsl;
        case builtin_shader::irradiance_frag:
            return irradiance_frag_glsl;
        case builtin_shader::brdf_vert:
            return brdf_vert_glsl;
        case builtin_shader::brdf_frag:
            return brdf_frag_glsl;
        case builtin_shader::prefilter_vert:
            return prefilter_vert_glsl;
        case builtin_shader::prefilter_frag:
            return prefilter_frag_glsl;
        case builtin_shader::skybox_vert:
            return skybox_vert_glsl;
        case builtin_shader::skybox_frag:
            return skybox_frag_glsl;
    }

    return {};
}

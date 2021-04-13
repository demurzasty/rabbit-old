#include <rabbit/graphics/builtin_shaders.hpp>

#include <shaders/simple.vert.spv.h>
#include <shaders/simple.frag.spv.h>
#include <shaders/forward.vert.spv.h>
#include <shaders/forward.frag.spv.h>
#include <shaders/irradiance.vert.spv.h>
#include <shaders/irradiance.frag.spv.h>
#include <shaders/brdf.vert.spv.h>
#include <shaders/brdf.frag.spv.h>
#include <shaders/prefilter.vert.spv.h>
#include <shaders/prefilter.frag.spv.h>
#include <shaders/skybox.vert.spv.h>
#include <shaders/skybox.frag.spv.h>

using namespace rb;

span<const std::uint8_t> builtin_shaders::get(builtin_shader shader) {
    switch (shader) {
        case builtin_shader::simple_vert:
            return simple_vert_spv;
        case builtin_shader::simple_frag:
            return simple_frag_spv;
        case builtin_shader::forward_vert:
            return forward_vert_spv;
        case builtin_shader::forward_frag:
            return forward_frag_spv;
        case builtin_shader::irradiance_vert:
            return irradiance_vert_spv;
        case builtin_shader::irradiance_frag:
            return irradiance_frag_spv;
        case builtin_shader::brdf_vert:
            return brdf_vert_spv;
        case builtin_shader::brdf_frag:
            return brdf_frag_spv;
        case builtin_shader::prefilter_vert:
            return prefilter_vert_spv;
        case builtin_shader::prefilter_frag:
            return prefilter_frag_spv;
        case builtin_shader::skybox_vert:
            return skybox_vert_spv;
        case builtin_shader::skybox_frag:
            return skybox_frag_spv;
    }

    return {};
}

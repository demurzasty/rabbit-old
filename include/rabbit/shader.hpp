#pragma once 

// TODO: Custom shaders support.

namespace rb {
    enum class shader_stage {
        vertex,
        fragment,
        geometry,
        tess_control,
        tess_evaluation,
        compute
    };
}

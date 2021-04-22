#include <rabbit/graphics/shader_data.hpp>

using namespace rb;

shader_data::shader_data(const shader_data_desc& desc)
    : _shader(desc.shader) {
    RB_ASSERT(_shader, "Shader is not provided");
}

const std::shared_ptr<shader>& shader_data::shader() const RB_NOEXCEPT {
    return _shader;
}

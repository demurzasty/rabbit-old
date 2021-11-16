#include <rabbit/glsl.hpp>
#include <rabbit/config.hpp>

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <glslang/Include/Types.h>
#include <glslang/Public/ShaderLang.h>

#include <map>

using namespace rb;

static std::map<shader_stage, EShLanguage> stages = {
    { shader_stage::vertex, EShLangVertex },
    { shader_stage::fragment, EShLangFragment },
    { shader_stage::geometry, EShLangGeometry },
    { shader_stage::tess_control, EShLangTessControl },
    { shader_stage::tess_evaluation, EShLangTessEvaluation },
    { shader_stage::compute, EShLangCompute },
};

std::vector<std::uint32_t> glsl::compile(shader_stage stage, const std::string& code) {
    static const auto initialized = glslang::InitializeProcess();
    RB_ASSERT(initialized, "GLSLang is not initialized.");

    glslang::TShader::ForbidIncluder includer;

    glslang::TShader shader{ stages.at(stage) };

    const char* codes[]{ code.c_str() };
    shader.setStrings(codes, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stages.at(stage), glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    const auto messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    std::string pre_processed_code;
    if (!shader.preprocess(&glslang::DefaultTBuiltInResource, 100, ENoProfile, false, false, messages, &pre_processed_code, includer)) {
        print("Failed pre-process:\n{}\n{}\n", shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    codes[0] = pre_processed_code.c_str();
    shader.setStrings(codes, 1);

    if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages)) {
        print("Failed parse:\n{}\n{}\n", shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        print("Failed link:\n{}\n{}\n", shader.getInfoLog(), shader.getInfoDebugLog());
        return {};
    }

    std::vector<std::uint32_t> spirv;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    glslang::GlslangToSpv(*program.getIntermediate(stages.at(stage)), spirv, &logger, &options);
    return spirv;
}

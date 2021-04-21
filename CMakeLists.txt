cmake_minimum_required (VERSION 3.8.2)

project (rabbit)

if (WIN32)
	set (RB_WINDOWS TRUE)
elseif (UNIX AND NOT APPLE)
	set (RB_LINUX TRUE)
elseif (APPLE)
	set (RB_OSX TRUE)
endif()

set (RB_VULKAN TRUE)

find_package (Python COMPONENTS Interpreter)

set (GLSL_VALIDATOR "${CMAKE_CURRENT_SOURCE_DIR}/bin/glslangValidator.exe")

file (GLOB_RECURSE GLSL_SOURCE_FILES "data/shaders/*.vert" "data/shaders/*.frag")

foreach (GLSL_SRC ${GLSL_SOURCE_FILES})
	get_filename_component (FILE_NAME ${GLSL_SRC} NAME)
	get_filename_component (FILE_EXT ${GLSL_SRC} EXT)

    set (GENERATED_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/generated/include/rabbit/generated/shaders")
	set (SPIRV_H "${GENERATED_DIRECTORY}/${FILE_NAME}.spv.h")
    
    string (REPLACE "." "_" VARIABLE_NAME ${FILE_NAME})

	add_custom_command (
        OUTPUT ${SPIRV_H}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIRECTORY}
        # GLSL -> SPIR-V -> C Header
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL_SRC} --vn "${VARIABLE_NAME}" -o ${SPIRV_H}
        DEPENDS ${GLSL_SRC}
    )
    list (APPEND SPIRV_BINARY_FILES ${SPIRV_H})
endforeach (GLSL_SRC)

add_custom_target (shaders DEPENDS ${SPIRV_BINARY_FILES})

add_subdirectory ("lib")

set (SRC 
    "src/application.cpp"
    "src/asset_manager.cpp"
    "src/buffer.cpp"
    "src/builder.cpp"
    "src/graphics_device.cpp"
    "src/material.cpp"
    "Src/mesh_loader.cpp"
    "src/mesh.cpp"
    "src/renderer.cpp"
    "src/resource_heap.cpp"
    "src/system.cpp"
    "src/texture_loader.cpp"
    "src/texture.cpp"
    "src/vertex.cpp"
    "src/window.cpp"
)

if (RB_WINDOWS)
    set (SRC ${SRC} 
        "src/win32/window_win32.cpp"
    )
endif ()

if (TRUE)
    set (SRC ${SRC} 
        "src/vulkan/buffer_vulkan.cpp"
        "src/vulkan/graphics_device_vulkan.cpp"
        "src/vulkan/material_vulkan.cpp"
        "src/vulkan/resource_heap_vulkan.cpp"
        "src/vulkan/texture_vulkan.cpp"
        "src/vulkan/utils_vulkan.cpp"
    )
endif ()

add_library (rabbit STATIC ${SRC})
target_include_directories (rabbit PUBLIC "include")
target_include_directories (rabbit PRIVATE "generated/include")
target_compile_features (rabbit PUBLIC cxx_std_17)
target_link_libraries (rabbit PUBLIC vma volk stb gsl json entt)
target_compile_definitions (rabbit PRIVATE VMA_STATIC_VULKAN_FUNCTIONS NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
add_dependencies (rabbit shaders)

if (MSVC)
    target_compile_definitions (rabbit PRIVATE _CRT_SECURE_NO_WARNINGS)
endif ()

target_compile_definitions (rabbit PUBLIC RB_WINDOWS=$<BOOL:${RB_WINDOWS}>)
target_compile_definitions (rabbit PUBLIC RB_LINUX=$<BOOL:${RB_LINUX}>)
target_compile_definitions (rabbit PUBLIC RB_MACOS=$<BOOL:${RB_MACOS}>)

target_compile_definitions (rabbit PUBLIC RB_VULKAN=$<BOOL:${RB_VULKAN}>)


if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory ("example")
endif ()

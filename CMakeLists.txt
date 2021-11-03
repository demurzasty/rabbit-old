cmake_minimum_required (VERSION 3.16)

project (rabbit)

if (WIN32)
	set (RB_WINDOWS TRUE)
elseif (UNIX AND NOT APPLE)
	set (RB_LINUX TRUE)
elseif (APPLE)
	set (RB_OSX TRUE)
endif()

set (RB_VULKAN TRUE)
set (RB_DIRECTX12 FALSE) 

option (RB_PROD_BUILD "Enable production build" OFF)

add_subdirectory ("lib")

set (GLSLANG_VALIDATOR "${CMAKE_CURRENT_SOURCE_DIR}/bin/win32/glslangValidator.exe")

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
		COMMAND ${GLSLANG_VALIDATOR} -V ${GLSL_SRC} --vn "${VARIABLE_NAME}" -Os -o ${SPIRV_H}
		DEPENDS ${GLSL_SRC}
	)
	list (APPEND SPIRV_BINARY_FILES ${SPIRV_H})
endforeach (GLSL_SRC)

add_custom_target (shaders DEPENDS ${SPIRV_BINARY_FILES})

set (SRC 
	"src/app.cpp"
	"src/assets.cpp"
	"src/camera_manager.cpp"
	"src/compression.cpp"
	"src/environment.cpp"
	"src/graphics.cpp"
	"src/image.cpp"
	"src/input.cpp"
	"src/material.cpp"
	"src/mesh.cpp"
	"src/prefab.cpp"
	"src/renderer.cpp"
	"src/s3tc.cpp"
	"src/settings.cpp"
	"src/system.cpp"
	"src/texture.cpp"
	"src/uuid.cpp"
	"src/viewport.cpp"
	"src/window.cpp"
)

if (NOT RB_PROD_BUILD) 
	set (SRC ${SRC}
		"src/editor/editor.cpp"
	)
endif ()

if (RB_WINDOWS) 
	set (SRC ${SRC}
		"src/win32/input_win32.cpp"
		"src/win32/window_win32.cpp"
	)
endif ()

if (RB_VULKAN) 
	set (SRC ${SRC}
		"src/vulkan/environment_vulkan.cpp"
		"src/vulkan/graphics_vulkan.cpp"
		"src/vulkan/material_vulkan.cpp"
		"src/vulkan/mesh_vulkan.cpp"
		"src/vulkan/shaders_vulkan.cpp"
		"src/vulkan/texture_vulkan.cpp"
		"src/vulkan/utils_vulkan.cpp"
		"src/vulkan/viewport_vulkan.cpp"
	)
endif ()

add_library (rabbit STATIC ${SRC})
target_compile_features (rabbit PUBLIC cxx_std_17)
target_include_directories (rabbit PUBLIC "include" PRIVATE "generated/include")
target_link_libraries (rabbit PUBLIC rgbcx miniz quickhull meshoptimizer stb fmt gsl json entt)
add_dependencies (rabbit shaders)

if (MSVC)
	target_compile_definitions (rabbit PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif ()

if (RB_VULKAN)
	target_link_libraries (rabbit PUBLIC vma volk)
	target_compile_definitions (rabbit PRIVATE VMA_STATIC_VULKAN_FUNCTIONS VK_USE_PLATFORM_WIN32_KHR)
endif ()

target_compile_definitions (rabbit PUBLIC RB_WINDOWS=$<BOOL:${RB_WINDOWS}>)
target_compile_definitions (rabbit PUBLIC RB_LINUX=$<BOOL:${RB_LINUX}>)
target_compile_definitions (rabbit PUBLIC RB_MACOS=$<BOOL:${RB_MACOS}>)

target_compile_definitions (rabbit PUBLIC RB_VULKAN=$<BOOL:${RB_VULKAN}>)
target_compile_definitions (rabbit PUBLIC RB_DIRECTX12=$<BOOL:${RB_DIRECTX12}>)

target_compile_definitions (rabbit PUBLIC -DRB_PROD_BUILD=$<BOOL:${RB_PROD_BUILD}>)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory ("example")
endif ()

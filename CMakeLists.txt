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

option (RB_EXPERIMENTAL "Enable experimental features" OFF)
option (RB_PROD_BUILD "Enable production build" OFF)

add_subdirectory ("lib")

set (GLSLANG_VALIDATOR "${CMAKE_CURRENT_SOURCE_DIR}/bin/win32/glslangValidator.exe")
set (B2H "${CMAKE_CURRENT_SOURCE_DIR}/bin/win32/b2h.exe")

file (GLOB_RECURSE GLSL_SOURCE_FILES
	"src/graphics/shaders/*.vert"
	"src/graphics/shaders/*.frag"
	"src/graphics/shaders/*.tesc"
	"src/graphics/shaders/*.tese"
	"src/graphics/shaders/*.geom"
	"src/graphics/shaders/*.comp"
)

set (GENERATED_SHADERS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shaders")

foreach (GLSL_SRC ${GLSL_SOURCE_FILES})
	get_filename_component (FILE_NAME ${GLSL_SRC} NAME)
	get_filename_component (FILE_EXT ${GLSL_SRC} EXT)

	set (GLSL_H "${GENERATED_SHADERS_DIRECTORY}/${FILE_NAME}.generated.h")
	# set (SPIRV_H "${GENERATED_DIRECTORY}/${FILE_NAME}.spv.h")

	string (REPLACE "." "_" VARIABLE_NAME ${FILE_NAME})

	add_custom_command (
		OUTPUT ${GLSL_H}
		# COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIRECTORY}
		# GLSL -> C Header
		COMMAND ${B2H} ${GLSL_SRC} ${GLSL_H} "${VARIABLE_NAME}"
		# GLSL -> SPIR-V -> C Header
		# COMMAND ${GLSLANG_VALIDATOR} -V ${GLSL_SRC} --vn "${VARIABLE_NAME}" -Os -o ${SPIRV_H}
		DEPENDS ${GLSL_SRC}
	)
	list (APPEND SPIRV_BINARY_FILES ${GLSL_H})
endforeach (GLSL_SRC)

add_custom_target (shaders DEPENDS ${SPIRV_BINARY_FILES})

set (SRC 
	"src/core/app.cpp"
	"src/core/assets.cpp"
	"src/core/clock.cpp"
	"src/core/prefab.cpp"
	"src/core/reflection.cpp"
	"src/core/settings.cpp"
	"src/core/system.cpp"
	"src/core/time.cpp"
	"src/core/uuid.cpp"
	"src/core/world.cpp"
	"src/graphics/glsl.cpp"
	"src/graphics/graphics.cpp"
	"src/graphics/material.cpp"
	"src/graphics/mesh.cpp"
	"src/graphics/texture.cpp"
	"src/loaders/texture_loader.cpp"
	"src/platform/window.cpp"
	"src/render/renderer.cpp"
)

if (NOT RB_PROD_BUILD) 
	set (SRC ${SRC}
		"src/editor/importers/texture_importer.cpp"
		"src/editor/editor.cpp"
		"src/editor/reflection.cpp"
	)
endif ()

if (RB_WINDOWS) 
	set (SRC ${SRC}
		"src/drivers/win32/window_win32.cpp"
	)
endif ()

if (RB_VULKAN) 
	set (SRC ${SRC}
		"src/drivers/vulkan/graphics_vulkan.cpp"
		"src/drivers/vulkan/texture_vulkan.cpp"
		"src/drivers/vulkan/utils_vulkan.cpp"
	)
endif ()

add_library (rabbit STATIC ${SRC})
target_compile_features (rabbit PUBLIC cxx_std_17)
target_include_directories (rabbit PUBLIC "include")
target_link_libraries (rabbit PUBLIC fmt gsl json entt)
target_link_libraries (rabbit PRIVATE glslang OSDependent OGLCompiler glslang SPIRV glslang-default-resource-limits)
target_link_libraries (rabbit PRIVATE rgbcx miniz quickhull meshoptimizer stb)
add_dependencies (rabbit shaders)

if (MSVC)
	target_compile_definitions (rabbit PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif ()

if (RB_VULKAN)
	target_link_libraries (rabbit PRIVATE vma volk)
	target_compile_definitions (rabbit PRIVATE VMA_STATIC_VULKAN_FUNCTIONS VK_USE_PLATFORM_WIN32_KHR)
endif ()

target_compile_definitions (rabbit PUBLIC RB_WINDOWS=$<BOOL:${RB_WINDOWS}>)
target_compile_definitions (rabbit PUBLIC RB_LINUX=$<BOOL:${RB_LINUX}>)
target_compile_definitions (rabbit PUBLIC RB_MACOS=$<BOOL:${RB_MACOS}>)

target_compile_definitions (rabbit PUBLIC RB_VULKAN=$<BOOL:${RB_VULKAN}>)
target_compile_definitions (rabbit PUBLIC RB_DIRECTX12=$<BOOL:${RB_DIRECTX12}>)

target_compile_definitions (rabbit PUBLIC -DRB_EXPERIMENTAL=$<BOOL:${RB_EXPERIMENTAL}>)
target_compile_definitions (rabbit PUBLIC -DRB_PROD_BUILD=$<BOOL:${RB_PROD_BUILD}>)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory ("example")
endif ()

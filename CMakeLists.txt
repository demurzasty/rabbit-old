cmake_minimum_required (VERSION 3.16)

project (rabbit)

if (EMSCRIPTEN)
	set (RB_EMSCRIPTEN TRUE)
elseif (WIN32)
	set (RB_WINDOWS TRUE)
elseif (UNIX AND NOT APPLE)
	set (RB_LINUX TRUE)
elseif (APPLE)
	set (RB_OSX TRUE)
endif()

set (RB_GRAPHICS_BACKEND_DIRECTX FALSE)
set (RB_GRAPHICS_BACKEND_OPENGL FALSE)
set (RB_GRAPHICS_BACKEND_METAL FALSE)

set (RB_PLATFORM_BACKEND_SDL2 FALSE)

set (RB_GAMEPAD_BACKEND_XINPUT FALSE)
set (RB_GAMEPAD_BACKEND_SDL2 FALSE)

set (RB_AUDIO_BACKEND_XAUDIO FALSE)
set (RB_AUDIO_BACKEND_OPENAL FALSE)

if (RB_EMSCRIPTEN)
	set (RB_GRAPHICS_BACKEND_OPENGL TRUE)
elseif (RB_WINDOWS)
	if (MSVC)
		set (RB_GRAPHICS_BACKEND_DIRECTX TRUE)
	else ()
		set (RB_GRAPHICS_BACKEND_OPENGL TRUE)
	endif ()

	set (RB_PLATFORM_BACKEND_SDL2 TRUE)
	set (RB_GAMEPAD_BACKEND_SDL2 TRUE)
	set (RB_AUDIO_BACKEND_XAUDIO TRUE)
elseif (RB_LINUX)
	set (RB_GRAPHICS_BACKEND_OPENGL TRUE)
	set (RB_PLATFORM_BACKEND_SDL2 TRUE)
	set (RB_GAMEPAD_BACKEND_SDL2 TRUE)
	set (RB_AUDIO_BACKEND_OPENAL TRUE)
elseif (RB_OSX)
	set (RB_GRAPHICS_BACKEND_METAL TRUE)
	set (RB_PLATFORM_BACKEND_SDL2 TRUE)
	set (RB_GAMEPAD_BACKEND_SDL2 TRUE)
	set (RB_AUDIO_BACKEND_OPENAL TRUE)
endif ()

option (RB_PROD_BUILD "Enable production build" OFF)

add_subdirectory ("lib")

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
	set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
	set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

file (GLOB_RECURSE GLSL_SOURCE_FILES "data/shaders/*.vert" "data/shaders/*.frag")

foreach (GLSL ${GLSL_SOURCE_FILES})
	get_filename_component (FILE_NAME ${GLSL} NAME)
	set (SPIRV "${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/${FILE_NAME}.spv")
	add_custom_command (
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/"
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
	)
	list (APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target (shaders DEPENDS ${SPIRV_BINARY_FILES})

set (SRC 
	"src/asset_manager.cpp"
	"src/buffer.cpp"
	"src/builder.cpp"
	"src/clock.cpp"
	"src/exception.cpp"
	"src/game.cpp"
	"src/gamepad.cpp"
	"src/graphics_device.cpp"
	"src/image.cpp"
	"src/keyboard.cpp"
	"src/loader.cpp"
	"src/mouse.cpp"
	"src/sprite_batch.cpp"
	"src/system.cpp"
	"src/texture_loader.cpp"
	"src/texture.cpp"
	"src/uuid.cpp"
	"src/window.cpp"

	"src/dummy/gamepad_dummy.cpp"
	"src/dummy/keyboard_dummy.cpp"
	"src/dummy/mouse_dummy.cpp"
)

if (RB_PLATFORM_BACKEND_WIN32) 
	set (SRC ${SRC}
		"src/win32/window_win32.cpp"
		"src/win32/keyboard_win32.cpp"
		"src/win32/mouse_win32.cpp"
	)
endif ()

if (RB_PLATFORM_BACKEND_SDL2)
	set (SRC ${SRC}
		"src/sdl2/window_sdl2.cpp"
		"src/sdl2/keyboard_sdl2.cpp"
		"src/sdl2/mouse_sdl2.cpp"
		"src/sdl2/gamepad_sdl2.cpp"
		"src/sdl2/gamecontrollerdb_sdl2.cpp"
	)
endif ()

if (RB_GAMEPAD_BACKEND_XINPUT) 
	set (SRC ${SRC}
		"src/xinput/gamepad_xinput.cpp"
	)
endif ()

if (RB_GRAPHICS_BACKEND_DIRECTX) 
	set (SRC ${SRC}
		"src/dx11/graphics_device_dx11.cpp"
		"src/dx11/texture_dx11.cpp"
		"src/dx11/buffer_dx11.cpp"
		"src/dx11/standard_shaders_dx11.cpp"
	)
endif ()

if (RB_GRAPHICS_BACKEND_OPENGL) 
	set (SRC ${SRC}
		"src/ogl3/graphics_device_ogl3.cpp"
		"src/ogl3/texture_ogl3.cpp"
		"src/ogl3/buffer_ogl3.cpp"
		"src/ogl3/standard_shaders_ogl3.cpp"
	)
endif ()

add_library (rabbit STATIC ${SRC})
target_compile_features (rabbit PUBLIC cxx_std_17)
target_include_directories (rabbit PUBLIC "include" "generated")
target_link_libraries (rabbit PUBLIC entt fmt stb json gsl)
add_dependencies (rabbit shaders)

if (MSVC) 
	target_compile_definitions (rabbit PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif ()

if (NOT RB_EMSCRIPTEN AND RB_PLATFORM_BACKEND_SDL2)
	target_link_libraries (rabbit PUBLIC SDL2-static)
endif ()

if (NOT RB_EMSCRIPTEN AND RB_GRAPHICS_BACKEND_OPENGL) 
	target_link_libraries (rabbit PRIVATE glew)

	if (RB_WINDOWS)
		target_link_libraries (rabbit PRIVATE opengl32)
	elseif (RB_LINUX)
		target_link_libraries (rabbit PRIVATE GL)
	endif ()
endif ()

target_compile_definitions (rabbit PUBLIC -DRB_EMSCRIPTEN=$<BOOL:${RB_EMSCRIPTEN}>)
target_compile_definitions (rabbit PUBLIC -DRB_WINDOWS=$<BOOL:${RB_WINDOWS}>)
target_compile_definitions (rabbit PUBLIC -DRB_LINUX=$<BOOL:${RB_LINUX}>)
target_compile_definitions (rabbit PUBLIC -DRB_MACOS=$<BOOL:${RB_MACOS}>)
target_compile_definitions (rabbit PUBLIC -DRB_SWITCH=$<BOOL:${RB_SWITCH}>)

target_compile_definitions (rabbit PUBLIC -DRB_PLATFORM_BACKEND_WIN32=$<BOOL:${RB_PLATFORM_BACKEND_WIN32}>)
target_compile_definitions (rabbit PUBLIC -DRB_PLATFORM_BACKEND_SDL2=$<BOOL:${RB_PLATFORM_BACKEND_SDL2}>)

target_compile_definitions (rabbit PUBLIC -DRB_GAMEPAD_BACKEND_SDL2=$<BOOL:${RB_GAMEPAD_BACKEND_SDL2}>)
target_compile_definitions (rabbit PUBLIC -DRB_GAMEPAD_BACKEND_XINPUT=$<BOOL:${RB_GAMEPAD_BACKEND_XINPUT}>)

target_compile_definitions (rabbit PUBLIC -DRB_GRAPHICS_BACKEND_DIRECTX=$<BOOL:${RB_GRAPHICS_BACKEND_DIRECTX}>)
target_compile_definitions (rabbit PUBLIC -DRB_GRAPHICS_BACKEND_OPENGL=$<BOOL:${RB_GRAPHICS_BACKEND_OPENGL}>)
target_compile_definitions (rabbit PUBLIC -DRB_GRAPHICS_BACKEND_METAL=$<BOOL:${RB_GRAPHICS_BACKEND_METAL}>)

target_compile_definitions (rabbit PUBLIC -DRB_AUDIO_BACKEND_XAUDIO=$<BOOL:${RB_AUDIO_BACKEND_XAUDIO}>)
target_compile_definitions (rabbit PUBLIC -DRB_AUDIO_BACKEND_OPENAL=$<BOOL:${RB_AUDIO_BACKEND_OPENAL}>)

target_compile_definitions (rabbit PUBLIC -DRB_PROD_BUILD=$<BOOL:${RB_PROD_BUILD}>)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory ("examples")
endif ()

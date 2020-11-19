cmake_minimum_required (VERSION 3.16)

project (rabbit)

if (WIN32)
	set (RB_WINDOWS TRUE)
elseif (UNIX AND NOT APPLE)
	set (RB_LINUX TRUE)
elseif (APPLE)
	set (RB_MACOS TRUE)
endif()

set (RB_GRAPHICS_BACKEND_DIRECTX FALSE)
set (RB_GRAPHICS_BACKEND_OPENGL FALSE)
set (RB_GRAPHICS_BACKEND_METAL FALSE)

set (RB_PLATFORM_BACKEND_WIN32 FALSE)
set (RB_PLATFORM_BACKEND_X11 FALSE)
set (RB_PLATFORM_BACKEND_COCOA FALSE)

set (RB_GAMEPAD_BACKEND_XINPUT FALSE)

set (RB_AUDIO_BACKEND_XAUDIO FALSE)
set (RB_AUDIO_BACKEND_OPENAL FALSE)

if (RB_WINDOWS)
	set (RB_GRAPHICS_BACKEND_DIRECTX TRUE)
	set (RB_GRAPHICS_BACKEND_OPENGL TRUE)
	set (RB_PLATFORM_BACKEND_WIN32 TRUE)
	set (RB_GAMEPAD_BACKEND_XINPUT TRUE)
	set (RB_AUDIO_BACKEND_XAUDIO TRUE)
elseif (RB_LINUX)
	set (RB_GRAPHICS_BACKEND_OPENGL TRUE)
	set (RB_PLATFORM_BACKEND_X11 TRUE)
	set (RB_AUDIO_BACKEND_OPENAL TRUE)
elseif (RB_MACOX)
	set (RB_GRAPHICS_BACKEND_METAL TRUE)
	set (RB_PLATFORM_BACKEND_COCOA TRUE)
	set (RB_AUDIO_BACKEND_OPENAL TRUE)
endif ()

option (RB_PROD_BUILD "Enable production build" OFF)

add_subdirectory ("lib")

set (SRC 
	"src/config.cpp"
	"src/uuid.cpp"
	"src/clock.cpp"
	"src/window.cpp"
	"src/keyboard.cpp"
	"src/mouse.cpp"
	"src/gamepad.cpp"
	"src/game.cpp"
	"src/texture.cpp"
	"src/texture_loader.cpp"
	"src/graphics_device.cpp"
	"src/sprite_batch.cpp"
	"src/image.cpp"
	"src/state.cpp"
	"src/state_manager.cpp"
)

if (RB_PLATFORM_BACKEND_WIN32) 
	set (SRC ${SRC}
		"src/win32/window_win32.cpp"
		"src/win32/keyboard_win32.cpp"
		"src/win32/mouse_win32.cpp"
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
		"src/dx11/standard_shaders_dx11.cpp"
	)
endif ()

if (RB_GRAPHICS_BACKEND_OPENGL) 
	set (SRC ${SRC}
		"src/ogl3/graphics_device_ogl3.cpp"
		"src/ogl3/texture_ogl3.cpp"
		"src/ogl3/standard_shaders_ogl3.cpp"
	)
endif ()

add_library (rabbit STATIC ${SRC})
target_compile_features (rabbit PUBLIC cxx_std_17)
target_include_directories (rabbit PUBLIC "include")
target_link_libraries (rabbit PUBLIC stb json gsl)

if (MSVC) 
	target_compile_definitions (rabbit PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif ()

target_compile_definitions (rabbit PUBLIC -DRB_WINDOWS=$<BOOL:${RB_WINDOWS}>)
target_compile_definitions (rabbit PUBLIC -DRB_LINUX=$<BOOL:${RB_LINUX}>)
target_compile_definitions (rabbit PUBLIC -DRB_MACOS=$<BOOL:${RB_MACOS}>)
target_compile_definitions (rabbit PUBLIC -DRB_SWITCH=$<BOOL:${RB_SWITCH}>)

target_compile_definitions (rabbit PUBLIC -DRB_PLATFORM_BACKEND_WIN32=$<BOOL:${RB_PLATFORM_BACKEND_WIN32}>)
target_compile_definitions (rabbit PUBLIC -DRB_PLATFORM_BACKEND_X11=$<BOOL:${RB_PLATFORM_BACKEND_X11}>)
target_compile_definitions (rabbit PUBLIC -DRB_PLATFORM_BACKEND_SWITCH=$<BOOL:${RB_PLATFORM_BACKEND_SWITCH}>)

target_compile_definitions (rabbit PUBLIC -DRB_GAMEPAD_BACKEND_XINPUT=$<BOOL:${RB_GAMEPAD_BACKEND_XINPUT}>)

target_compile_definitions (rabbit PUBLIC -DRB_GRAPHICS_BACKEND_DIRECTX=$<BOOL:${RB_GRAPHICS_BACKEND_DIRECTX}>)
target_compile_definitions (rabbit PUBLIC -DRB_GRAPHICS_BACKEND_OPENGL=$<BOOL:${RB_GRAPHICS_BACKEND_OPENGL}>)
target_compile_definitions (rabbit PUBLIC -DRB_GRAPHICS_BACKEND_METAL=$<BOOL:${RB_GRAPHICS_BACKEND_METAL}>)

target_compile_definitions (rabbit PUBLIC -DRB_AUDIO_BACKEND_XAUDIO=$<BOOL:${RB_AUDIO_BACKEND_XAUDIO}>)
target_compile_definitions (rabbit PUBLIC -DRB_AUDIO_BACKEND_OPENAL=$<BOOL:${RB_AUDIO_BACKEND_OPENAL}>)

target_compile_definitions (rabbit PUBLIC -DRB_PROD_BUILD=$<BOOL:${RB_PROD_BUILD}>)

add_subdirectory ("examples")

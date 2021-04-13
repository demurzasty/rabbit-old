cmake_minimum_required (VERSION 3.8.2)

add_subdirectory ("entt-3.6.0")
add_subdirectory ("gsl-3.1.0")
add_subdirectory ("json-3.9.1")
add_subdirectory ("stb")
add_subdirectory ("fmt-7.1.2")

if (NOT EMSCRIPTEN AND (RB_WINDOWS OR RB_LINUX OR RB_OSX))
	if (RB_PLATFORM_BACKEND_SDL2)
		set (SDL_SHARED_ENABLED_BY_DEFAULT OFF)
		add_subdirectory ("SDL2-2.0.12")
	endif ()

	if (RB_GRAPHICS_BACKEND_OPENGL) 
		add_subdirectory ("glew-2.1.0")
	endif ()
endif ()

if (RB_GRAPHICS_BACKEND_VULKAN)
	if (RB_WINDOWS)
		set (VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
	endif ()

	add_subdirectory ("volk-1.2.170")
endif ()
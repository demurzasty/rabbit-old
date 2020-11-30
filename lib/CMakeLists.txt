cmake_minimum_required (VERSION 3.8.2)

add_subdirectory ("gsl-3.1.0")
add_subdirectory ("json-3.9.1")
add_subdirectory ("stb")
add_subdirectory ("fmt-7.1.2")

if (NOT EMSCRIPTEN AND (RB_WINDOWS OR RB_LINUX OR RB_OSX))
	set (SDL_SHARED_ENABLED_BY_DEFAULT OFF)
	add_subdirectory ("SDL2-2.0.12")

	if (RB_GRAPHICS_BACKEND_OPENGL) 
		add_subdirectory ("glew-2.1.0")
	endif ()
endif ()

if (NOT RB_EMSCRIPTEN AND RB_PLATFORM_BACKEND_GLFW)
	set (GLFW_BUILD_DOCS OFF CACHE BOOL "Disable docs building for GLFW")
	set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples building for GLFW")
	set (GLFW_BUILD_TESTS OFF CACHE BOOL "Disable tests building for GLFW")
	set (GLFW_INSTALL OFF CACHE BOOL "Disable GLFW installation")
	add_subdirectory ("glfw-3.3.2")
endif ()
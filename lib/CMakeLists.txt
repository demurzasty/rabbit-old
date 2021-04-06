cmake_minimum_required (VERSION 3.8.2)

add_subdirectory ("entt-3.6.0")
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

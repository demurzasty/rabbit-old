cmake_minimum_required (VERSION 3.8.2)

add_subdirectory ("gsl-3.1.0")
add_subdirectory ("json-3.9.1")
add_subdirectory ("stb")

if (RB_GRAPHICS_BACKEND_OPENGL) 
	add_subdirectory ("glad-0.1.34")
endif ()

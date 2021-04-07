cmake_minimum_required (VERSION 3.16)

add_executable (basic "src/main.cpp")
target_compile_features (basic PUBLIC cxx_std_17)
target_link_libraries (basic PUBLIC rabbit)
target_compile_definitions (basic PUBLIC -DDATA_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_custom_command(
    TARGET basic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            ${CMAKE_CURRENT_BINARY_DIR}/data
)
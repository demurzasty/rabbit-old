cmake_minimum_required (VERSION 3.16)

add_executable (gamepad "src/main.cpp")
target_compile_features (gamepad PUBLIC cxx_std_17)
target_link_libraries (gamepad PUBLIC rabbit)

add_custom_command(
    TARGET gamepad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            ${CMAKE_CURRENT_BINARY_DIR}/data
)
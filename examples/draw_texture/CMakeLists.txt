cmake_minimum_required (VERSION 3.16)

add_executable (draw_texture "src/main.cpp")
target_compile_features (draw_texture PUBLIC cxx_std_17)
target_link_libraries (draw_texture PUBLIC rabbit)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

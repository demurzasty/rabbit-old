cmake_minimum_required (VERSION 3.8.2)

add_executable (example "src/main.cpp")
target_link_libraries (example PUBLIC rabbit)
target_compile_definitions (example PUBLIC DATA_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/data")

#add_custom_command(
#    TARGET example POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory 
#            ${CMAKE_CURRENT_SOURCE_DIR}/data
#            ${CMAKE_CURRENT_BINARY_DIR}/data
#)

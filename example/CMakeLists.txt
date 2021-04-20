cmake_minimum_required (VERSION 3.8.2)

add_executable (example "src/main.cpp")
target_link_libraries (example PUBLIC rabbit)

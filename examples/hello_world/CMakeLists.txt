cmake_minimum_required (VERSION 3.16)

add_executable (hello_world "src/main.cpp")
target_compile_features (hello_world PUBLIC cxx_std_17)
target_link_libraries (hello_world PUBLIC rabbit)

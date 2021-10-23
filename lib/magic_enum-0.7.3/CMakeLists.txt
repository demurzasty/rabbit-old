cmake_minimum_required (VERSION 3.8.2)

add_library (magic_enum INTERFACE)
target_include_directories (magic_enum INTERFACE "include")
target_compile_features (magic_enum INTERFACE cxx_std_17)

cmake_minimum_required (VERSION 3.8.2)

add_library (base64 STATIC "src/base64.cpp")
target_include_directories (base64 PUBLIC "include")
target_compile_features (base64 PUBLIC cxx_std_17)

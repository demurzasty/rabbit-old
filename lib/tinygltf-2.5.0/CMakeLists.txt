cmake_minimum_required (VERSION 3.8.2)

add_library (tinygltf INTERFACE)
target_include_directories (tinygltf INTERFACE "include")
target_compile_features (tinygltf INTERFACE cxx_std_11)

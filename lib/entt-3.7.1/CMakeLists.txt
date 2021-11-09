cmake_minimum_required (VERSION 3.8.2)

add_library (entt INTERFACE)
target_include_directories (entt INTERFACE "include")
target_compile_features (entt INTERFACE cxx_std_17)

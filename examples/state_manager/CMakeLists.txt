cmake_minimum_required (VERSION 3.16)

add_executable (state_manager "src/main.cpp" "src/test_state.cpp")
target_compile_features (state_manager PUBLIC cxx_std_17)
target_link_libraries (state_manager PUBLIC rabbit)

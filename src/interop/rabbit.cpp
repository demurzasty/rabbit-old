#include <rabbit/rabbit.hpp>

using namespace rb;

extern "C" {
    __declspec(dllexport) void __cdecl app_setup() {
        app::setup();
    }

    __declspec(dllexport) void __cdecl app_run(const char* initial_scene) {
        app::run(initial_scene);
    }
}

#include <rabbit/platform/window.hpp>

#if RB_WINDOWS
#include "win32/window_win32.hpp"
namespace rb { using window_impl = window_win32; }
#endif

using namespace rb;

void window::install(installer<window>& installer) {
    installer.install<window_impl>();
}

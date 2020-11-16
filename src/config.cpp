#include <rabbit/config.hpp>

using namespace rb;

config::config() {
#if RB_WINDOW_BACKEND_WIN32
    window_backend = window_backend::win32;
#elif RB_WINDOW_BACKEND_SWITCH
    window_backend = window_backend::nswitch;
#endif

#if RB_KEYBOARD_BACKEND_WIN32
    keyboard_backend = keyboard_backend::win32;
#endif

#if RB_MOUSE_BACKEND_WIN32
    mouse_backend = mouse_backend::win32;
#endif

#if RB_GAMEPAD_BACKEND_XINPUT
    gamepad_backend = gamepad_backend::xinput;
#elif RB_GAMEPAD_BACKEND_DINPUT
    gamepad_backend = gamepad_backend::dinput;
#endif

#if RB_GRAPHICS_BACKEND_DIRECTX
    graphics_backend = graphics_backend::directx11;
#elif RB_GRAPHICS_BACKEND_OPENGL
    graphics_backend = graphics_backend::opengl3;
#endif

#if RB_AUDIO_BACKEND_XAUDIO
    audio_backend = audio_backend::xaudio;
#elif RB_AUDIO_BACKEND_OPENAL
    audio_backend = audio_backend::openal;
#endif

}

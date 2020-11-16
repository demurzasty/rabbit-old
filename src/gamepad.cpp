#include <rabbit/gamepad.hpp>

#if RB_GAMEPAD_BACKEND_XINPUT
#include "xinput/gamepad_xinput.hpp"
#endif

using namespace rb;

std::shared_ptr<gamepad> rb::make_gamepad(const config& config) {
#if RB_GAMEPAD_BACKEND_XINPUT
    if (config.gamepad_backend == gamepad_backend::xinput) {
        return std::make_shared<gamepad_xinput>();
    }
#endif

    // todo: provide dummy 
    return nullptr;
}

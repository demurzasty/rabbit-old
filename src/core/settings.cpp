#include <rabbit/core/settings.hpp>

using namespace rb;

version settings::app_version{ 1, 0, 0 };
std::string settings::window_title{ "RabBit" };
vec2u settings::window_size{ 1440, 810 };
bool settings::fullscreen{ false };
graphics_backend settings::graphics_backend{ graphics_backend::vulkan };
bool settings::vsync{ true };

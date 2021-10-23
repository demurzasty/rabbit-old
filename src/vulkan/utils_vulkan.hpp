#pragma once 

#include <rabbit/config.hpp>

#ifdef _MSC_VER
#   pragma warning(disable : 26812)
#endif

#include <volk.h>

#ifdef _DEBUG
#	define RB_CHECK_VULKAN(expr, msg, ...) do { \
			[[maybe_unused]] const auto result = (expr); \
			RB_ASSERT(result == VK_SUCCESS, msg, __VA_ARGS__); \
		} while (0)
#else
#	define RB_CHECK_VULKAN(expr, ...) (expr)
#endif

#define RB_VK(expr, msg, ...) RB_CHECK_VULKAN(expr, msg, __VA_ARGS__)

namespace rb {
	struct utils_vulkan {
		static VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool command_pool);

		static void end_single_time_commands(VkDevice device, VkQueue graphics_queue, VkCommandPool command_pool, VkCommandBuffer command_buffer);
	};
}

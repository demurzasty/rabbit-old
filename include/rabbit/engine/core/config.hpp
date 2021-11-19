#pragma once 

#include <rabbit/engine/core/format.hpp>

#ifdef _MSC_VER
#	define RB_DEBUG_BREAK() __debugbreak()
#else
#	define RB_DEBUG_BREAK() __builtin_trap()
#endif

#ifdef _DEBUG
#	define RB_ASSERT(expr, msg, ...) do { \
			if (!(expr)) { \
				rb::print(msg, __VA_ARGS__); \
				RB_DEBUG_BREAK(); \
			} \
		} while (0)
#else
#	define RB_ASSERT(...) ((void)0)
#endif

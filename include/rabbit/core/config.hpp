#pragma once 

#include "format.hpp"

#ifdef _MSC_VER
#	define RB_NOVTABLE __declspec(novtable)
#else
#	define RB_NOVTABLE 
#endif

#ifdef _MSC_VER
#	define RB_DEBUG_BREAK() __debugbreak()
#else
#	define RB_DEBUG_BREAK() __builtin_trap()
#endif

#ifdef _DEBUG
#	define RB_DEBUG_LOG(msg, ...) rb::println(msg, __VA_ARGS__)
#else
#	define RB_DEBUG_LOG(...) ((void)0)
#endif

#ifdef _DEBUG
#	define RB_ASSERT(expr, msg, ...) do { \
			if (!(expr)) { \
				RB_DEBUG_LOG(msg, __VA_ARGS__); \
				RB_DEBUG_BREAK(); \
			} \
		} while (0)
#else
#	define RB_ASSERT(...) ((void)0)
#endif

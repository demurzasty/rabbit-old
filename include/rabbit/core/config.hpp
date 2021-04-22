#pragma once 

#ifndef RB_LIKELY
#   define RB_LIKELY [[likely]]
#endif

#ifndef RB_NOEXCEPT
#   define RB_NOEXCEPT noexcept
#endif

#ifndef RB_MAYBE_UNUSED
#   define RB_MAYBE_UNUSED [[maybe_unused]]
#endif

#ifndef RB_NODISCARD
#   define RB_NODISCARD [[nodiscard]]
#endif

#ifndef RB_ASSERT
#   include <cassert>
#   define RB_ASSERT(cond, msg) assert((cond) && msg)
#endif

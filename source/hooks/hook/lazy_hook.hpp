// ============================================================================

/// @brief Lazy hooking macros

#ifndef RAYBENCH_HOOK_LAZY_HOOK_MACROS_H_
#define RAYBENCH_HOOK_LAZY_HOOK_MACROS_H_

#define RAYBENCH_LAZY_INIT \
    static std::atomic<bool> is_hooked = false; \
    std::mutex hook_mutex; \
    bool LazyHook (void**)

#define RAYBENCH_LAZY_HOOK(cond, prefix, parameter) \
    do { \
        if (prefix::is_hooked.load(std::memory_order_acquire) == false) { \
            std::scoped_lock lock(prefix::hook_mutex); \
            if (cond && prefix::is_hooked.load(std::memory_order_relaxed) == false) { \
                if (prefix::LazyHook(reinterpret_cast<void**>(parameter)) == true) { \
                    prefix::is_hooked.store(true, std::memory_order_release); \
                } \
            } \
        } \
    } while (0)

#endif

// ----------------------------------------------------------------------------
#ifndef LLAMA_COMPAT_H
#define LLAMA_COMPAT_H

#include <stddef.h>
#include <sys/mman.h>

// Android NDK headers may not define POSIX_MADV_* even when posix_madvise is available.
#if !defined(POSIX_MADV_WILLNEED) || !defined(POSIX_MADV_RANDOM)
#define LLAMA_NEEDS_POSIX_MADVISE 1
#endif

#ifndef POSIX_MADV_WILLNEED
#if defined(MADV_WILLNEED)
#define POSIX_MADV_WILLNEED MADV_WILLNEED
#else
#define POSIX_MADV_WILLNEED 3
#endif

#if defined(__ANDROID__) && defined(LLAMA_NEEDS_POSIX_MADVISE)
static inline int llama_posix_madvise(void *addr, size_t len, int advice) {
    return madvise(addr, len, advice);
}
#ifndef posix_madvise
#define posix_madvise llama_posix_madvise
#endif
#endif
#endif

#ifndef POSIX_MADV_RANDOM
#if defined(MADV_RANDOM)
#define POSIX_MADV_RANDOM MADV_RANDOM
#else
#define POSIX_MADV_RANDOM 1
#endif
#endif

#endif

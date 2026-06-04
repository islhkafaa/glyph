#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#if defined(__linux__) && (defined(__arm__) || defined(__aarch64__))
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

struct CpuFeatures {
    bool has_avx2 = false;
    bool has_avx512f = false;
    bool has_neon = false;
};

inline CpuFeatures probe_cpu_features() {
    CpuFeatures features;
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(_MSC_VER)
    int info[4];
    __cpuid(info, 0);
    int ids = info[0];
    if (ids >= 7) {
        __cpuidex(info, 7, 0);
        features.has_avx2 = (info[1] & (1 << 5)) != 0;
        features.has_avx512f = (info[1] & (1 << 16)) != 0;
    }
#else
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (__get_cpuid_max(0, nullptr) >= 7) {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        features.has_avx2 = (ebx & (1 << 5)) != 0;
        features.has_avx512f = (ebx & (1 << 16)) != 0;
    }
#endif
#elif defined(__aarch64__) || defined(_M_ARM64)
    features.has_neon = true;
#elif defined(__arm__) || defined(_M_ARM)
#if defined(__linux__)
    unsigned long hwcap = getauxval(AT_HWCAP);
#if defined(HWCAP_NEON)
    features.has_neon = (hwcap & HWCAP_NEON) != 0;
#elif defined(HWCAP_ASIMD)
    features.has_neon = (hwcap & HWCAP_ASIMD) != 0;
#endif
#elif defined(__APPLE__)
    features.has_neon = true;
#endif
#endif
    return features;
}

#include "distance/dispatch.hpp"

#include <mutex>

#include "cpu_features.hpp"

extern float scalar_cosine(std::span<const float> a, std::span<const float> b);
extern float scalar_l2(std::span<const float> a, std::span<const float> b);
extern float scalar_dot(std::span<const float> a, std::span<const float> b);
extern float scalar_l1(std::span<const float> a, std::span<const float> b);
extern float scalar_hamming(std::span<const float> a, std::span<const float> b);
extern float scalar_l2_sq(std::span<const float> a, std::span<const float> b);
extern float scalar_sq8_cosine(std::span<const float> query, std::span<const uint8_t> code,
                               std::span<const float> min_vals, std::span<const float> scales);
extern float scalar_sq8_l2(std::span<const float> query, std::span<const uint8_t> code,
                           std::span<const float> min_vals, std::span<const float> scales);
extern float scalar_sq8_dot(std::span<const float> query, std::span<const uint8_t> code,
                            std::span<const float> min_vals, std::span<const float> scales);
extern float scalar_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                           std::span<const float> min_vals, std::span<const float> scales);
extern float scalar_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                                std::span<const float> min_vals, std::span<const float> scales);

#ifdef __AVX2__
extern float avx2_cosine(std::span<const float> a, std::span<const float> b);
extern float avx2_l2(std::span<const float> a, std::span<const float> b);
extern float avx2_dot(std::span<const float> a, std::span<const float> b);
extern float avx2_l1(std::span<const float> a, std::span<const float> b);
extern float avx2_hamming(std::span<const float> a, std::span<const float> b);
extern float avx2_l2_sq(std::span<const float> a, std::span<const float> b);
extern float avx2_sq8_cosine(std::span<const float> query, std::span<const uint8_t> code,
                             std::span<const float> min_vals, std::span<const float> scales);
extern float avx2_sq8_l2(std::span<const float> query, std::span<const uint8_t> code,
                         std::span<const float> min_vals, std::span<const float> scales);
extern float avx2_sq8_dot(std::span<const float> query, std::span<const uint8_t> code,
                          std::span<const float> min_vals, std::span<const float> scales);
extern float avx2_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                         std::span<const float> min_vals, std::span<const float> scales);
extern float avx2_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                              std::span<const float> min_vals, std::span<const float> scales);
#endif

#ifdef __ARM_NEON
extern float neon_cosine(std::span<const float> a, std::span<const float> b);
extern float neon_l2(std::span<const float> a, std::span<const float> b);
extern float neon_dot(std::span<const float> a, std::span<const float> b);
extern float neon_l1(std::span<const float> a, std::span<const float> b);
extern float neon_hamming(std::span<const float> a, std::span<const float> b);
extern float neon_l2_sq(std::span<const float> a, std::span<const float> b);
extern float neon_sq8_cosine(std::span<const float> query, std::span<const uint8_t> code,
                             std::span<const float> min_vals, std::span<const float> scales);
extern float neon_sq8_l2(std::span<const float> query, std::span<const uint8_t> code,
                         std::span<const float> min_vals, std::span<const float> scales);
extern float neon_sq8_dot(std::span<const float> query, std::span<const uint8_t> code,
                          std::span<const float> min_vals, std::span<const float> scales);
extern float neon_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                         std::span<const float> min_vals, std::span<const float> scales);
extern float neon_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                              std::span<const float> min_vals, std::span<const float> scales);
#endif

static DispatchTable g_dispatch_table;
static Sq8DispatchTable g_sq8_dispatch_table;
static std::once_flag g_init_flag;

void build_dispatch_table() {
    std::call_once(g_init_flag, []() {
        CpuFeatures features = probe_cpu_features();
        if (features.has_avx2) {
#ifdef __AVX2__
            g_dispatch_table.cosine = avx2_cosine;
            g_dispatch_table.l2 = avx2_l2;
            g_dispatch_table.dot_product = avx2_dot;
            g_dispatch_table.l1 = avx2_l1;
            g_dispatch_table.hamming = avx2_hamming;
            g_dispatch_table.l2_sq = avx2_l2_sq;

            g_sq8_dispatch_table.cosine = avx2_sq8_cosine;
            g_sq8_dispatch_table.l2 = avx2_sq8_l2;
            g_sq8_dispatch_table.dot_product = avx2_sq8_dot;
            g_sq8_dispatch_table.l1 = avx2_sq8_l1;
            g_sq8_dispatch_table.hamming = avx2_sq8_hamming;
#else
            g_dispatch_table.cosine = scalar_cosine;
            g_dispatch_table.l2 = scalar_l2;
            g_dispatch_table.dot_product = scalar_dot;
            g_dispatch_table.l1 = scalar_l1;
            g_dispatch_table.hamming = scalar_hamming;
            g_dispatch_table.l2_sq = scalar_l2_sq;

            g_sq8_dispatch_table.cosine = scalar_sq8_cosine;
            g_sq8_dispatch_table.l2 = scalar_sq8_l2;
            g_sq8_dispatch_table.dot_product = scalar_sq8_dot;
            g_sq8_dispatch_table.l1 = scalar_sq8_l1;
            g_sq8_dispatch_table.hamming = scalar_sq8_hamming;
#endif
        } else if (features.has_neon) {
#ifdef __ARM_NEON
            g_dispatch_table.cosine = neon_cosine;
            g_dispatch_table.l2 = neon_l2;
            g_dispatch_table.dot_product = neon_dot;
            g_dispatch_table.l1 = neon_l1;
            g_dispatch_table.hamming = neon_hamming;
            g_dispatch_table.l2_sq = neon_l2_sq;

            g_sq8_dispatch_table.cosine = neon_sq8_cosine;
            g_sq8_dispatch_table.l2 = neon_sq8_l2;
            g_sq8_dispatch_table.dot_product = neon_sq8_dot;
            g_sq8_dispatch_table.l1 = neon_sq8_l1;
            g_sq8_dispatch_table.hamming = neon_sq8_hamming;
#else
            g_dispatch_table.cosine = scalar_cosine;
            g_dispatch_table.l2 = scalar_l2;
            g_dispatch_table.dot_product = scalar_dot;
            g_dispatch_table.l1 = scalar_l1;
            g_dispatch_table.hamming = scalar_hamming;
            g_dispatch_table.l2_sq = scalar_l2_sq;

            g_sq8_dispatch_table.cosine = scalar_sq8_cosine;
            g_sq8_dispatch_table.l2 = scalar_sq8_l2;
            g_sq8_dispatch_table.dot_product = scalar_sq8_dot;
            g_sq8_dispatch_table.l1 = scalar_sq8_l1;
            g_sq8_dispatch_table.hamming = scalar_sq8_hamming;
#endif
        } else {
            g_dispatch_table.cosine = scalar_cosine;
            g_dispatch_table.l2 = scalar_l2;
            g_dispatch_table.dot_product = scalar_dot;
            g_dispatch_table.l1 = scalar_l1;
            g_dispatch_table.hamming = scalar_hamming;
            g_dispatch_table.l2_sq = scalar_l2_sq;

            g_sq8_dispatch_table.cosine = scalar_sq8_cosine;
            g_sq8_dispatch_table.l2 = scalar_sq8_l2;
            g_sq8_dispatch_table.dot_product = scalar_sq8_dot;
            g_sq8_dispatch_table.l1 = scalar_sq8_l1;
            g_sq8_dispatch_table.hamming = scalar_sq8_hamming;
        }
    });
}

const DispatchTable& get_dispatch_table() {
    build_dispatch_table();
    return g_dispatch_table;
}

const Sq8DispatchTable& get_sq8_dispatch_table() {
    build_dispatch_table();
    return g_sq8_dispatch_table;
}

Sq8KernelFn get_sq8_kernel(Metric metric) {
    const auto& table = get_sq8_dispatch_table();
    switch (metric) {
        case Metric::Cosine:
            return table.cosine;
        case Metric::L2:
            return table.l2;
        case Metric::DotProduct:
            return table.dot_product;
        case Metric::L1:
            return table.l1;
        case Metric::Hamming:
            return table.hamming;
    }
    return nullptr;
}

KernelFn get_kernel(Metric metric) {
    const auto& table = get_dispatch_table();
    switch (metric) {
        case Metric::Cosine:
            return table.cosine;
        case Metric::L2:
            return table.l2;
        case Metric::DotProduct:
            return table.dot_product;
        case Metric::L1:
            return table.l1;
        case Metric::Hamming:
            return table.hamming;
    }
    return nullptr;
}

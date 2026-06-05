#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>

#ifdef __AVX2__
#include <immintrin.h>

extern float scalar_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                           std::span<const float> min_vals, std::span<const float> scales);
extern float scalar_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                                std::span<const float> min_vals, std::span<const float> scales);

float avx2_l2(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    __m256 sum_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum_vec = _mm256_add_ps(sum_vec, _mm256_mul_ps(diff, diff));
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] +
                buffer[7];
    for (; i < size; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float avx2_cosine(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    __m256 dot_vec = _mm256_setzero_ps();
    __m256 norm_a_vec = _mm256_setzero_ps();
    __m256 norm_b_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        dot_vec = _mm256_add_ps(dot_vec, _mm256_mul_ps(va, vb));
        norm_a_vec = _mm256_add_ps(norm_a_vec, _mm256_mul_ps(va, va));
        norm_b_vec = _mm256_add_ps(norm_b_vec, _mm256_mul_ps(vb, vb));
    }
    alignas(32) float dot_buf[8];
    alignas(32) float na_buf[8];
    alignas(32) float nb_buf[8];
    _mm256_storeu_ps(dot_buf, dot_vec);
    _mm256_storeu_ps(na_buf, norm_a_vec);
    _mm256_storeu_ps(nb_buf, norm_b_vec);
    float dot = dot_buf[0] + dot_buf[1] + dot_buf[2] + dot_buf[3] + dot_buf[4] + dot_buf[5] +
                dot_buf[6] + dot_buf[7];
    float norm_a = na_buf[0] + na_buf[1] + na_buf[2] + na_buf[3] + na_buf[4] + na_buf[5] +
                   na_buf[6] + na_buf[7];
    float norm_b = nb_buf[0] + nb_buf[1] + nb_buf[2] + nb_buf[3] + nb_buf[4] + nb_buf[5] +
                   nb_buf[6] + nb_buf[7];
    for (; i < size; ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    if (norm_a == 0.0f || norm_b == 0.0f) {
        return 1.0f;
    }
    return 1.0f - (dot / (std::sqrt(norm_a) * std::sqrt(norm_b)));
}

float avx2_dot(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    __m256 sum_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        sum_vec = _mm256_add_ps(sum_vec, _mm256_mul_ps(va, vb));
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] +
                buffer[7];
    for (; i < size; ++i) {
        sum += a[i] * b[i];
    }
    return -sum;
}

float avx2_l1(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    __m256 sum_vec = _mm256_setzero_ps();
    __m256 abs_mask = _mm256_set1_ps(-0.0f);
    for (; i + 7 < size; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 diff = _mm256_sub_ps(va, vb);
        __m256 abs_diff = _mm256_andnot_ps(abs_mask, diff);
        sum_vec = _mm256_add_ps(sum_vec, abs_diff);
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] +
                buffer[7];
    for (; i < size; ++i) {
        sum += std::abs(a[i] - b[i]);
    }
    return sum;
}

float avx2_hamming(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    uint32_t total_dist = 0;
    for (; i + 7 < size; i += 8) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&b[i]));
        __m256i vx = _mm256_xor_si256(va, vb);
        alignas(32) uint32_t buffer[8];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(buffer), vx);
        total_dist += std::popcount(buffer[0]) + std::popcount(buffer[1]) +
                      std::popcount(buffer[2]) + std::popcount(buffer[3]) +
                      std::popcount(buffer[4]) + std::popcount(buffer[5]) +
                      std::popcount(buffer[6]) + std::popcount(buffer[7]);
    }
    for (; i < size; ++i) {
        uint32_t ua;
        uint32_t ub;
        std::memcpy(&ua, &a[i], sizeof(float));
        std::memcpy(&ub, &b[i], sizeof(float));
        total_dist += std::popcount(ua ^ ub);
    }
    return static_cast<float>(total_dist);
}

float avx2_l2_sq(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    __m256 sum_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum_vec = _mm256_add_ps(sum_vec, _mm256_mul_ps(diff, diff));
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] +
                buffer[7];
    for (; i < size; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

float avx2_sq8_l2(std::span<const float> query, std::span<const uint8_t> code,
                  std::span<const float> min_vals, std::span<const float> scales) {
    size_t i = 0;
    size_t size = query.size();
    __m256 sum_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 q_vec = _mm256_loadu_ps(&query[i]);
        __m128i code_u8 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&code[i]));
        __m256i code_i32 = _mm256_cvtepu8_epi32(code_u8);
        __m256 code_ps = _mm256_cvtepi32_ps(code_i32);
        __m256 min_vec = _mm256_loadu_ps(&min_vals[i]);
        __m256 scale_vec = _mm256_loadu_ps(&scales[i]);
        __m256 decoded = _mm256_add_ps(min_vec, _mm256_mul_ps(code_ps, scale_vec));
        __m256 diff = _mm256_sub_ps(q_vec, decoded);
        sum_vec = _mm256_add_ps(sum_vec, _mm256_mul_ps(diff, diff));
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] +
                buffer[7];
    for (; i < size; ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        float diff = query[i] - decoded;
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float avx2_sq8_dot(std::span<const float> query, std::span<const uint8_t> code,
                   std::span<const float> min_vals, std::span<const float> scales) {
    size_t i = 0;
    size_t size = query.size();
    __m256 sum_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 q_vec = _mm256_loadu_ps(&query[i]);
        __m128i code_u8 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&code[i]));
        __m256i code_i32 = _mm256_cvtepu8_epi32(code_u8);
        __m256 code_ps = _mm256_cvtepi32_ps(code_i32);
        __m256 min_vec = _mm256_loadu_ps(&min_vals[i]);
        __m256 scale_vec = _mm256_loadu_ps(&scales[i]);
        __m256 decoded = _mm256_add_ps(min_vec, _mm256_mul_ps(code_ps, scale_vec));
        sum_vec = _mm256_add_ps(sum_vec, _mm256_mul_ps(q_vec, decoded));
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] +
                buffer[7];
    for (; i < size; ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        sum += query[i] * decoded;
    }
    return -sum;
}

float avx2_sq8_cosine(std::span<const float> query, std::span<const uint8_t> code,
                      std::span<const float> min_vals, std::span<const float> scales) {
    size_t i = 0;
    size_t size = query.size();
    __m256 dot_vec = _mm256_setzero_ps();
    __m256 norm_q_vec = _mm256_setzero_ps();
    __m256 norm_c_vec = _mm256_setzero_ps();
    for (; i + 7 < size; i += 8) {
        __m256 q_vec = _mm256_loadu_ps(&query[i]);
        __m128i code_u8 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&code[i]));
        __m256i code_i32 = _mm256_cvtepu8_epi32(code_u8);
        __m256 code_ps = _mm256_cvtepi32_ps(code_i32);
        __m256 min_vec = _mm256_loadu_ps(&min_vals[i]);
        __m256 scale_vec = _mm256_loadu_ps(&scales[i]);
        __m256 decoded = _mm256_add_ps(min_vec, _mm256_mul_ps(code_ps, scale_vec));

        dot_vec = _mm256_add_ps(dot_vec, _mm256_mul_ps(q_vec, decoded));
        norm_q_vec = _mm256_add_ps(norm_q_vec, _mm256_mul_ps(q_vec, q_vec));
        norm_c_vec = _mm256_add_ps(norm_c_vec, _mm256_mul_ps(decoded, decoded));
    }
    alignas(32) float dot_buf[8];
    alignas(32) float nq_buf[8];
    alignas(32) float nc_buf[8];
    _mm256_storeu_ps(dot_buf, dot_vec);
    _mm256_storeu_ps(nq_buf, norm_q_vec);
    _mm256_storeu_ps(nc_buf, norm_c_vec);
    float dot = dot_buf[0] + dot_buf[1] + dot_buf[2] + dot_buf[3] + dot_buf[4] + dot_buf[5] +
                dot_buf[6] + dot_buf[7];
    float norm_q = nq_buf[0] + nq_buf[1] + nq_buf[2] + nq_buf[3] + nq_buf[4] + nq_buf[5] +
                   nq_buf[6] + nq_buf[7];
    float norm_c = nc_buf[0] + nc_buf[1] + nc_buf[2] + nc_buf[3] + nc_buf[4] + nc_buf[5] +
                   nc_buf[6] + nc_buf[7];
    for (; i < size; ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        dot += query[i] * decoded;
        norm_q += query[i] * query[i];
        norm_c += decoded * decoded;
    }
    if (norm_q == 0.0f || norm_c == 0.0f) {
        return 1.0f;
    }
    return 1.0f - (dot / (std::sqrt(norm_q) * std::sqrt(norm_c)));
}

float avx2_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                  std::span<const float> min_vals, std::span<const float> scales) {
    return scalar_sq8_l1(query, code, min_vals, scales);
}

float avx2_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                       std::span<const float> min_vals, std::span<const float> scales) {
    return scalar_sq8_hamming(query, code, min_vals, scales);
}
#endif

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>

#ifdef __ARM_NEON
#include <arm_neon.h>

extern float scalar_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                           std::span<const float> min_vals, std::span<const float> scales);
extern float scalar_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                                std::span<const float> min_vals, std::span<const float> scales);

float neon_l2(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t diff = vsubq_f32(va, vb);
        sum_vec = vmlaq_f32(sum_vec, diff, diff);
    }
    alignas(16) float buffer[4];
    vst1q_f32(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    for (; i < size; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float neon_cosine(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    float32x4_t dot_vec = vdupq_n_f32(0.0f);
    float32x4_t norm_a_vec = vdupq_n_f32(0.0f);
    float32x4_t norm_b_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        dot_vec = vmlaq_f32(dot_vec, va, vb);
        norm_a_vec = vmlaq_f32(norm_a_vec, va, va);
        norm_b_vec = vmlaq_f32(norm_b_vec, vb, vb);
    }
    alignas(16) float dot_buf[4];
    alignas(16) float na_buf[4];
    alignas(16) float nb_buf[4];
    vst1q_f32(dot_buf, dot_vec);
    vst1q_f32(na_buf, norm_a_vec);
    vst1q_f32(nb_buf, norm_b_vec);
    float dot = dot_buf[0] + dot_buf[1] + dot_buf[2] + dot_buf[3];
    float norm_a = na_buf[0] + na_buf[1] + na_buf[2] + na_buf[3];
    float norm_b = nb_buf[0] + nb_buf[1] + nb_buf[2] + nb_buf[3];
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

float neon_dot(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        sum_vec = vmlaq_f32(sum_vec, va, vb);
    }
    alignas(16) float buffer[4];
    vst1q_f32(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    for (; i < size; ++i) {
        sum += a[i] * b[i];
    }
    return -sum;
}

float neon_l1(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t diff = vsubq_f32(va, vb);
        float32x4_t abs_diff = vabsq_f32(diff);
        sum_vec = vaddq_f32(sum_vec, abs_diff);
    }
    alignas(16) float buffer[4];
    vst1q_f32(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    for (; i < size; ++i) {
        sum += std::abs(a[i] - b[i]);
    }
    return sum;
}

float neon_hamming(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    uint32_t total_dist = 0;
    for (; i + 3 < size; i += 4) {
        uint32x4_t va = vreinterpretq_u32_f32(vld1q_f32(&a[i]));
        uint32x4_t vb = vreinterpretq_u32_f32(vld1q_f32(&b[i]));
        uint32x4_t vx = veorq_u32(va, vb);
        alignas(16) uint32_t buffer[4];
        vst1q_u32(buffer, vx);
        total_dist += std::popcount(buffer[0]) + std::popcount(buffer[1]) +
                      std::popcount(buffer[2]) + std::popcount(buffer[3]);
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

float neon_l2_sq(std::span<const float> a, std::span<const float> b) {
    size_t i = 0;
    size_t size = a.size();
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t diff = vsubq_f32(va, vb);
        sum_vec = vmlaq_f32(sum_vec, diff, diff);
    }
    alignas(16) float buffer[4];
    vst1q_f32(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    for (; i < size; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

float neon_sq8_l2(std::span<const float> query, std::span<const uint8_t> code,
                  std::span<const float> min_vals, std::span<const float> scales) {
    size_t i = 0;
    size_t size = query.size();
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t q_vec = vld1q_f32(&query[i]);

        uint32_t temp;
        std::memcpy(&temp, &code[i], sizeof(temp));
        uint8x8_t code_u8 = vreinterpret_u8_u32(vdup_n_u32(temp));
        uint16x8_t code_u16 = vmovl_u8(code_u8);
        uint32x4_t code_u32 = vmovl_u16(vget_low_u16(code_u16));
        float32x4_t code_ps = vcvtq_f32_u32(code_u32);

        float32x4_t min_vec = vld1q_f32(&min_vals[i]);
        float32x4_t scale_vec = vld1q_f32(&scales[i]);

        float32x4_t decoded = vmlaq_f32(min_vec, code_ps, scale_vec);
        float32x4_t diff = vsubq_f32(q_vec, decoded);
        sum_vec = vmlaq_f32(sum_vec, diff, diff);
    }
    alignas(16) float buffer[4];
    vst1q_f32(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    for (; i < size; ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        float diff = query[i] - decoded;
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float neon_sq8_dot(std::span<const float> query, std::span<const uint8_t> code,
                   std::span<const float> min_vals, std::span<const float> scales) {
    size_t i = 0;
    size_t size = query.size();
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t q_vec = vld1q_f32(&query[i]);

        uint32_t temp;
        std::memcpy(&temp, &code[i], sizeof(temp));
        uint8x8_t code_u8 = vreinterpret_u8_u32(vdup_n_u32(temp));
        uint16x8_t code_u16 = vmovl_u8(code_u8);
        uint32x4_t code_u32 = vmovl_u16(vget_low_u16(code_u16));
        float32x4_t code_ps = vcvtq_f32_u32(code_u32);

        float32x4_t min_vec = vld1q_f32(&min_vals[i]);
        float32x4_t scale_vec = vld1q_f32(&scales[i]);

        float32x4_t decoded = vmlaq_f32(min_vec, code_ps, scale_vec);
        sum_vec = vmlaq_f32(sum_vec, q_vec, decoded);
    }
    alignas(16) float buffer[4];
    vst1q_f32(buffer, sum_vec);
    float sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    for (; i < size; ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        sum += query[i] * decoded;
    }
    return -sum;
}

float neon_sq8_cosine(std::span<const float> query, std::span<const uint8_t> code,
                      std::span<const float> min_vals, std::span<const float> scales) {
    size_t i = 0;
    size_t size = query.size();
    float32x4_t dot_vec = vdupq_n_f32(0.0f);
    float32x4_t norm_q_vec = vdupq_n_f32(0.0f);
    float32x4_t norm_c_vec = vdupq_n_f32(0.0f);
    for (; i + 3 < size; i += 4) {
        float32x4_t q_vec = vld1q_f32(&query[i]);

        uint32_t temp;
        std::memcpy(&temp, &code[i], sizeof(temp));
        uint8x8_t code_u8 = vreinterpret_u8_u32(vdup_n_u32(temp));
        uint16x8_t code_u16 = vmovl_u8(code_u8);
        uint32x4_t code_u32 = vmovl_u16(vget_low_u16(code_u16));
        float32x4_t code_ps = vcvtq_f32_u32(code_u32);

        float32x4_t min_vec = vld1q_f32(&min_vals[i]);
        float32x4_t scale_vec = vld1q_f32(&scales[i]);

        float32x4_t decoded = vmlaq_f32(min_vec, code_ps, scale_vec);

        dot_vec = vmlaq_f32(dot_vec, q_vec, decoded);
        norm_q_vec = vmlaq_f32(norm_q_vec, q_vec, q_vec);
        norm_c_vec = vmlaq_f32(norm_c_vec, decoded, decoded);
    }
    alignas(16) float dot_buf[4];
    alignas(16) float nq_buf[4];
    alignas(16) float nc_buf[4];
    vst1q_f32(dot_buf, dot_vec);
    vst1q_f32(nq_buf, norm_q_vec);
    vst1q_f32(nc_buf, norm_c_vec);
    float dot = dot_buf[0] + dot_buf[1] + dot_buf[2] + dot_buf[3];
    float norm_q = nq_buf[0] + nq_buf[1] + nq_buf[2] + nq_buf[3];
    float norm_c = nc_buf[0] + nc_buf[1] + nc_buf[2] + nc_buf[3];
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

float neon_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                  std::span<const float> min_vals, std::span<const float> scales) {
    return scalar_sq8_l1(query, code, min_vals, scales);
}

float neon_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                       std::span<const float> min_vals, std::span<const float> scales) {
    return scalar_sq8_hamming(query, code, min_vals, scales);
}
#endif

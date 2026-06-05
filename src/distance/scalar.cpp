#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>

float scalar_l2(std::span<const float> a, std::span<const float> b) {
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float scalar_cosine(std::span<const float> a, std::span<const float> b) {
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    if (norm_a == 0.0f || norm_b == 0.0f) {
        return 1.0f;
    }
    return 1.0f - (dot / (std::sqrt(norm_a) * std::sqrt(norm_b)));
}

float scalar_dot(std::span<const float> a, std::span<const float> b) {
    float dot = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
    }
    return -dot;
}

float scalar_l1(std::span<const float> a, std::span<const float> b) {
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        sum += std::abs(a[i] - b[i]);
    }
    return sum;
}

float scalar_hamming(std::span<const float> a, std::span<const float> b) {
    uint32_t dist = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        uint32_t ua;
        uint32_t ub;
        std::memcpy(&ua, &a[i], sizeof(float));
        std::memcpy(&ub, &b[i], sizeof(float));
        dist += std::popcount(ua ^ ub);
    }
    return static_cast<float>(dist);
}

float scalar_l2_sq(std::span<const float> a, std::span<const float> b) {
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

float scalar_sq8_l2(std::span<const float> query, std::span<const uint8_t> code,
                    std::span<const float> min_vals, std::span<const float> scales) {
    float sum = 0.0f;
    for (size_t i = 0; i < query.size(); ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        float diff = query[i] - decoded;
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float scalar_sq8_dot(std::span<const float> query, std::span<const uint8_t> code,
                     std::span<const float> min_vals, std::span<const float> scales) {
    float sum = 0.0f;
    for (size_t i = 0; i < query.size(); ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        sum += query[i] * decoded;
    }
    return -sum;
}

float scalar_sq8_cosine(std::span<const float> query, std::span<const uint8_t> code,
                        std::span<const float> min_vals, std::span<const float> scales) {
    float dot = 0.0f;
    float norm_q = 0.0f;
    float norm_c = 0.0f;
    for (size_t i = 0; i < query.size(); ++i) {
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

float scalar_sq8_l1(std::span<const float> query, std::span<const uint8_t> code,
                    std::span<const float> min_vals, std::span<const float> scales) {
    float sum = 0.0f;
    for (size_t i = 0; i < query.size(); ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        sum += std::abs(query[i] - decoded);
    }
    return sum;
}

float scalar_sq8_hamming(std::span<const float> query, std::span<const uint8_t> code,
                         std::span<const float> min_vals, std::span<const float> scales) {
    uint32_t dist = 0;
    for (size_t i = 0; i < query.size(); ++i) {
        float decoded = min_vals[i] + code[i] * scales[i];
        uint32_t ua;
        uint32_t ub;
        std::memcpy(&ua, &query[i], sizeof(float));
        std::memcpy(&ub, &decoded, sizeof(float));
        dist += std::popcount(ua ^ ub);
    }
    return static_cast<float>(dist);
}

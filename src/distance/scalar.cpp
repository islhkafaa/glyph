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

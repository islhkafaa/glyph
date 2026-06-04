#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

extern float scalar_cosine(std::span<const float> a, std::span<const float> b);
extern float scalar_l2(std::span<const float> a, std::span<const float> b);
extern float scalar_dot(std::span<const float> a, std::span<const float> b);
extern float scalar_l1(std::span<const float> a, std::span<const float> b);
extern float scalar_hamming(std::span<const float> a, std::span<const float> b);

TEST(DistanceScalarTest, IdenticalVectorsL2IsZero) {
    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {1.0f, 2.0f, 3.0f};
    EXPECT_FLOAT_EQ(scalar_l2(a, b), 0.0f);
}

TEST(DistanceScalarTest, L2CalculatesCorrectly) {
    std::vector<float> a = {1.0f, 2.0f};
    std::vector<float> b = {4.0f, 6.0f};
    EXPECT_FLOAT_EQ(scalar_l2(a, b), 5.0f);
}

TEST(DistanceScalarTest, CosineOrthogonalIsOne) {
    std::vector<float> a = {1.0f, 0.0f};
    std::vector<float> b = {0.0f, 1.0f};
    EXPECT_FLOAT_EQ(scalar_cosine(a, b), 1.0f);
}

TEST(DistanceScalarTest, CosineIdenticalIsZero) {
    std::vector<float> a = {2.0f, 4.0f};
    std::vector<float> b = {2.0f, 4.0f};
    EXPECT_NEAR(scalar_cosine(a, b), 0.0f, 1e-6f);
}

TEST(DistanceScalarTest, CosineZeroVectorReturnsOne) {
    std::vector<float> a = {0.0f, 0.0f};
    std::vector<float> b = {1.0f, 2.0f};
    EXPECT_FLOAT_EQ(scalar_cosine(a, b), 1.0f);
}

TEST(DistanceScalarTest, DotProductNegative) {
    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};
    EXPECT_FLOAT_EQ(scalar_dot(a, b), -32.0f);
}

TEST(DistanceScalarTest, L1CalculatesCorrectly) {
    std::vector<float> a = {1.0f, 5.0f};
    std::vector<float> b = {3.0f, 2.0f};
    EXPECT_FLOAT_EQ(scalar_l1(a, b), 5.0f);
}

TEST(DistanceScalarTest, HammingDifferingBits) {
    float fa = 0.0f;
    float fb = 0.0f;
    uint32_t ua = 0b10101010;
    uint32_t ub = 0b01010101;
    std::memcpy(&fa, &ua, sizeof(float));
    std::memcpy(&fb, &ub, sizeof(float));
    std::vector<float> a = {fa};
    std::vector<float> b = {fb};
    EXPECT_FLOAT_EQ(scalar_hamming(a, b), 8.0f);
}

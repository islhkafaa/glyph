#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "distance/kernel.hpp"

extern float scalar_cosine(std::span<const float> a, std::span<const float> b);
extern float scalar_l2(std::span<const float> a, std::span<const float> b);
extern float scalar_dot(std::span<const float> a, std::span<const float> b);
extern float scalar_l1(std::span<const float> a, std::span<const float> b);
extern float scalar_hamming(std::span<const float> a, std::span<const float> b);

TEST(DistanceDispatchTest, KernelsAreNotNull) {
    EXPECT_NE(get_kernel(Metric::Cosine), nullptr);
    EXPECT_NE(get_kernel(Metric::L2), nullptr);
    EXPECT_NE(get_kernel(Metric::DotProduct), nullptr);
    EXPECT_NE(get_kernel(Metric::L1), nullptr);
    EXPECT_NE(get_kernel(Metric::Hamming), nullptr);
}

class DistanceDispatchCompareTest : public ::testing::TestWithParam<int> {};

TEST_P(DistanceDispatchCompareTest, MatchScalarReference) {
    int dim = GetParam();
    std::vector<float> a(dim);
    std::vector<float> b(dim);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);

    for (int i = 0; i < dim; ++i) {
        a[i] = dist(gen);
        b[i] = dist(gen);
    }

    auto kernel_l2 = get_kernel(Metric::L2);
    EXPECT_NEAR(kernel_l2(a, b), scalar_l2(a, b), 1e-4f);

    auto kernel_cosine = get_kernel(Metric::Cosine);
    EXPECT_NEAR(kernel_cosine(a, b), scalar_cosine(a, b), 1e-4f);

    auto kernel_dot = get_kernel(Metric::DotProduct);
    EXPECT_NEAR(kernel_dot(a, b), scalar_dot(a, b), 1e-4f);

    auto kernel_l1 = get_kernel(Metric::L1);
    EXPECT_NEAR(kernel_l1(a, b), scalar_l1(a, b), 1e-4f);

    auto kernel_hamming = get_kernel(Metric::Hamming);
    EXPECT_NEAR(kernel_hamming(a, b), scalar_hamming(a, b), 1e-4f);
}

INSTANTIATE_TEST_SUITE_P(VectorDimensions, DistanceDispatchCompareTest,
                         ::testing::Values(3, 4, 8, 11, 16, 31, 32, 64));

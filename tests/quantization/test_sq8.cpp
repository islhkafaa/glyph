#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "distance/dispatch.hpp"
#include "quantization/sq8.hpp"

TEST(Sq8Test, TrainAndEncodeDecode) {
    Sq8Codebook cb;
    std::vector<std::vector<float>> vectors = {
        {0.0f, 10.0f, -5.0f}, {1.0f, 5.0f, 0.0f}, {2.0f, 0.0f, 5.0f}};
    cb.train(vectors);

    EXPECT_EQ(cb.dim, 3);
    EXPECT_NEAR(cb.min_vals[0], 0.0f, 1e-5f);
    EXPECT_NEAR(cb.min_vals[1], 0.0f, 1e-5f);
    EXPECT_NEAR(cb.min_vals[2], -5.0f, 1e-5f);

    std::vector<float> test_vec = {1.0f, 5.0f, 0.0f};
    auto code = cb.encode(test_vec);
    EXPECT_EQ(code.size(), 3);

    auto decoded = cb.decode(code);
    EXPECT_NEAR(decoded[0], test_vec[0], 0.1f);
    EXPECT_NEAR(decoded[1], test_vec[1], 0.1f);
    EXPECT_NEAR(decoded[2], test_vec[2], 0.1f);
}

TEST(Sq8Test, DistancePreservation) {
    Sq8Codebook cb;
    std::vector<std::vector<float>> vectors = {{0.0f, 0.0f}, {10.0f, 10.0f}, {-10.0f, -10.0f}};
    cb.train(vectors);

    std::vector<float> query = {1.0f, 1.0f};

    auto code1 = cb.encode(vectors[0]);
    auto code2 = cb.encode(vectors[1]);
    auto code3 = cb.encode(vectors[2]);

    float d1 = cb.distance(query, code1, Metric::L2);
    float d2 = cb.distance(query, code2, Metric::L2);
    float d3 = cb.distance(query, code3, Metric::L2);

    EXPECT_LT(d1, d2);
    EXPECT_LT(d1, d3);
}

TEST(Sq8Test, SimdDistanceComparison) {
    Sq8Codebook cb;
    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < 100; ++i) {
        vectors.push_back({static_cast<float>(i), 2.0f * i, -3.0f * i, 0.5f * i});
    }
    cb.train(vectors);

    std::vector<float> query = {10.0f, 20.0f, -30.0f, 5.0f};
    auto code = cb.encode(vectors[50]);

    float d_l2_cb = cb.distance(query, code, Metric::L2);
    std::vector<float> decoded = cb.decode(code);
    float d_l2_ref = get_kernel(Metric::L2)(query, decoded);
    EXPECT_NEAR(d_l2_cb, d_l2_ref, 1e-4f);

    float d_dot_cb = cb.distance(query, code, Metric::DotProduct);
    float d_dot_ref = get_kernel(Metric::DotProduct)(query, decoded);
    EXPECT_NEAR(d_dot_cb, d_dot_ref, 1e-4f);

    float d_cos_cb = cb.distance(query, code, Metric::Cosine);
    float d_cos_ref = get_kernel(Metric::Cosine)(query, decoded);
    EXPECT_NEAR(d_cos_cb, d_cos_ref, 1e-4f);
}

TEST(Sq8Test, Serialize) {
    Sq8Codebook cb;
    std::vector<std::vector<float>> vectors = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    cb.train(vectors);

    std::stringstream ss;
    cb.serialize(ss);

    Sq8Codebook cb2 = Sq8Codebook::deserialize(ss);
    EXPECT_EQ(cb2.dim, cb.dim);
    EXPECT_EQ(cb2.min_vals, cb.min_vals);
    EXPECT_EQ(cb2.scales, cb.scales);
}

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

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
    auto kernel = get_kernel(Metric::L2);

    auto code1 = cb.encode(vectors[0]);
    auto code2 = cb.encode(vectors[1]);
    auto code3 = cb.encode(vectors[2]);

    float d1 = cb.distance(query, code1, kernel);
    float d2 = cb.distance(query, code2, kernel);
    float d3 = cb.distance(query, code3, kernel);

    EXPECT_LT(d1, d2);
    EXPECT_LT(d1, d3);
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

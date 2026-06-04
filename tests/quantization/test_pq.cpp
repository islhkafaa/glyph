#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "quantization/pq.hpp"

TEST(PqTest, TrainAndEncodeDecode) {
    PqCodebook cb;
    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < 300; ++i) {
        vectors.push_back({static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(-i),
                           static_cast<float>(i + 5)});
    }
    cb.train(vectors, 2);

    EXPECT_EQ(cb.dim, 4);
    EXPECT_EQ(cb.M_pq, 2);
    EXPECT_EQ(cb.ksub, 256);

    std::vector<float> test_vec = {10.0f, 20.0f, -10.0f, 15.0f};
    auto code = cb.encode(test_vec);
    EXPECT_EQ(code.size(), 2);

    auto decoded = cb.decode(code);
    EXPECT_EQ(decoded.size(), 4);
}

TEST(PqTest, AdcDistanceRanking) {
    PqCodebook cb;
    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < 300; ++i) {
        vectors.push_back({static_cast<float>(i), static_cast<float>(-i)});
    }
    cb.train(vectors, 1);

    std::vector<float> query = {5.0f, -5.0f};
    PqLookup lookup = cb.build_lookup(query, Metric::L2);
    auto kernel = get_kernel(Metric::L2);

    auto code_close = cb.encode(vectors[5]);
    auto code_far = cb.encode(vectors[290]);

    float d_close = cb.adc_distance(lookup, code_close, kernel, query);
    float d_far = cb.adc_distance(lookup, code_far, kernel, query);

    EXPECT_LT(d_close, d_far);
}

TEST(PqTest, Serialize) {
    PqCodebook cb;
    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < 10; ++i) {
        vectors.push_back({static_cast<float>(i), static_cast<float>(i)});
    }
    cb.train(vectors, 1);

    std::stringstream ss;
    cb.serialize(ss);

    PqCodebook cb2 = PqCodebook::deserialize(ss);
    EXPECT_EQ(cb2.dim, cb.dim);
    EXPECT_EQ(cb2.M_pq, cb.M_pq);
    EXPECT_EQ(cb2.ksub, cb.ksub);
    EXPECT_EQ(cb2.centroids, cb.centroids);
}

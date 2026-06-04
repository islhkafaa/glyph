#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "hnsw/graph.hpp"

TEST(GraphQuantizedTest, Sq8SearchRecall) {
    HnswConfig config;
    config.dim = 4;
    config.quant_mode = QuantizationMode::SQ8;
    HnswGraph graph(config);

    for (std::uint64_t i = 0; i < 100; ++i) {
        std::vector<float> vec = {static_cast<float>(i), 0.0f, 0.0f, 0.0f};
        graph.upsert(i, vec);
    }

    graph.train_quantization();
    EXPECT_TRUE(graph.quant_trained());

    std::vector<float> query = {50.0f, 0.0f, 0.0f, 0.0f};
    auto results = graph.search(query, 1, 10);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 50);
}

TEST(GraphQuantizedTest, PqSearchRecall) {
    HnswConfig config;
    config.dim = 4;
    config.quant_mode = QuantizationMode::PQ;
    config.pq_m = 2;
    HnswGraph graph(config);

    for (std::uint64_t i = 0; i < 300; ++i) {
        std::vector<float> vec = {static_cast<float>(i) * 100.0f, 0.0f,
                                  -static_cast<float>(i) * 100.0f, 0.0f};
        graph.upsert(i, vec);
    }

    graph.train_quantization();
    EXPECT_TRUE(graph.quant_trained());

    std::vector<float> query = {150.0f * 100.0f, 0.0f, -150.0f * 100.0f, 0.0f};
    auto results = graph.search(query, 1, 50);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 150);
}

TEST(GraphQuantizedTest, QuantizedSerializeRoundtrip) {
    HnswConfig config;
    config.dim = 4;
    config.quant_mode = QuantizationMode::SQ8;
    HnswGraph graph(config);

    for (std::uint64_t i = 0; i < 100; ++i) {
        std::vector<float> vec = {static_cast<float>(i), 1.0f, 2.0f, 3.0f};
        graph.upsert(i, vec);
    }

    graph.train_quantization();

    std::stringstream ss;
    graph.serialize(ss);

    HnswGraph graph2 = HnswGraph::deserialize(ss);
    EXPECT_TRUE(graph2.quant_trained());
    EXPECT_EQ(graph2.size(), 100);

    std::vector<float> query = {50.0f, 1.0f, 2.0f, 3.0f};
    auto res1 = graph.search(query, 5, 10);
    auto res2 = graph2.search(query, 5, 10);

    ASSERT_EQ(res1.size(), res2.size());
    for (std::size_t i = 0; i < res1.size(); ++i) {
        EXPECT_EQ(res1[i].id, res2[i].id);
        EXPECT_NEAR(res1[i].distance, res2[i].distance, 1e-4f);
    }
}

TEST(GraphQuantizedTest, UpsertAfterTraining) {
    HnswConfig config;
    config.dim = 4;
    config.quant_mode = QuantizationMode::SQ8;
    HnswGraph graph(config);

    for (std::uint64_t i = 0; i < 100; ++i) {
        std::vector<float> vec = {static_cast<float>(i), 0.0f, 0.0f, 0.0f};
        graph.upsert(i, vec);
    }

    graph.train_quantization();

    std::vector<float> new_vec = {55.5f, 0.0f, 0.0f, 0.0f};
    graph.upsert(105, new_vec);

    std::vector<float> query = {55.5f, 0.0f, 0.0f, 0.0f};
    auto results = graph.search(query, 1, 10);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 105);
}

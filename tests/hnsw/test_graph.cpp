#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_set>
#include <vector>

#include "hnsw/graph.hpp"

TEST(HnswGraphTest, InsertAndSize) {
    HnswConfig config;
    config.dim = 4;
    config.M = 8;
    config.M0 = 16;
    config.ef_construction = 50;
    config.metric = Metric::L2;

    HnswGraph graph(config);
    EXPECT_EQ(graph.size(), 0);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f, 0.0f, 0.0f};

    graph.upsert(1, v1);
    EXPECT_EQ(graph.size(), 1);

    graph.upsert(2, v2);
    EXPECT_EQ(graph.size(), 2);
}

TEST(HnswGraphTest, SearchReturnsKResults) {
    HnswConfig config;
    config.dim = 4;
    config.M = 8;
    config.M0 = 16;
    config.ef_construction = 50;
    config.metric = Metric::L2;

    HnswGraph graph(config);

    for (std::uint64_t i = 1; i <= 10; ++i) {
        std::vector<float> v = {static_cast<float>(i), 0.0f, 0.0f, 0.0f};
        graph.upsert(i, v);
    }

    std::vector<float> query = {5.2f, 0.0f, 0.0f, 0.0f};
    auto results = graph.search(query, 5, 20);

    EXPECT_EQ(results.size(), 5);
}

TEST(HnswGraphTest, SearchNearestIsCorrect) {
    HnswConfig config;
    config.dim = 4;
    config.M = 8;
    config.M0 = 16;
    config.ef_construction = 50;
    config.metric = Metric::L2;

    HnswGraph graph(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {10.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v3 = {100.0f, 0.0f, 0.0f, 0.0f};

    graph.upsert(1, v1);
    graph.upsert(2, v2);
    graph.upsert(3, v3);

    std::vector<float> query = {12.0f, 0.0f, 0.0f, 0.0f};
    auto results = graph.search(query, 1, 10);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 2);
}

TEST(HnswGraphTest, DeleteExcludesFromResults) {
    HnswConfig config;
    config.dim = 4;
    config.M = 8;
    config.M0 = 16;
    config.ef_construction = 50;
    config.metric = Metric::L2;

    HnswGraph graph(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {2.0f, 0.0f, 0.0f, 0.0f};

    graph.upsert(1, v1);
    graph.upsert(2, v2);

    graph.remove(2);
    EXPECT_EQ(graph.size(), 1);

    std::vector<float> query = {2.1f, 0.0f, 0.0f, 0.0f};
    auto results = graph.search(query, 2, 10);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 1);
}

TEST(HnswGraphTest, DuplicateUpsertReplaces) {
    HnswConfig config;
    config.dim = 4;
    config.M = 8;
    config.M0 = 16;
    config.ef_construction = 50;
    config.metric = Metric::L2;

    HnswGraph graph(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {10.0f, 0.0f, 0.0f, 0.0f};

    graph.upsert(1, v1);
    EXPECT_EQ(graph.size(), 1);

    graph.upsert(1, v2);
    EXPECT_EQ(graph.size(), 1);

    std::vector<float> query = {9.5f, 0.0f, 0.0f, 0.0f};
    auto results = graph.search(query, 1, 10);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 1);
    EXPECT_NEAR(results[0].distance, 0.5f, 1e-4f);
}

TEST(HnswGraphTest, BatchInsertStressTest) {
    const int dim = 16;
    const int num_vectors = 500;
    const int num_queries = 50;
    const int k = 10;
    const int ef = 50;

    HnswConfig config;
    config.dim = dim;
    config.M = 16;
    config.M0 = 32;
    config.ef_construction = 100;
    config.metric = Metric::L2;

    HnswGraph graph(config);

    std::mt19937 gen(1337);
    std::uniform_real_distribution<float> dist(-5.0f, 5.0f);

    std::vector<std::vector<float>> dataset(num_vectors, std::vector<float>(dim));
    for (int i = 0; i < num_vectors; ++i) {
        for (int d = 0; d < dim; ++d) {
            dataset[i][d] = dist(gen);
        }
        graph.upsert(static_cast<VectorId>(i), dataset[i]);
    }

    std::vector<std::vector<float>> queries(num_queries, std::vector<float>(dim));
    for (int i = 0; i < num_queries; ++i) {
        for (int d = 0; d < dim; ++d) {
            queries[i][d] = dist(gen);
        }
    }

    auto compute_l2 = [](std::span<const float> a, std::span<const float> b) {
        float sum = 0.0f;
        for (std::size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    };

    double total_recall = 0.0;

    for (int q_idx = 0; q_idx < num_queries; ++q_idx) {
        const auto& query = queries[q_idx];

        std::vector<std::pair<float, VectorId>> gt;
        gt.reserve(num_vectors);
        for (int i = 0; i < num_vectors; ++i) {
            float d = compute_l2(query, dataset[i]);
            gt.push_back({d, static_cast<VectorId>(i)});
        }
        std::sort(gt.begin(), gt.end());

        std::unordered_set<VectorId> gt_set;
        for (int i = 0; i < k; ++i) {
            gt_set.insert(gt[i].second);
        }

        auto search_results = graph.search(query, k, ef);

        int hits = 0;
        for (const auto& res : search_results) {
            if (gt_set.contains(res.id)) {
                hits++;
            }
        }
        total_recall += static_cast<double>(hits) / k;
    }

    double avg_recall = total_recall / num_queries;
    EXPECT_GT(avg_recall, 0.8);
}

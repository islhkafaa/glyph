#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "server/metrics.hpp"

TEST(MetricsTest, BasicCounters) {
    auto& metrics = Metrics::instance();

    metrics.upserts.store(0);
    metrics.searches.store(0);
    metrics.deletes.store(0);
    metrics.errors.store(0);
    metrics.total_requests.store(0);

    metrics.upserts.fetch_add(5);
    metrics.searches.fetch_add(10);
    metrics.deletes.fetch_add(3);
    metrics.errors.fetch_add(1);
    metrics.total_requests.fetch_add(20);

    auto snap = metrics.snapshot();
    EXPECT_EQ(snap.upserts, 5);
    EXPECT_EQ(snap.searches, 10);
    EXPECT_EQ(snap.deletes, 3);
    EXPECT_EQ(snap.errors, 1);
    EXPECT_EQ(snap.total_requests, 20);
}

TEST(MetricsTest, ConcurrentIncrements) {
    auto& metrics = Metrics::instance();

    metrics.upserts.store(0);
    metrics.searches.store(0);
    metrics.deletes.store(0);
    metrics.errors.store(0);
    metrics.total_requests.store(0);

    const int num_threads = 10;
    const int iterations = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&metrics]() {
            for (int j = 0; j < iterations; ++j) {
                metrics.upserts.fetch_add(1, std::memory_order_relaxed);
                metrics.searches.fetch_add(2, std::memory_order_relaxed);
                metrics.deletes.fetch_add(3, std::memory_order_relaxed);
                metrics.errors.fetch_add(4, std::memory_order_relaxed);
                metrics.total_requests.fetch_add(5, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto snap = metrics.snapshot();
    EXPECT_EQ(snap.upserts, num_threads * iterations);
    EXPECT_EQ(snap.searches, num_threads * iterations * 2);
    EXPECT_EQ(snap.deletes, num_threads * iterations * 3);
    EXPECT_EQ(snap.errors, num_threads * iterations * 4);
    EXPECT_EQ(snap.total_requests, num_threads * iterations * 5);
}

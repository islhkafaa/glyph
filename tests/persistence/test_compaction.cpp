#include <gtest/gtest.h>

#include <filesystem>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "persistence/engine.hpp"

class CompactionTest : public ::testing::Test {
   protected:
    std::filesystem::path temp_dir_;

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(100000000, 999999999);
        temp_dir_ = std::filesystem::temp_directory_path() /
                    ("glyph_compaction_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override { std::filesystem::remove_all(temp_dir_); }
};

TEST_F(CompactionTest, RebuildCompaction) {
    HnswConfig config;
    config.dim = 4;
    config.M = 8;
    config.metric = Metric::L2;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns1", config);

        auto& col = engine.registry().get("ns1");

        for (int i = 0; i < 2000; ++i) {
            std::vector<float> vec = {static_cast<float>(i), 0.0f, 0.0f, 0.0f};
            engine.upsert("ns1", i, vec, {});
        }

        EXPECT_EQ(col.size(), 2000);
        EXPECT_EQ(col.deleted_count(), 0);

        for (int i = 0; i < 1100; ++i) {
            engine.remove("ns1", i);
        }

        EXPECT_EQ(col.size(), 900);
        EXPECT_EQ(col.deleted_count(), 1100);

        col.compact();

        EXPECT_EQ(col.size(), 900);
        EXPECT_EQ(col.deleted_count(), 0);

        std::vector<float> query = {1500.0f, 0.0f, 0.0f, 0.0f};
        auto results = col.search(query, 1, 10);
        ASSERT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].id, 1500);

        engine.checkpoint();
    }

    {
        StorageEngine engine(temp_dir_);
        ASSERT_TRUE(engine.registry().exists("ns1"));
        auto& col = engine.registry().get("ns1");
        EXPECT_EQ(col.size(), 900);
        EXPECT_EQ(col.deleted_count(), 0);

        std::vector<float> query = {1500.0f, 0.0f, 0.0f, 0.0f};
        auto results = col.search(query, 1, 10);
        ASSERT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].id, 1500);
    }
}

#include <gtest/gtest.h>

#include <filesystem>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "persistence/engine.hpp"

class SegmentedStorageTest : public ::testing::Test {
   protected:
    std::filesystem::path temp_dir_;

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(100000000, 999999999);
        temp_dir_ = std::filesystem::temp_directory_path() /
                    ("glyph_segmented_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override { std::filesystem::remove_all(temp_dir_); }
};

TEST_F(SegmentedStorageTest, BasicSegmentedPersistenceAndCompaction) {
    HnswConfig config;
    config.dim = 3;
    config.M = 8;
    config.metric = Metric::L2;
    config.segment_max_size = 3;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns1", config);

        engine.upsert("ns1", 1, std::vector<float>{1.0f, 0.0f, 0.0f}, {});
        engine.upsert("ns1", 2, std::vector<float>{0.0f, 1.0f, 0.0f}, {});
        engine.upsert("ns1", 3, std::vector<float>{0.0f, 0.0f, 1.0f}, {});

        engine.upsert("ns1", 4, std::vector<float>{2.0f, 0.0f, 0.0f}, {});
        engine.upsert("ns1", 5, std::vector<float>{0.0f, 2.0f, 0.0f}, {});
        engine.upsert("ns1", 6, std::vector<float>{0.0f, 0.0f, 2.0f}, {});

        engine.upsert("ns1", 7, std::vector<float>{3.0f, 0.0f, 0.0f}, {});

        engine.checkpoint();
    }

    {
        StorageEngine engine(temp_dir_);
        ASSERT_TRUE(engine.registry().exists("ns1"));
        auto& col = engine.registry().get("ns1");
        EXPECT_EQ(col.size(), 7);

        EXPECT_EQ(col.segments().size(), 4);

        col.remove(2);
        col.compact();

        EXPECT_EQ(col.segments().size(), 2);

        auto results = col.search(std::vector<float>{0.0f, 1.0f, 0.0f}, 5, 10);
        for (const auto& r : results) {
            EXPECT_NE(r.id, 2);
        }
    }
}

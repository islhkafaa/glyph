#include <gtest/gtest.h>

#include <filesystem>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "persistence/engine.hpp"

class StorageEngineTest : public ::testing::Test {
   protected:
    std::filesystem::path temp_dir_;

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(100000000, 999999999);
        temp_dir_ = std::filesystem::temp_directory_path() /
                    ("glyph_engine_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override { std::filesystem::remove_all(temp_dir_); }
};

TEST_F(StorageEngineTest, UpsertSurvivesRestart) {
    HnswConfig config;
    config.dim = 3;
    config.M = 8;
    config.metric = Metric::L2;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns1", config);
        std::vector<float> vec = {1.0f, 2.0f, 3.0f};
        Metadata meta;
        meta["info"] = std::string("test");
        engine.upsert("ns1", 42, vec, meta);
    }

    {
        StorageEngine engine(temp_dir_);
        ASSERT_TRUE(engine.registry().exists("ns1"));
        auto& ns = engine.registry().get("ns1");
        EXPECT_EQ(ns.size(), 1);
        std::vector<float> query = {1.0f, 2.0f, 3.0f};
        auto results = ns.search(query, 1, 10);
        ASSERT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].id, 42);
        EXPECT_EQ(std::get<std::string>(results[0].meta.at("info")), "test");
    }
}

TEST_F(StorageEngineTest, DeleteSurvivesRestart) {
    HnswConfig config;
    config.dim = 3;
    config.M = 8;
    config.metric = Metric::L2;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns1", config);
        std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
        std::vector<float> vec2 = {0.0f, 1.0f, 0.0f};
        engine.upsert("ns1", 1, vec1, {});
        engine.upsert("ns1", 2, vec2, {});
        engine.remove("ns1", 2);
    }

    {
        StorageEngine engine(temp_dir_);
        ASSERT_TRUE(engine.registry().exists("ns1"));
        auto& ns = engine.registry().get("ns1");
        EXPECT_EQ(ns.size(), 1);
        std::vector<float> query = {0.0f, 1.0f, 0.0f};
        auto results = ns.search(query, 5, 10);
        ASSERT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].id, 1);
    }
}

TEST_F(StorageEngineTest, CheckpointAndRecover) {
    HnswConfig config;
    config.dim = 3;
    config.M = 8;
    config.metric = Metric::L2;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns1", config);
        std::vector<float> vec = {5.0f, 5.0f, 5.0f};
        engine.upsert("ns1", 100, vec, {});
        engine.checkpoint();
    }

    {
        StorageEngine engine(temp_dir_);
        ASSERT_TRUE(engine.registry().exists("ns1"));
        auto& ns = engine.registry().get("ns1");
        EXPECT_EQ(ns.size(), 1);
        std::vector<float> query = {5.0f, 5.0f, 5.0f};
        auto results = ns.search(query, 1, 10);
        ASSERT_EQ(results.size(), 1);
        EXPECT_EQ(results[0].id, 100);
    }
}

TEST_F(StorageEngineTest, NamespaceDropSurvivesRestart) {
    HnswConfig config;
    config.dim = 3;
    config.M = 8;
    config.metric = Metric::L2;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns1", config);
        engine.create_namespace("ns2", config);
        engine.drop_namespace("ns1");
    }

    {
        StorageEngine engine(temp_dir_);
        EXPECT_FALSE(engine.registry().exists("ns1"));
        EXPECT_TRUE(engine.registry().exists("ns2"));
    }
}

TEST_F(StorageEngineTest, MultiNamespaceSurvivesRestart) {
    HnswConfig config1;
    config1.dim = 2;
    config1.M = 8;
    config1.metric = Metric::L2;

    HnswConfig config2;
    config2.dim = 4;
    config2.M = 16;
    config2.metric = Metric::Cosine;

    {
        StorageEngine engine(temp_dir_);
        engine.create_namespace("ns_a", config1);
        engine.create_namespace("ns_b", config2);

        std::vector<float> vec_a = {1.0f, 2.0f};
        std::vector<float> vec_b = {3.0f, 4.0f, 5.0f, 6.0f};

        engine.upsert("ns_a", 10, vec_a, {});
        engine.upsert("ns_b", 20, vec_b, {});
    }

    {
        StorageEngine engine(temp_dir_);
        ASSERT_TRUE(engine.registry().exists("ns_a"));
        ASSERT_TRUE(engine.registry().exists("ns_b"));

        auto& ns_a = engine.registry().get("ns_a");
        auto& ns_b = engine.registry().get("ns_b");

        EXPECT_EQ(ns_a.size(), 1);
        EXPECT_EQ(ns_b.size(), 1);

        EXPECT_EQ(ns_a.config().dim, 2);
        EXPECT_EQ(ns_b.config().dim, 4);
        EXPECT_EQ(ns_b.config().metric, Metric::Cosine);
    }
}

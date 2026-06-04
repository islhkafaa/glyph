#include <gtest/gtest.h>

#include "collection/collection.hpp"

TEST(CollectionTest, UpsertAndSize) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    EXPECT_EQ(col.size(), 0);

    Metadata meta1 = {{"category", "test"}, {"valid", true}};
    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    col.upsert(101, v1, meta1);

    EXPECT_EQ(col.size(), 1);
}

TEST(CollectionTest, BatchUpsert) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    std::vector<UpsertEntry> entries;
    entries.push_back({101, {1.0f, 0.0f, 0.0f, 0.0f}, {{"tag", "A"}}});
    entries.push_back({102, {0.0f, 1.0f, 0.0f, 0.0f}, {{"tag", "B"}}});

    col.batch_upsert(entries);
    EXPECT_EQ(col.size(), 2);
}

TEST(CollectionTest, SearchReturnsMetadata) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    Metadata meta = {{"name", "item_a"}, {"score", 9.5}};
    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    col.upsert(1, v1, meta);

    std::vector<float> q = {1.1f, 0.0f, 0.0f, 0.0f};
    auto results = col.search(q, 1, 10);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 1);

    auto score_val = results[0].meta.at("score");
    EXPECT_DOUBLE_EQ(std::get<double>(score_val), 9.5);

    auto name_val = results[0].meta.at("name");
    EXPECT_EQ(std::get<std::string>(name_val), "item_a");
}

TEST(CollectionTest, FilterByStringEquality) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f, 0.0f, 0.0f};
    col.upsert(1, v1, {{"color", "red"}});
    col.upsert(2, v2, {{"color", "blue"}});

    Filter filter = EqFilter{"color", "blue"};
    std::vector<float> q = {0.5f, 0.5f, 0.0f, 0.0f};
    auto results = col.search(q, 2, 10, filter);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 2);
}

TEST(CollectionTest, FilterByNumericRange) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f, 0.0f, 0.0f};
    std::vector<float> v3 = {0.0f, 0.0f, 1.0f, 0.0f};
    col.upsert(1, v1, {{"age", 15.0}});
    col.upsert(2, v2, {{"age", 25.0}});
    col.upsert(3, v3, {{"age", 35.0}});

    Filter filter = RangeFilter{"age", 20.0, 30.0};
    std::vector<float> q = {0.3f, 0.3f, 0.3f, 0.0f};
    auto results = col.search(q, 3, 10, filter);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 2);
}

TEST(CollectionTest, RemoveClearsMetadata) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    col.upsert(1, v1, {{"tag", "A"}});
    col.remove(1);

    std::vector<float> q = {1.0f, 0.0f, 0.0f, 0.0f};
    auto results = col.search(q, 1, 10);
    EXPECT_TRUE(results.empty());
}

TEST(CollectionTest, NoFilterMatchesAll) {
    HnswConfig config;
    config.dim = 4;
    Collection col(config);

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f, 0.0f, 0.0f};
    col.upsert(1, v1, {{"tag", "A"}});
    col.upsert(2, v2, {{"tag", "B"}});

    std::vector<float> q = {0.5f, 0.5f, 0.0f, 0.0f};
    auto results = col.search(q, 2, 10);
    EXPECT_EQ(results.size(), 2);
}

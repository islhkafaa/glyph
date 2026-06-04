#include <gtest/gtest.h>

#include <algorithm>

#include "collection/registry.hpp"

TEST(RegistryTest, CreateAndExists) {
    NamespaceRegistry registry;
    HnswConfig config;
    config.dim = 4;

    EXPECT_FALSE(registry.exists("col_a"));
    registry.create("col_a", config);
    EXPECT_TRUE(registry.exists("col_a"));
}

TEST(RegistryTest, GetUnknownThrows) {
    NamespaceRegistry registry;
    EXPECT_THROW(registry.get("unknown"), std::out_of_range);
}

TEST(RegistryTest, DropRemovesNamespace) {
    NamespaceRegistry registry;
    HnswConfig config;
    config.dim = 4;

    registry.create("col_a", config);
    EXPECT_TRUE(registry.exists("col_a"));

    registry.drop("col_a");
    EXPECT_FALSE(registry.exists("col_a"));
}

TEST(RegistryTest, ListReturnsAllNames) {
    NamespaceRegistry registry;
    HnswConfig config;
    config.dim = 4;

    registry.create("col_a", config);
    registry.create("col_b", config);

    auto list = registry.list();
    EXPECT_EQ(list.size(), 2);
    EXPECT_NE(std::find(list.begin(), list.end(), "col_a"), list.end());
    EXPECT_NE(std::find(list.begin(), list.end(), "col_b"), list.end());
}

TEST(RegistryTest, IndependentIndexes) {
    NamespaceRegistry registry;
    HnswConfig config1;
    config1.dim = 4;

    HnswConfig config2;
    config2.dim = 4;

    registry.create("col_a", config1);
    registry.create("col_b", config2);

    auto& col_a = registry.get("col_a");
    auto& col_b = registry.get("col_b");

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f, 0.0f, 0.0f};
    col_a.upsert(1, v1, {});
    col_b.upsert(2, v2, {});

    EXPECT_EQ(col_a.size(), 1);
    EXPECT_EQ(col_b.size(), 1);

    std::vector<float> q = {1.0f, 0.0f, 0.0f, 0.0f};
    auto res_a = col_a.search(q, 10, 10);
    ASSERT_EQ(res_a.size(), 1);
    EXPECT_EQ(res_a[0].id, 1);

    auto res_b = col_b.search(q, 10, 10);
    ASSERT_EQ(res_b.size(), 1);
    EXPECT_EQ(res_b[0].id, 2);
}

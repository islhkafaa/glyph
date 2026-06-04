#include <gtest/gtest.h>

#include <filesystem>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "wal/log.hpp"

class WalLogTest : public ::testing::Test {
   protected:
    std::filesystem::path temp_dir_;
    std::filesystem::path wal_path_;

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(100000000, 999999999);
        temp_dir_ = std::filesystem::temp_directory_path() /
                    ("glyph_wal_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir_);
        wal_path_ = temp_dir_ / "wal.log";
    }

    void TearDown() override { std::filesystem::remove_all(temp_dir_); }
};

TEST_F(WalLogTest, AppendAndReplayUpsert) {
    {
        WalLog wal(wal_path_);
        std::vector<float> vec = {1.5f, 2.5f, -3.5f};
        Metadata meta;
        meta["key1"] = std::string("val1");
        meta["key2"] = 3.14;
        meta["key3"] = true;

        wal.append_upsert("ns1", 12345, vec, meta);
        wal.flush();
    }

    WalLog wal(wal_path_);
    auto entries = wal.read_all();
    ASSERT_EQ(entries.size(), 1);
    const auto& entry = entries[0];
    EXPECT_EQ(entry.type, WalRecordType::Upsert);
    EXPECT_EQ(entry.ns_name, "ns1");
    EXPECT_EQ(entry.id, 12345);
    EXPECT_EQ(entry.vec.size(), 3);
    EXPECT_FLOAT_EQ(entry.vec[0], 1.5f);
    EXPECT_FLOAT_EQ(entry.vec[1], 2.5f);
    EXPECT_FLOAT_EQ(entry.vec[2], -3.5f);
    EXPECT_EQ(std::get<std::string>(entry.meta.at("key1")), "val1");
    EXPECT_DOUBLE_EQ(std::get<double>(entry.meta.at("key2")), 3.14);
    EXPECT_TRUE(std::get<bool>(entry.meta.at("key3")));
}

TEST_F(WalLogTest, AppendAndReplayDelete) {
    {
        WalLog wal(wal_path_);
        wal.append_delete("ns1", 9999);
        wal.flush();
    }

    WalLog wal(wal_path_);
    auto entries = wal.read_all();
    ASSERT_EQ(entries.size(), 1);
    const auto& entry = entries[0];
    EXPECT_EQ(entry.type, WalRecordType::Delete);
    EXPECT_EQ(entry.ns_name, "ns1");
    EXPECT_EQ(entry.id, 9999);
}

TEST_F(WalLogTest, AppendAndReplayCreateNamespace) {
    {
        WalLog wal(wal_path_);
        HnswConfig config;
        config.dim = 128;
        config.M = 32;
        config.M0 = 64;
        config.ef_construction = 150;
        config.metric = Metric::Cosine;
        config.max_elements = 10000;

        wal.append_create_namespace("new_ns", config);
        wal.flush();
    }

    WalLog wal(wal_path_);
    auto entries = wal.read_all();
    ASSERT_EQ(entries.size(), 1);
    const auto& entry = entries[0];
    EXPECT_EQ(entry.type, WalRecordType::CreateNamespace);
    EXPECT_EQ(entry.ns_name, "new_ns");
    EXPECT_EQ(entry.config.dim, 128);
    EXPECT_EQ(entry.config.M, 32);
    EXPECT_EQ(entry.config.M0, 64);
    EXPECT_EQ(entry.config.ef_construction, 150);
    EXPECT_EQ(entry.config.metric, Metric::Cosine);
    EXPECT_EQ(entry.config.max_elements, 10000);
}

TEST_F(WalLogTest, AppendAndReplayCheckpoint) {
    {
        WalLog wal(wal_path_);
        wal.append_checkpoint("some/snapshot/path.bin");
        wal.flush();
    }

    WalLog wal(wal_path_);
    auto entries = wal.read_all();
    ASSERT_EQ(entries.size(), 1);
    const auto& entry = entries[0];
    EXPECT_EQ(entry.type, WalRecordType::Checkpoint);
    EXPECT_EQ(entry.snapshot_path, "some/snapshot/path.bin");
}

TEST_F(WalLogTest, MultipleRecordsOrdered) {
    {
        WalLog wal(wal_path_);
        HnswConfig config;
        config.dim = 8;
        wal.append_create_namespace("ns_ordered", config);
        std::vector<float> vec = {1.0f, 2.0f};
        wal.append_upsert("ns_ordered", 1, vec, {});
        wal.append_delete("ns_ordered", 1);
        wal.append_checkpoint("snap.bin");
        wal.append_drop_namespace("ns_ordered");
        wal.flush();
    }

    WalLog wal(wal_path_);
    auto entries = wal.read_all();
    ASSERT_EQ(entries.size(), 5);
    EXPECT_EQ(entries[0].type, WalRecordType::CreateNamespace);
    EXPECT_EQ(entries[1].type, WalRecordType::Upsert);
    EXPECT_EQ(entries[2].type, WalRecordType::Delete);
    EXPECT_EQ(entries[3].type, WalRecordType::Checkpoint);
    EXPECT_EQ(entries[4].type, WalRecordType::DropNamespace);
}

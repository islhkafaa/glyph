#pragma once

#include <filesystem>
#include <mutex>
#include <span>
#include <string_view>
#include <thread>

#include "collection/registry.hpp"
#include "hnsw/config.hpp"
#include "types.hpp"
#include "wal/log.hpp"

class StorageEngine {
   public:
    explicit StorageEngine(std::filesystem::path data_dir);
    ~StorageEngine();

    NamespaceRegistry& registry();

    void create_namespace(std::string_view name, HnswConfig config);
    void drop_namespace(std::string_view name);
    void upsert(std::string_view ns, VectorId id, std::span<const float> vec, Metadata meta);
    void remove(std::string_view ns, VectorId id);

    void checkpoint();
    void recover();

   private:
    std::filesystem::path data_dir_;
    NamespaceRegistry registry_;
    WalLog wal_;
    std::mutex checkpoint_mutex_;
    std::jthread flusher_;

    std::filesystem::path wal_path() const;
    std::filesystem::path snapshot_path() const;
};

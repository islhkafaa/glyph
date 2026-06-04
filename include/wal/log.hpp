#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "hnsw/config.hpp"
#include "types.hpp"
#include "wal/record.hpp"

class WalLog {
   public:
    explicit WalLog(const std::filesystem::path& path);

    void append_upsert(std::string_view ns, VectorId id, std::span<const float> vec,
                       const Metadata& meta);
    void append_delete(std::string_view ns, VectorId id);
    void append_create_namespace(std::string_view ns, const HnswConfig& config);
    void append_drop_namespace(std::string_view ns);
    void append_checkpoint(std::string_view snapshot_path);

    void flush();

    struct ReplayEntry {
        WalRecordType type;
        std::string ns_name;
        VectorId id;
        std::vector<float> vec;
        Metadata meta;
        HnswConfig config;
        std::string snapshot_path;
    };

    std::vector<ReplayEntry> read_all() const;

   private:
    std::filesystem::path path_;
    mutable std::mutex mutex_;
    std::ofstream out_;

    void write_record(WalRecordType type, std::string_view payload);
};

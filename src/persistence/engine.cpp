#include "persistence/engine.hpp"

#include <chrono>
#include <fstream>
#include <stdexcept>

StorageEngine::StorageEngine(std::filesystem::path data_dir)
    : data_dir_(std::move(data_dir)), wal_(wal_path()) {
    std::filesystem::create_directories(data_dir_);
    recover();
    flusher_ = std::jthread([this](std::stop_token stop_token) {
        while (!stop_token.stop_requested()) {
            for (int i = 0; i < 300 && !stop_token.stop_requested(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (stop_token.stop_requested()) {
                break;
            }
            checkpoint();
            bool compacted_any = false;
            std::vector<std::string> ns_list = registry_.list();
            for (const auto& ns : ns_list) {
                try {
                    auto& col = registry_.get(ns);
                    std::uint64_t deleted = col.deleted_count();
                    std::uint64_t active = col.size();
                    if (deleted >= 1000) {
                        double ratio = static_cast<double>(deleted) / (active + deleted);
                        if (ratio >= 0.10) {
                            col.compact();
                            compacted_any = true;
                        }
                    }
                } catch (...) {
                }
            }
            if (compacted_any) {
                checkpoint();
            }
        }
    });
}

StorageEngine::~StorageEngine() {
    if (flusher_.joinable()) {
        flusher_.request_stop();
        flusher_.join();
    }
    checkpoint();
}

NamespaceRegistry& StorageEngine::registry() { return registry_; }

void StorageEngine::create_namespace(std::string_view name, HnswConfig config) {
    wal_.append_create_namespace(name, config);
    wal_.flush();
    registry_.create(name, config);
}

void StorageEngine::drop_namespace(std::string_view name) {
    wal_.append_drop_namespace(name);
    wal_.flush();
    registry_.drop(name);
}

void StorageEngine::upsert(std::string_view ns, VectorId id, std::span<const float> vec,
                           Metadata meta) {
    wal_.append_upsert(ns, id, vec, meta);
    wal_.flush();
    registry_.get(ns).upsert(id, vec, std::move(meta));
}

void StorageEngine::remove(std::string_view ns, VectorId id) {
    wal_.append_delete(ns, id);
    wal_.flush();
    registry_.get(ns).remove(id);
}

void StorageEngine::checkpoint() {
    std::lock_guard<std::mutex> lock(checkpoint_mutex_);
    auto temp_snap = snapshot_path();
    temp_snap += ".tmp";
    {
        std::ofstream out(temp_snap, std::ios::out | std::ios::binary);
        if (!out) {
            throw std::runtime_error("Failed to open temp snapshot file: " + temp_snap.string());
        }
        registry_.serialize(out);
        out.flush();
    }
    std::filesystem::rename(temp_snap, snapshot_path());
    wal_.append_checkpoint(snapshot_path().string());
    wal_.flush();
}

void StorageEngine::recover() {
    auto entries = wal_.read_all();
    std::ptrdiff_t last_checkpoint_idx = -1;
    for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(entries.size()) - 1; i >= 0; --i) {
        if (entries[i].type == WalRecordType::Checkpoint) {
            last_checkpoint_idx = i;
            break;
        }
    }
    if (last_checkpoint_idx != -1) {
        std::ifstream in(entries[last_checkpoint_idx].snapshot_path,
                         std::ios::in | std::ios::binary);
        if (in) {
            registry_.deserialize(in);
        } else {
            throw std::runtime_error("Failed to open snapshot file during recovery: " +
                                     entries[last_checkpoint_idx].snapshot_path);
        }
    }
    for (std::size_t i = static_cast<std::size_t>(last_checkpoint_idx + 1); i < entries.size();
         ++i) {
        const auto& entry = entries[i];
        if (entry.type == WalRecordType::Upsert) {
            if (registry_.exists(entry.ns_name)) {
                registry_.get(entry.ns_name).upsert(entry.id, entry.vec, entry.meta);
            }
        } else if (entry.type == WalRecordType::Delete) {
            if (registry_.exists(entry.ns_name)) {
                registry_.get(entry.ns_name).remove(entry.id);
            }
        } else if (entry.type == WalRecordType::CreateNamespace) {
            registry_.create(entry.ns_name, entry.config);
        } else if (entry.type == WalRecordType::DropNamespace) {
            registry_.drop(entry.ns_name);
        }
    }
}

std::filesystem::path StorageEngine::wal_path() const { return data_dir_ / "wal.log"; }

std::filesystem::path StorageEngine::snapshot_path() const { return data_dir_ / "snapshot.bin"; }

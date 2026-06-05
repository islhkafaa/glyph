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
    auto ns_dir = data_dir_ / "namespaces" / std::string(name);
    if (std::filesystem::exists(ns_dir)) {
        std::filesystem::remove_all(ns_dir);
    }
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
    auto ns_base_dir = data_dir_ / "namespaces";
    std::filesystem::create_directories(ns_base_dir);

    std::vector<std::string> ns_list = registry_.list();
    for (const auto& ns : ns_list) {
        auto& col = registry_.get(ns);
        auto ns_dir = ns_base_dir / ns;
        std::filesystem::create_directories(ns_dir);

        std::shared_ptr<Segment> active_seg;
        for (const auto& seg : col.segments()) {
            if (!seg->is_immutable) {
                active_seg = seg;
                break;
            }
        }
        if (active_seg && active_seg->graph.size() > 0) {
            col.seal_active();
        }

        const auto& segments = col.segments();
        for (const auto& seg : segments) {
            if (seg->is_immutable) {
                auto seg_path = ns_dir / ("segment_" + std::to_string(seg->id) + ".bin");
                if (!std::filesystem::exists(seg_path)) {
                    auto temp_seg_path = seg_path;
                    temp_seg_path += ".tmp";
                    {
                        std::ofstream out(temp_seg_path, std::ios::out | std::ios::binary);
                        if (!out) {
                            throw std::runtime_error("Failed to open segment file: " +
                                                     temp_seg_path.string());
                        }
                        seg->serialize(out);
                        out.flush();
                    }
                    std::filesystem::rename(temp_seg_path, seg_path);
                }
            }
        }

        auto manifest_path = ns_dir / "manifest.bin";
        auto temp_manifest_path = manifest_path;
        temp_manifest_path += ".tmp";
        {
            std::ofstream out(temp_manifest_path, std::ios::out | std::ios::binary);
            if (!out) {
                throw std::runtime_error("Failed to open manifest file: " +
                                         temp_manifest_path.string());
            }

            wal::write_config(out, col.config());

            std::uint32_t next_seg_id = 0;
            std::vector<std::uint32_t> imm_seg_ids;
            for (const auto& seg : segments) {
                next_seg_id = std::max(next_seg_id, seg->id + 1);
                if (seg->is_immutable) {
                    imm_seg_ids.push_back(seg->id);
                }
            }

            wal::write_uint32(out, next_seg_id);
            wal::write_uint32(out, static_cast<std::uint32_t>(imm_seg_ids.size()));
            for (std::uint32_t id : imm_seg_ids) {
                wal::write_uint32(out, id);
            }
            out.flush();
        }
        std::filesystem::rename(temp_manifest_path, manifest_path);
    }

    wal_.append_checkpoint(ns_base_dir.string());
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
        auto ns_base_dir = std::filesystem::path(entries[last_checkpoint_idx].snapshot_path);
        if (std::filesystem::exists(ns_base_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(ns_base_dir)) {
                if (entry.is_directory()) {
                    std::string ns_name = entry.path().filename().string();
                    auto manifest_path = entry.path() / "manifest.bin";
                    if (std::filesystem::exists(manifest_path)) {
                        std::ifstream in(manifest_path, std::ios::in | std::ios::binary);
                        if (!in) {
                            throw std::runtime_error("Failed to open manifest file: " +
                                                     manifest_path.string());
                        }
                        HnswConfig config = wal::read_config(in);
                        std::uint32_t next_seg_id = wal::read_uint32(in);
                        std::uint32_t num_segs = wal::read_uint32(in);

                        std::vector<std::shared_ptr<Segment>> segments;
                        segments.reserve(num_segs);
                        for (std::uint32_t j = 0; j < num_segs; ++j) {
                            std::uint32_t seg_id = wal::read_uint32(in);
                            auto seg_path =
                                entry.path() / ("segment_" + std::to_string(seg_id) + ".bin");
                            std::ifstream seg_in(seg_path, std::ios::in | std::ios::binary);
                            if (!seg_in) {
                                throw std::runtime_error("Failed to open segment file: " +
                                                         seg_path.string());
                            }
                            auto seg = Segment::deserialize(seg_in);
                            segments.push_back(std::move(seg));
                        }

                        Collection col(config, std::move(segments), next_seg_id);
                        registry_.register_collection(ns_name, std::move(col));
                    }
                }
            }
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

#include "wal/log.hpp"

#include <sstream>
#include <stdexcept>

WalLog::WalLog(const std::filesystem::path& path) : path_(path) {
    if (path_.has_parent_path()) {
        std::filesystem::create_directories(path_.parent_path());
    }
    out_.open(path_, std::ios::app | std::ios::binary);
    if (!out_) {
        throw std::runtime_error("Failed to open WAL file: " + path_.string());
    }
}

void WalLog::write_record(WalRecordType type, std::string_view payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    wal::write_uint32(out_, static_cast<std::uint32_t>(type));
    wal::write_uint32(out_, static_cast<std::uint32_t>(payload.size()));
    if (!payload.empty()) {
        out_.write(payload.data(), payload.size());
        if (!out_) {
            throw std::runtime_error("Failed to write record payload");
        }
    }
}

void WalLog::append_upsert(std::string_view ns, VectorId id, std::span<const float> vec,
                           const Metadata& meta) {
    std::stringstream ss(std::ios::out | std::ios::binary);
    wal::write_string(ss, ns);
    wal::write_uint64(ss, id);
    wal::write_uint32(ss, static_cast<std::uint32_t>(vec.size()));
    for (float f : vec) {
        wal::write_float(ss, f);
    }
    wal::write_metadata(ss, meta);
    write_record(WalRecordType::Upsert, ss.str());
}

void WalLog::append_delete(std::string_view ns, VectorId id) {
    std::stringstream ss(std::ios::out | std::ios::binary);
    wal::write_string(ss, ns);
    wal::write_uint64(ss, id);
    write_record(WalRecordType::Delete, ss.str());
}

void WalLog::append_create_namespace(std::string_view ns, const HnswConfig& config) {
    std::stringstream ss(std::ios::out | std::ios::binary);
    wal::write_string(ss, ns);
    wal::write_config(ss, config);
    write_record(WalRecordType::CreateNamespace, ss.str());
}

void WalLog::append_drop_namespace(std::string_view ns) {
    std::stringstream ss(std::ios::out | std::ios::binary);
    wal::write_string(ss, ns);
    write_record(WalRecordType::DropNamespace, ss.str());
}

void WalLog::append_checkpoint(std::string_view snapshot_path) {
    std::stringstream ss(std::ios::out | std::ios::binary);
    wal::write_string(ss, snapshot_path);
    write_record(WalRecordType::Checkpoint, ss.str());
}

void WalLog::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    out_.flush();
}

std::vector<WalLog::ReplayEntry> WalLog::read_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& mutable_out = const_cast<std::ofstream&>(out_);
    mutable_out.flush();
    mutable_out.close();

    std::vector<ReplayEntry> entries;
    {
        std::ifstream in(path_, std::ios::in | std::ios::binary);
        if (in) {
            while (true) {
                std::uint32_t type_val = 0;
                in.read(reinterpret_cast<char*>(&type_val), sizeof(type_val));
                if (in.eof()) {
                    break;
                }
                if (!in) {
                    throw std::runtime_error("Failed to read record type from WAL");
                }

                std::uint32_t payload_len = 0;
                in.read(reinterpret_cast<char*>(&payload_len), sizeof(payload_len));
                if (!in) {
                    throw std::runtime_error("Failed to read payload length from WAL");
                }

                std::string payload(payload_len, '\0');
                if (payload_len > 0) {
                    in.read(payload.data(), payload_len);
                    if (!in) {
                        throw std::runtime_error("Failed to read payload data from WAL");
                    }
                }

                std::stringstream ss(payload, std::ios::in | std::ios::binary);
                ReplayEntry entry;
                entry.type = static_cast<WalRecordType>(type_val);

                if (entry.type == WalRecordType::Upsert) {
                    entry.ns_name = wal::read_string(ss);
                    entry.id = wal::read_uint64(ss);
                    std::uint32_t dim = wal::read_uint32(ss);
                    entry.vec.resize(dim);
                    for (std::uint32_t i = 0; i < dim; ++i) {
                        entry.vec[i] = wal::read_float(ss);
                    }
                    entry.meta = wal::read_metadata(ss);
                } else if (entry.type == WalRecordType::Delete) {
                    entry.ns_name = wal::read_string(ss);
                    entry.id = wal::read_uint64(ss);
                } else if (entry.type == WalRecordType::CreateNamespace) {
                    entry.ns_name = wal::read_string(ss);
                    entry.config = wal::read_config(ss);
                } else if (entry.type == WalRecordType::DropNamespace) {
                    entry.ns_name = wal::read_string(ss);
                } else if (entry.type == WalRecordType::Checkpoint) {
                    entry.snapshot_path = wal::read_string(ss);
                } else {
                    throw std::runtime_error("Invalid WAL record type");
                }
                entries.push_back(std::move(entry));
            }
        }
    }

    mutable_out.open(path_, std::ios::app | std::ios::binary);
    if (!mutable_out) {
        throw std::runtime_error("Failed to reopen WAL file after read_all: " + path_.string());
    }

    return entries;
}

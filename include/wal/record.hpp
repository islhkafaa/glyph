#pragma once

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "hnsw/config.hpp"
#include "metadata/value.hpp"

enum class WalRecordType : std::uint32_t {
    Upsert = 1,
    Delete = 2,
    CreateNamespace = 3,
    DropNamespace = 4,
    Checkpoint = 5
};

namespace wal {

inline void write_uint32(std::ostream& out, std::uint32_t val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    if (!out) {
        throw std::runtime_error("Failed to write uint32");
    }
}

inline std::uint32_t read_uint32(std::istream& in) {
    std::uint32_t val = 0;
    in.read(reinterpret_cast<char*>(&val), sizeof(val));
    if (!in) {
        throw std::runtime_error("Failed to read uint32");
    }
    return val;
}

inline void write_uint64(std::ostream& out, std::uint64_t val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    if (!out) {
        throw std::runtime_error("Failed to write uint64");
    }
}

inline std::uint64_t read_uint64(std::istream& in) {
    std::uint64_t val = 0;
    in.read(reinterpret_cast<char*>(&val), sizeof(val));
    if (!in) {
        throw std::runtime_error("Failed to read uint64");
    }
    return val;
}

inline void write_float(std::ostream& out, float val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    if (!out) {
        throw std::runtime_error("Failed to write float");
    }
}

inline float read_float(std::istream& in) {
    float val = 0.0f;
    in.read(reinterpret_cast<char*>(&val), sizeof(val));
    if (!in) {
        throw std::runtime_error("Failed to read float");
    }
    return val;
}

inline void write_double(std::ostream& out, double val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    if (!out) {
        throw std::runtime_error("Failed to write double");
    }
}

inline double read_double(std::istream& in) {
    double val = 0.0;
    in.read(reinterpret_cast<char*>(&val), sizeof(val));
    if (!in) {
        throw std::runtime_error("Failed to read double");
    }
    return val;
}

inline void write_bool(std::ostream& out, bool val) {
    std::uint8_t byte = val ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
    if (!out) {
        throw std::runtime_error("Failed to write bool");
    }
}

inline bool read_bool(std::istream& in) {
    std::uint8_t byte = 0;
    in.read(reinterpret_cast<char*>(&byte), sizeof(byte));
    if (!in) {
        throw std::runtime_error("Failed to read bool");
    }
    return byte != 0;
}

inline void write_string(std::ostream& out, std::string_view str) {
    write_uint32(out, static_cast<std::uint32_t>(str.size()));
    if (!str.empty()) {
        out.write(str.data(), str.size());
        if (!out) {
            throw std::runtime_error("Failed to write string data");
        }
    }
}

inline std::string read_string(std::istream& in) {
    std::uint32_t len = read_uint32(in);
    std::string str(len, '\0');
    if (len > 0) {
        in.read(str.data(), len);
        if (!in) {
            throw std::runtime_error("Failed to read string data");
        }
    }
    return str;
}

inline void write_metadata(std::ostream& out, const Metadata& meta) {
    write_uint32(out, static_cast<std::uint32_t>(meta.size()));
    for (const auto& [key, value] : meta) {
        write_string(out, key);
        std::uint8_t tag = static_cast<std::uint8_t>(value.index());
        out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
        if (!out) {
            throw std::runtime_error("Failed to write metadata type tag");
        }
        if (tag == 0) {
            write_string(out, std::get<std::string>(value));
        } else if (tag == 1) {
            write_double(out, std::get<double>(value));
        } else if (tag == 2) {
            write_bool(out, std::get<bool>(value));
        } else {
            throw std::runtime_error("Unknown metadata type in variant");
        }
    }
}

inline Metadata read_metadata(std::istream& in) {
    std::uint32_t size = read_uint32(in);
    Metadata meta;
    meta.reserve(size);
    for (std::uint32_t i = 0; i < size; ++i) {
        std::string key = read_string(in);
        std::uint8_t tag = 0;
        in.read(reinterpret_cast<char*>(&tag), sizeof(tag));
        if (!in) {
            throw std::runtime_error("Failed to read metadata type tag");
        }
        MetadataValue val;
        if (tag == 0) {
            val = read_string(in);
        } else if (tag == 1) {
            val = read_double(in);
        } else if (tag == 2) {
            val = read_bool(in);
        } else {
            throw std::runtime_error("Invalid metadata type tag");
        }
        meta[std::move(key)] = std::move(val);
    }
    return meta;
}

inline void write_config(std::ostream& out, const HnswConfig& config) {
    write_uint32(out, config.dim);
    write_uint32(out, config.M);
    write_uint32(out, config.M0);
    write_uint32(out, config.ef_construction);
    write_uint32(out, static_cast<std::uint32_t>(config.metric));
    write_uint64(out, config.max_elements);
}

inline HnswConfig read_config(std::istream& in) {
    HnswConfig config;
    config.dim = read_uint32(in);
    config.M = read_uint32(in);
    config.M0 = read_uint32(in);
    config.ef_construction = read_uint32(in);
    config.metric = static_cast<Metric>(read_uint32(in));
    config.max_elements = read_uint64(in);
    return config;
}

}  // namespace wal

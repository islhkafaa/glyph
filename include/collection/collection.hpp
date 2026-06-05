#pragma once

#include <cstdint>
#include <shared_mutex>
#include <span>
#include <vector>

#include "hnsw/graph.hpp"
#include "metadata/filter.hpp"
#include "metadata/value.hpp"

struct UpsertEntry {
    VectorId id;
    std::vector<float> vec;
    Metadata meta;
};

class Collection {
   public:
    struct SearchResult {
        VectorId id;
        float distance;
        Metadata meta;
    };

    explicit Collection(HnswConfig config);
    Collection(Collection&& other) noexcept;
    Collection(HnswGraph&& graph, std::unordered_map<VectorId, Metadata>&& metadata);

    void upsert(VectorId id, std::span<const float> vec, Metadata meta);
    void batch_upsert(std::vector<UpsertEntry> entries);
    void remove(VectorId id);
    void train();
    void compact();
    std::uint64_t deleted_count() const;

    std::vector<SearchResult> search(std::span<const float> query, std::uint32_t k,
                                     std::uint32_t ef, const Filter& filter = {}) const;

    std::uint64_t size() const;
    const HnswConfig& config() const;

    void serialize(std::ostream& out) const;
    static Collection deserialize(std::istream& in);

   private:
    HnswGraph graph_;
    mutable std::shared_mutex meta_mutex_;
    std::unordered_map<VectorId, Metadata> metadata_;
};

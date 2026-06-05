#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <shared_mutex>
#include <span>
#include <vector>

#include "collection/segment.hpp"
#include "hnsw/config.hpp"
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
    Collection(HnswConfig config, std::vector<std::shared_ptr<Segment>>&& segments,
               std::uint32_t next_segment_id);

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

    void seal_active();
    const std::vector<std::shared_ptr<Segment>>& segments() const;

   private:
    void seal_active_unsafe();
    HnswConfig config_;
    mutable std::shared_mutex rw_mutex_;
    std::vector<std::shared_ptr<Segment>> segments_;
    std::shared_ptr<Segment> active_segment_;
    std::uint32_t next_segment_id_ = 0;
};

#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "hnsw/graph.hpp"
#include "metadata/value.hpp"

class Segment {
   public:
    std::uint32_t id;
    HnswGraph graph;
    std::unordered_map<VectorId, Metadata> metadata;
    std::unordered_set<VectorId> tombstones;
    bool is_immutable = false;
    mutable std::shared_mutex mutex;

    Segment(std::uint32_t seg_id, HnswConfig config);
    Segment(std::uint32_t seg_id, HnswGraph&& g, std::unordered_map<VectorId, Metadata>&& meta,
            std::unordered_set<VectorId>&& stones);

    void serialize(std::ostream& out) const;
    static std::unique_ptr<Segment> deserialize(std::istream& in);
};

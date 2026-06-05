#include "collection/segment.hpp"

#include <iostream>

#include "wal/record.hpp"

Segment::Segment(std::uint32_t seg_id, HnswConfig config) : id(seg_id), graph(config) {}

Segment::Segment(std::uint32_t seg_id, HnswGraph&& g, std::unordered_map<VectorId, Metadata>&& meta,
                 std::unordered_set<VectorId>&& stones)
    : id(seg_id), graph(std::move(g)), metadata(std::move(meta)), tombstones(std::move(stones)) {}

void Segment::serialize(std::ostream& out) const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    wal::write_uint32(out, id);
    wal::write_bool(out, is_immutable);
    graph.serialize(out);
    wal::write_uint32(out, static_cast<std::uint32_t>(metadata.size()));
    for (const auto& [vid, meta] : metadata) {
        wal::write_uint64(out, vid);
        wal::write_metadata(out, meta);
    }
    wal::write_uint32(out, static_cast<std::uint32_t>(tombstones.size()));
    for (const auto& vid : tombstones) {
        wal::write_uint64(out, vid);
    }
}

std::unique_ptr<Segment> Segment::deserialize(std::istream& in) {
    std::uint32_t seg_id = wal::read_uint32(in);
    bool imm = wal::read_bool(in);
    HnswGraph g = HnswGraph::deserialize(in);
    std::uint32_t meta_size = wal::read_uint32(in);
    std::unordered_map<VectorId, Metadata> meta;
    meta.reserve(meta_size);
    for (std::uint32_t i = 0; i < meta_size; ++i) {
        VectorId vid = wal::read_uint64(in);
        Metadata m = wal::read_metadata(in);
        meta[vid] = std::move(m);
    }
    std::uint32_t stone_size = wal::read_uint32(in);
    std::unordered_set<VectorId> stones;
    stones.reserve(stone_size);
    for (std::uint32_t i = 0; i < stone_size; ++i) {
        VectorId vid = wal::read_uint64(in);
        stones.insert(vid);
    }
    auto seg = std::make_unique<Segment>(seg_id, std::move(g), std::move(meta), std::move(stones));
    seg->is_immutable = imm;
    return seg;
}

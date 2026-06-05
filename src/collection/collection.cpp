#include "collection/collection.hpp"

#include <algorithm>
#include <mutex>
#include <unordered_set>

#include "wal/record.hpp"

Collection::Collection(HnswConfig config) : config_(config), next_segment_id_(0) {
    active_segment_ = std::make_shared<Segment>(next_segment_id_++, config_);
    segments_.push_back(active_segment_);
}

Collection::Collection(Collection&& other) noexcept
    : config_(other.config_),
      segments_(std::move(other.segments_)),
      active_segment_(std::move(other.active_segment_)),
      next_segment_id_(other.next_segment_id_) {}

Collection::Collection(HnswGraph&& graph, std::unordered_map<VectorId, Metadata>&& metadata)
    : config_(graph.config()), next_segment_id_(1) {
    active_segment_ = std::make_shared<Segment>(0, std::move(graph), std::move(metadata),
                                                std::unordered_set<VectorId>{});
    segments_.push_back(active_segment_);
}

Collection::Collection(HnswConfig config, std::vector<std::shared_ptr<Segment>>&& segments,
                       std::uint32_t next_segment_id)
    : config_(config), segments_(std::move(segments)), next_segment_id_(next_segment_id) {
    for (const auto& seg : segments_) {
        if (!seg->is_immutable) {
            active_segment_ = seg;
            break;
        }
    }
    if (!active_segment_) {
        active_segment_ = std::make_shared<Segment>(next_segment_id_++, config_);
        segments_.push_back(active_segment_);
    }
}

void Collection::upsert(VectorId id, std::span<const float> vec, Metadata meta) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    active_segment_->tombstones.erase(id);
    active_segment_->graph.upsert(id, vec);
    if (meta.empty()) {
        active_segment_->metadata.erase(id);
    } else {
        active_segment_->metadata[id] = std::move(meta);
    }

    if (active_segment_->graph.size() >= config_.segment_max_size) {
        seal_active_unsafe();
    }
}

void Collection::batch_upsert(std::vector<UpsertEntry> entries) {
    for (auto& entry : entries) {
        upsert(entry.id, entry.vec, std::move(entry.meta));
    }
}

void Collection::remove(VectorId id) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    active_segment_->tombstones.insert(id);
    active_segment_->graph.remove(id);
    active_segment_->metadata.erase(id);
}

void Collection::train() {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    active_segment_->graph.train_quantization();
}

void Collection::compact() {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    if (active_segment_ &&
        (active_segment_->graph.size() > 0 || !active_segment_->tombstones.empty())) {
        seal_active_unsafe();
    }

    std::vector<std::shared_ptr<Segment>> imm_segments;
    std::shared_ptr<Segment> active_seg;

    for (const auto& seg : segments_) {
        if (seg->is_immutable) {
            imm_segments.push_back(seg);
        } else {
            active_seg = seg;
        }
    }

    if (imm_segments.empty()) {
        return;
    }
    if (imm_segments.size() == 1 && imm_segments[0]->tombstones.empty()) {
        return;
    }

    std::uint32_t merged_id = imm_segments.back()->id;

    std::unordered_set<VectorId> resolved;
    std::unordered_set<VectorId> active_tombstones;

    struct VectorEntry {
        VectorId id;
        std::vector<float> vec;
        Metadata meta;
    };
    std::vector<VectorEntry> to_merge;

    for (auto it = imm_segments.rbegin(); it != imm_segments.rend(); ++it) {
        auto seg = *it;
        std::shared_lock<std::shared_mutex> seg_lock(seg->mutex);

        for (const auto& tid : seg->tombstones) {
            active_tombstones.insert(tid);
        }

        auto seg_vectors = seg->graph.get_all_vectors();
        for (auto& vdata : seg_vectors) {
            VectorId vid = vdata.id;
            if (resolved.contains(vid)) {
                continue;
            }
            if (active_tombstones.contains(vid)) {
                resolved.insert(vid);
                continue;
            }

            Metadata meta;
            auto mit = seg->metadata.find(vid);
            if (mit != seg->metadata.end()) {
                meta = mit->second;
            }

            to_merge.push_back({vid, std::move(vdata.vec), std::move(meta)});
            resolved.insert(vid);
        }
    }

    auto merged_seg = std::make_shared<Segment>(merged_id, config_);
    for (auto& entry : to_merge) {
        merged_seg->graph.upsert(entry.id, entry.vec);
        if (!entry.meta.empty()) {
            merged_seg->metadata[entry.id] = std::move(entry.meta);
        }
    }

    if (config_.quant_mode != QuantizationMode::None && !to_merge.empty()) {
        merged_seg->graph.train_quantization();
    }
    merged_seg->is_immutable = true;

    std::vector<std::shared_ptr<Segment>> new_segments;
    new_segments.push_back(merged_seg);
    if (active_seg) {
        new_segments.push_back(active_seg);
    }

    segments_ = std::move(new_segments);
}

std::uint64_t Collection::deleted_count() const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    std::uint64_t count = 0;
    for (const auto& seg : segments_) {
        std::shared_lock<std::shared_mutex> seg_lock(seg->mutex);
        count += seg->tombstones.size();
    }
    return count;
}

std::vector<Collection::SearchResult> Collection::search(std::span<const float> query,
                                                         std::uint32_t k, std::uint32_t ef,
                                                         const Filter& filter) const {
    std::shared_lock<std::shared_mutex> collection_lock(rw_mutex_);

    std::uint32_t search_k = k;
    std::uint32_t search_ef = ef;
    bool has_filter = !std::holds_alternative<std::monostate>(filter);
    if (has_filter) {
        search_k = k * 4;
        search_ef = std::max(ef, search_k);
    }

    struct Candidate {
        VectorId id;
        float distance;
        std::uint32_t segment_id;
        Metadata meta;
    };
    std::vector<Candidate> all_candidates;

    std::unordered_set<VectorId> resolved_ids;
    std::unordered_set<VectorId> active_tombstones;

    struct SegSearchResults {
        std::uint32_t segment_id;
        std::vector<HnswGraph::SearchResult> results;
        std::shared_ptr<Segment> segment;
    };
    std::vector<SegSearchResults> seg_results;
    seg_results.reserve(segments_.size());

    for (auto it = segments_.rbegin(); it != segments_.rend(); ++it) {
        auto seg = *it;
        std::shared_lock<std::shared_mutex> seg_lock(seg->mutex);
        auto results = seg->graph.search(query, search_k, search_ef);
        seg_results.push_back({seg->id, std::move(results), seg});
    }

    for (auto& sr : seg_results) {
        auto seg = sr.segment;
        std::shared_lock<std::shared_mutex> seg_lock(seg->mutex);

        for (const auto& tid : seg->tombstones) {
            active_tombstones.insert(tid);
        }

        for (const auto& cand : sr.results) {
            VectorId vid = cand.id;
            if (resolved_ids.contains(vid)) {
                continue;
            }
            if (active_tombstones.contains(vid)) {
                resolved_ids.insert(vid);
                continue;
            }

            Metadata meta;
            auto meta_it = seg->metadata.find(vid);
            if (meta_it != seg->metadata.end()) {
                meta = meta_it->second;
            }

            if (matches(meta, filter)) {
                all_candidates.push_back({vid, cand.distance, seg->id, std::move(meta)});
            }
            resolved_ids.insert(vid);
        }
    }

    std::sort(all_candidates.begin(), all_candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });

    std::vector<SearchResult> final_results;
    final_results.reserve(std::min(static_cast<std::size_t>(k), all_candidates.size()));
    for (std::size_t i = 0; i < all_candidates.size() && final_results.size() < k; ++i) {
        final_results.push_back(
            {all_candidates[i].id, all_candidates[i].distance, std::move(all_candidates[i].meta)});
    }

    return final_results;
}

std::uint64_t Collection::size() const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    std::uint64_t total_size = 0;
    for (const auto& seg : segments_) {
        std::shared_lock<std::shared_mutex> seg_lock(seg->mutex);
        total_size += seg->graph.size();
    }
    return total_size;
}

const HnswConfig& Collection::config() const { return config_; }

void Collection::serialize(std::ostream& out) const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    wal::write_config(out, config_);
    wal::write_uint32(out, next_segment_id_);
    wal::write_uint32(out, static_cast<std::uint32_t>(segments_.size()));
    for (const auto& seg : segments_) {
        seg->serialize(out);
    }
}

Collection Collection::deserialize(std::istream& in) {
    HnswConfig config = wal::read_config(in);
    std::uint32_t next_seg_id = wal::read_uint32(in);
    std::uint32_t num_segs = wal::read_uint32(in);
    std::vector<std::shared_ptr<Segment>> segments;
    segments.reserve(num_segs);
    for (std::uint32_t i = 0; i < num_segs; ++i) {
        segments.push_back(Segment::deserialize(in));
    }
    return Collection(config, std::move(segments), next_seg_id);
}

void Collection::seal_active_unsafe() {
    if (active_segment_ && !active_segment_->is_immutable) {
        active_segment_->is_immutable = true;
    }
    active_segment_ = std::make_shared<Segment>(next_segment_id_++, config_);
    segments_.push_back(active_segment_);
}

void Collection::seal_active() {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    seal_active_unsafe();
}

const std::vector<std::shared_ptr<Segment>>& Collection::segments() const { return segments_; }

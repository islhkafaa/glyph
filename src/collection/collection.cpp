#include "collection/collection.hpp"

#include <mutex>

#include "wal/record.hpp"

Collection::Collection(HnswConfig config) : graph_(config) {}

Collection::Collection(Collection&& other) noexcept
    : graph_(std::move(other.graph_)), metadata_(std::move(other.metadata_)) {}

Collection::Collection(HnswGraph&& graph, std::unordered_map<VectorId, Metadata>&& metadata)
    : graph_(std::move(graph)), metadata_(std::move(metadata)) {}

void Collection::upsert(VectorId id, std::span<const float> vec, Metadata meta) {
    graph_.upsert(id, vec);
    std::unique_lock<std::shared_mutex> lock(meta_mutex_);
    if (meta.empty()) {
        metadata_.erase(id);
    } else {
        metadata_[id] = std::move(meta);
    }
}

void Collection::batch_upsert(std::vector<UpsertEntry> entries) {
    for (auto& entry : entries) {
        upsert(entry.id, entry.vec, std::move(entry.meta));
    }
}

void Collection::remove(VectorId id) {
    graph_.remove(id);
    std::unique_lock<std::shared_mutex> lock(meta_mutex_);
    metadata_.erase(id);
}

void Collection::train() { graph_.train_quantization(); }

void Collection::compact() { graph_.compact(); }

std::uint64_t Collection::deleted_count() const { return graph_.deleted_count(); }

std::vector<Collection::SearchResult> Collection::search(std::span<const float> query,
                                                         std::uint32_t k, std::uint32_t ef,
                                                         const Filter& filter) const {
    std::uint32_t search_k = k;
    std::uint32_t search_ef = ef;
    bool has_filter = !std::holds_alternative<std::monostate>(filter);
    if (has_filter) {
        search_k = k * 4;
        search_ef = std::max(ef, search_k);
    }

    auto candidates = graph_.search(query, search_k, search_ef);

    std::vector<SearchResult> results;
    results.reserve(std::min(static_cast<std::size_t>(k), candidates.size()));

    std::shared_lock<std::shared_mutex> lock(meta_mutex_);
    for (const auto& cand : candidates) {
        if (results.size() >= k) {
            break;
        }
        auto it = metadata_.find(cand.id);
        Metadata meta;
        if (it != metadata_.end()) {
            meta = it->second;
        }
        if (matches(meta, filter)) {
            results.push_back({cand.id, cand.distance, std::move(meta)});
        }
    }

    return results;
}

std::uint64_t Collection::size() const { return graph_.size(); }

const HnswConfig& Collection::config() const { return graph_.config(); }

void Collection::serialize(std::ostream& out) const {
    std::shared_lock<std::shared_mutex> lock(meta_mutex_);
    graph_.serialize(out);
    wal::write_uint32(out, static_cast<std::uint32_t>(metadata_.size()));
    for (const auto& [id, meta] : metadata_) {
        wal::write_uint64(out, id);
        wal::write_metadata(out, meta);
    }
}

Collection Collection::deserialize(std::istream& in) {
    HnswGraph graph = HnswGraph::deserialize(in);
    std::uint32_t size = wal::read_uint32(in);
    std::unordered_map<VectorId, Metadata> metadata;
    metadata.reserve(size);
    for (std::uint32_t i = 0; i < size; ++i) {
        VectorId id = wal::read_uint64(in);
        Metadata meta = wal::read_metadata(in);
        metadata[id] = std::move(meta);
    }
    return Collection(std::move(graph), std::move(metadata));
}

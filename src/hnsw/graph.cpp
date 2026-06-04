#include "hnsw/graph.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>

#include "wal/record.hpp"

HnswGraph::HnswGraph(HnswConfig config)
    : config_(config), kernel_(get_kernel(config.metric)), max_level_(-1), rng_(42) {
    if (config_.M0 == 0) {
        config_.M0 = 2 * config_.M;
    }
}

HnswGraph::HnswGraph(HnswGraph&& other) noexcept
    : config_(other.config_),
      kernel_(other.kernel_),
      id_to_node_(std::move(other.id_to_node_)),
      node_to_id_(std::move(other.node_to_id_)),
      vectors_(std::move(other.vectors_)),
      node_level_(std::move(other.node_level_)),
      layers_(std::move(other.layers_)),
      deleted_nodes_(std::move(other.deleted_nodes_)),
      entry_point_(other.entry_point_),
      max_level_(other.max_level_),
      rng_(std::move(other.rng_)) {}

void HnswGraph::upsert(VectorId id, std::span<const float> vec) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);

    auto it = id_to_node_.find(id);
    if (it != id_to_node_.end()) {
        deleted_nodes_.insert(it->second);
        id_to_node_.erase(it);
        if (id_to_node_.empty()) {
            max_level_ = -1;
            entry_point_ = 0;
            vectors_.clear();
            node_to_id_.clear();
            node_level_.clear();
            layers_.clear();
            deleted_nodes_.clear();
        }
    }

    std::uint32_t node_idx = static_cast<std::uint32_t>(vectors_.size());
    vectors_.push_back(std::vector<float>(vec.begin(), vec.end()));
    node_to_id_.push_back(id);
    id_to_node_[id] = node_idx;

    int level = generate_random_level();
    node_level_.push_back(level);

    if (max_level_ == -1) {
        max_level_ = level;
        entry_point_ = node_idx;
        layers_.resize(max_level_ + 1);
        for (int l = 0; l <= max_level_; ++l) {
            layers_[l].adj.resize(node_idx + 1);
        }
        return;
    }

    if (level > max_level_) {
        layers_.resize(level + 1);
    }
    for (int l = 0; l <= level; ++l) {
        if (layers_[l].adj.size() <= node_idx) {
            layers_[l].adj.resize(node_idx + 1);
        }
    }

    std::uint32_t curr_obj = entry_point_;
    std::vector<float> query_vec(vec.begin(), vec.end());

    for (int l = max_level_; l > level; --l) {
        curr_obj = search_layer(query_vec, curr_obj, l);
    }

    std::vector<std::uint32_t> enter_nodes = {curr_obj};
    for (int l = std::min(level, max_level_); l >= 0; --l) {
        auto candidates = search_layer_ef(query_vec, enter_nodes, config_.ef_construction, l);

        std::uint32_t M_max = (l == 0) ? config_.M0 : config_.M;
        auto neighbors = select_neighbors(query_vec, candidates, M_max);

        for (std::uint32_t neighbor : neighbors) {
            layers_[l].adj[node_idx].push_back(neighbor);
            layers_[l].adj[neighbor].push_back(node_idx);
        }

        for (std::uint32_t neighbor : neighbors) {
            auto& n_neighbors = layers_[l].adj[neighbor];
            std::uint32_t n_M_max = (l == 0) ? config_.M0 : config_.M;
            if (n_neighbors.size() > n_M_max) {
                std::vector<std::pair<float, std::uint32_t>> n_candidates;
                n_candidates.reserve(n_neighbors.size());
                std::span<const float> n_vec = vectors_[neighbor];
                for (std::uint32_t nn : n_neighbors) {
                    float dist = kernel_(n_vec, vectors_[nn]);
                    n_candidates.push_back({dist, nn});
                }
                std::sort(n_candidates.begin(), n_candidates.end());
                auto new_n_neighbors = select_neighbors(n_vec, n_candidates, n_M_max);
                n_neighbors = std::move(new_n_neighbors);
            }
        }

        enter_nodes.clear();
        enter_nodes.reserve(candidates.size());
        for (const auto& c : candidates) {
            enter_nodes.push_back(c.second);
        }
    }

    if (level > max_level_) {
        max_level_ = level;
        entry_point_ = node_idx;
    }
}

void HnswGraph::remove(VectorId id) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    auto it = id_to_node_.find(id);
    if (it != id_to_node_.end()) {
        deleted_nodes_.insert(it->second);
        id_to_node_.erase(it);
        if (id_to_node_.empty()) {
            max_level_ = -1;
            entry_point_ = 0;
            vectors_.clear();
            node_to_id_.clear();
            node_level_.clear();
            layers_.clear();
            deleted_nodes_.clear();
        }
    }
}

std::vector<HnswGraph::SearchResult> HnswGraph::search(std::span<const float> query,
                                                       std::uint32_t k, std::uint32_t ef) const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);

    if (max_level_ == -1 || id_to_node_.empty()) {
        return {};
    }

    std::uint32_t curr_obj = entry_point_;
    for (int l = max_level_; l > 0; --l) {
        curr_obj = search_layer(query, curr_obj, l);
    }

    auto candidates = search_layer_ef(query, {curr_obj}, std::max(ef, k), 0);

    std::vector<SearchResult> results;
    results.reserve(std::min(static_cast<std::size_t>(k), candidates.size()));
    for (std::size_t i = 0; i < candidates.size() && results.size() < k; ++i) {
        results.push_back({node_to_id_[candidates[i].second], candidates[i].first});
    }

    return results;
}

std::uint64_t HnswGraph::size() const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    return id_to_node_.size();
}

const HnswConfig& HnswGraph::config() const { return config_; }

int HnswGraph::generate_random_level() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    if (r == 0.0) {
        r = 0.0000001;
    }
    double m_l = 1.0 / std::log(static_cast<double>(config_.M));
    return static_cast<int>(std::floor(-std::log(r) * m_l));
}

std::uint32_t HnswGraph::search_layer(std::span<const float> query, std::uint32_t enter_node,
                                      int level) const {
    std::uint32_t curr_node = enter_node;
    float curr_dist = kernel_(query, vectors_[curr_node]);
    bool changed = true;
    while (changed) {
        changed = false;
        const auto& neighbors = layers_[level].adj[curr_node];
        for (std::uint32_t neighbor : neighbors) {
            float dist = kernel_(query, vectors_[neighbor]);
            if (dist < curr_dist) {
                curr_dist = dist;
                curr_node = neighbor;
                changed = true;
            }
        }
    }
    return curr_node;
}

std::vector<std::pair<float, std::uint32_t>> HnswGraph::search_layer_ef(
    std::span<const float> query, const std::vector<std::uint32_t>& enter_nodes, std::uint32_t ef,
    int level) const {
    struct CompareMin {
        bool operator()(const std::pair<float, std::uint32_t>& a,
                        const std::pair<float, std::uint32_t>& b) const {
            return a.first > b.first;
        }
    };
    struct CompareMax {
        bool operator()(const std::pair<float, std::uint32_t>& a,
                        const std::pair<float, std::uint32_t>& b) const {
            return a.first < b.first;
        }
    };

    std::unordered_set<std::uint32_t> visited(enter_nodes.begin(), enter_nodes.end());
    std::vector<std::pair<float, std::uint32_t>> candidates;
    std::vector<std::pair<float, std::uint32_t>> nearest;

    for (std::uint32_t enter_node : enter_nodes) {
        float dist = kernel_(query, vectors_[enter_node]);
        candidates.push_back({dist, enter_node});
        if (!deleted_nodes_.contains(enter_node)) {
            nearest.push_back({dist, enter_node});
        }
    }

    std::make_heap(candidates.begin(), candidates.end(), CompareMin());
    std::make_heap(nearest.begin(), nearest.end(), CompareMax());

    while (!candidates.empty()) {
        std::pop_heap(candidates.begin(), candidates.end(), CompareMin());
        auto curr = candidates.back();
        candidates.pop_back();

        float furthest_dist =
            nearest.empty() ? std::numeric_limits<float>::max() : nearest.front().first;
        if (curr.first > furthest_dist && nearest.size() >= ef) {
            break;
        }

        const auto& neighbors = layers_[level].adj[curr.second];
        for (std::uint32_t neighbor : neighbors) {
            if (visited.insert(neighbor).second) {
                float dist = kernel_(query, vectors_[neighbor]);
                float f_dist =
                    nearest.empty() ? std::numeric_limits<float>::max() : nearest.front().first;
                if (dist < f_dist || nearest.size() < ef) {
                    candidates.push_back({dist, neighbor});
                    std::push_heap(candidates.begin(), candidates.end(), CompareMin());

                    if (!deleted_nodes_.contains(neighbor)) {
                        nearest.push_back({dist, neighbor});
                        std::push_heap(nearest.begin(), nearest.end(), CompareMax());

                        if (nearest.size() > ef) {
                            std::pop_heap(nearest.begin(), nearest.end(), CompareMax());
                            nearest.pop_back();
                        }
                    }
                }
            }
        }
    }

    std::sort(nearest.begin(), nearest.end());
    return nearest;
}

std::vector<std::uint32_t> HnswGraph::select_neighbors(
    [[maybe_unused]] std::span<const float> query,
    const std::vector<std::pair<float, std::uint32_t>>& candidates, std::uint32_t M_max) const {
    std::vector<std::uint32_t> result;
    result.reserve(std::min(static_cast<std::size_t>(M_max), candidates.size()));
    for (std::size_t i = 0; i < candidates.size() && result.size() < M_max; ++i) {
        result.push_back(candidates[i].second);
    }
    return result;
}

void HnswGraph::serialize(std::ostream& out) const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    wal::write_config(out, config_);
    std::uint32_t active_count = 0;
    std::vector<std::uint32_t> compact_map(vectors_.size(), 0xFFFFFFFF);
    for (std::uint32_t i = 0; i < vectors_.size(); ++i) {
        if (!deleted_nodes_.contains(i)) {
            compact_map[i] = active_count++;
        }
    }
    wal::write_uint32(out, active_count);
    for (std::uint32_t i = 0; i < vectors_.size(); ++i) {
        if (!deleted_nodes_.contains(i)) {
            wal::write_uint64(out, node_to_id_[i]);
            wal::write_uint32(out, static_cast<std::uint32_t>(vectors_[i].size()));
            for (float f : vectors_[i]) {
                wal::write_float(out, f);
            }
            wal::write_uint32(out, static_cast<std::uint32_t>(node_level_[i]));
        }
    }
    std::uint32_t new_entry_point = 0;
    int new_max_level = -1;
    if (active_count > 0) {
        if (!deleted_nodes_.contains(entry_point_)) {
            new_entry_point = compact_map[entry_point_];
            new_max_level = max_level_;
        } else {
            int max_l = -1;
            std::uint32_t best_node = 0;
            for (std::uint32_t i = 0; i < vectors_.size(); ++i) {
                if (!deleted_nodes_.contains(i)) {
                    if (node_level_[i] > max_l) {
                        max_l = node_level_[i];
                        best_node = i;
                    }
                }
            }
            if (max_l != -1) {
                new_entry_point = compact_map[best_node];
                new_max_level = max_l;
            }
        }
    }
    wal::write_uint32(out, static_cast<std::uint32_t>(new_max_level));
    wal::write_uint32(out, new_entry_point);
    for (int l = 0; l <= new_max_level; ++l) {
        for (std::uint32_t i = 0; i < vectors_.size(); ++i) {
            if (!deleted_nodes_.contains(i)) {
                std::vector<std::uint32_t> active_neighbors;
                if (l < static_cast<int>(layers_.size()) && i < layers_[l].adj.size()) {
                    for (std::uint32_t n : layers_[l].adj[i]) {
                        if (n < compact_map.size() && compact_map[n] != 0xFFFFFFFF) {
                            active_neighbors.push_back(compact_map[n]);
                        }
                    }
                }
                wal::write_uint32(out, static_cast<std::uint32_t>(active_neighbors.size()));
                for (std::uint32_t n : active_neighbors) {
                    wal::write_uint32(out, n);
                }
            }
        }
    }
}

HnswGraph HnswGraph::deserialize(std::istream& in) {
    HnswConfig config = wal::read_config(in);
    HnswGraph graph(config);
    std::uint32_t active_count = wal::read_uint32(in);
    graph.vectors_.resize(active_count);
    graph.node_to_id_.resize(active_count);
    graph.id_to_node_.reserve(active_count);
    graph.node_level_.resize(active_count);
    for (std::uint32_t i = 0; i < active_count; ++i) {
        graph.node_to_id_[i] = wal::read_uint64(in);
        std::uint32_t dim = wal::read_uint32(in);
        graph.vectors_[i].resize(dim);
        for (std::uint32_t d = 0; d < dim; ++d) {
            graph.vectors_[i][d] = wal::read_float(in);
        }
        graph.node_level_[i] = static_cast<int>(wal::read_uint32(in));
        graph.id_to_node_[graph.node_to_id_[i]] = i;
    }
    graph.max_level_ = static_cast<int>(wal::read_uint32(in));
    graph.entry_point_ = wal::read_uint32(in);
    if (graph.max_level_ >= 0) {
        graph.layers_.resize(graph.max_level_ + 1);
        for (int l = 0; l <= graph.max_level_; ++l) {
            graph.layers_[l].adj.resize(active_count);
            for (std::uint32_t i = 0; i < active_count; ++i) {
                std::uint32_t neighbor_count = wal::read_uint32(in);
                graph.layers_[l].adj[i].resize(neighbor_count);
                for (std::uint32_t j = 0; j < neighbor_count; ++j) {
                    graph.layers_[l].adj[i][j] = wal::read_uint32(in);
                }
            }
        }
    }
    return graph;
}

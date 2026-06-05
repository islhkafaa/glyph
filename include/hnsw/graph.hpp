#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <shared_mutex>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "distance/kernel.hpp"
#include "hnsw/config.hpp"
#include "hnsw/layer.hpp"
#include "quantization/pq.hpp"
#include "quantization/sq8.hpp"
#include "types.hpp"

class HnswGraph {
   public:
    struct SearchResult {
        VectorId id;
        float distance;
    };

    struct VectorData {
        VectorId id;
        std::vector<float> vec;
    };
    std::vector<VectorData> get_all_vectors() const;

    explicit HnswGraph(HnswConfig config);
    HnswGraph(HnswGraph&& other) noexcept;

    void upsert(VectorId id, std::span<const float> vec);
    void remove(VectorId id);

    std::vector<SearchResult> search(std::span<const float> query, std::uint32_t k,
                                     std::uint32_t ef) const;

    std::uint64_t size() const;
    const HnswConfig& config() const;

    void train_quantization();
    bool quant_trained() const { return quant_trained_; }

    void compact();
    std::uint64_t deleted_count() const;

    void serialize(std::ostream& out) const;
    static HnswGraph deserialize(std::istream& in);

   private:
    HnswConfig config_;
    KernelFn kernel_;

    mutable std::shared_mutex rw_mutex_;

    std::unordered_map<VectorId, std::uint32_t> id_to_node_;
    std::vector<VectorId> node_to_id_;
    std::vector<std::vector<float>> vectors_;
    std::vector<std::vector<std::uint8_t>> quant_codes_;
    bool quant_trained_ = false;
    std::optional<Sq8Codebook> sq8_codebook_;
    std::optional<PqCodebook> pq_codebook_;
    std::vector<int> node_level_;
    std::vector<Layer> layers_;
    std::unordered_set<std::uint32_t> deleted_nodes_;

    std::uint32_t entry_point_;
    int max_level_;

    mutable std::mt19937 rng_;

    int generate_random_level();

    std::uint32_t search_layer(std::span<const float> query, std::uint32_t enter_node,
                               int level) const;

    std::vector<std::pair<float, std::uint32_t>> search_layer_ef(
        std::span<const float> query, const std::vector<std::uint32_t>& enter_nodes,
        std::uint32_t ef, int level) const;

    std::vector<std::uint32_t> select_neighbors(
        std::span<const float> query,
        const std::vector<std::pair<float, std::uint32_t>>& candidates, std::uint32_t M_max) const;
};

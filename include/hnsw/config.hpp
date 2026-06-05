#pragma once

#include <cstdint>

#include "distance/metric.hpp"
#include "types.hpp"

enum class QuantizationMode : std::uint32_t { None = 0, SQ8 = 1, PQ = 2 };

struct HnswConfig {
    Dimension dim;
    std::uint32_t M = 16;
    std::uint32_t M0 = 32;
    std::uint32_t ef_construction = 200;
    Metric metric = Metric::L2;
    std::uint64_t max_elements = 0;
    QuantizationMode quant_mode = QuantizationMode::None;
    std::uint32_t pq_m = 8;
    std::uint32_t segment_max_size = 10000;
};

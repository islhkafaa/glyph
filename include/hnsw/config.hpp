#pragma once

#include <cstdint>

#include "distance/metric.hpp"
#include "types.hpp"

struct HnswConfig {
    Dimension dim;
    std::uint32_t M = 16;
    std::uint32_t M0 = 32;
    std::uint32_t ef_construction = 200;
    Metric metric = Metric::L2;
    std::uint64_t max_elements = 0;
};

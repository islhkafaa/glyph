#pragma once

#include <cstdint>
#include <vector>

struct Layer {
    std::vector<std::vector<std::uint32_t>> adj;
};

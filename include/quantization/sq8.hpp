#pragma once

#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "distance/metric.hpp"
#include "types.hpp"

struct Sq8Codebook {
    std::vector<float> min_vals;
    std::vector<float> scales;
    Dimension dim;

    void train(std::span<const std::vector<float>> vectors);
    std::vector<uint8_t> encode(std::span<const float> vec) const;
    std::vector<float> decode(std::span<const uint8_t> code) const;
    float distance(std::span<const float> query, std::span<const uint8_t> code,
                   Metric metric) const;

    void serialize(std::ostream& out) const;
    static Sq8Codebook deserialize(std::istream& in);
};

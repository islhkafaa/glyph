#pragma once

#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "distance/kernel.hpp"
#include "types.hpp"

struct PqLookup {
    Metric metric;
    std::vector<std::vector<float>> table;
    float query_norm;
    std::vector<std::vector<float>> centroid_norms_sq;
};

struct PqCodebook {
    std::uint32_t M_pq;
    std::uint32_t ksub;
    Dimension dim;

    std::vector<std::vector<std::vector<float>>> centroids;

    void train(std::span<const std::vector<float>> vectors, std::uint32_t m_pq);
    std::vector<uint8_t> encode(std::span<const float> vec) const;
    std::vector<float> decode(std::span<const uint8_t> code) const;

    PqLookup build_lookup(std::span<const float> query, Metric metric) const;
    float adc_distance(const PqLookup& lookup, std::span<const uint8_t> code, KernelFn kernel,
                       std::span<const float> query) const;

    void serialize(std::ostream& out) const;
    static PqCodebook deserialize(std::istream& in);
};

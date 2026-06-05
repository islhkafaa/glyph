#include "quantization/sq8.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "distance/dispatch.hpp"
#include "wal/record.hpp"

void Sq8Codebook::train(std::span<const std::vector<float>> vectors) {
    if (vectors.empty()) {
        return;
    }
    dim = static_cast<Dimension>(vectors[0].size());
    min_vals.assign(dim, std::numeric_limits<float>::max());
    std::vector<float> max_vals(dim, -std::numeric_limits<float>::max());

    for (const auto& vec : vectors) {
        for (Dimension d = 0; d < dim; ++d) {
            min_vals[d] = std::min(min_vals[d], vec[d]);
            max_vals[d] = std::max(max_vals[d], vec[d]);
        }
    }

    scales.resize(dim);
    for (Dimension d = 0; d < dim; ++d) {
        float diff = max_vals[d] - min_vals[d];
        if (diff < 1e-30f) {
            scales[d] = 0.0f;
        } else {
            scales[d] = diff / 255.0f;
        }
    }
}

std::vector<uint8_t> Sq8Codebook::encode(std::span<const float> vec) const {
    std::vector<uint8_t> code(dim);
    for (Dimension d = 0; d < dim; ++d) {
        if (scales[d] == 0.0f) {
            code[d] = 0;
        } else {
            float val = (vec[d] - min_vals[d]) / scales[d];
            code[d] = static_cast<uint8_t>(std::clamp(std::round(val), 0.0f, 255.0f));
        }
    }
    return code;
}

std::vector<float> Sq8Codebook::decode(std::span<const uint8_t> code) const {
    std::vector<float> vec(dim);
    for (Dimension d = 0; d < dim; ++d) {
        vec[d] = min_vals[d] + code[d] * scales[d];
    }
    return vec;
}

float Sq8Codebook::distance(std::span<const float> query, std::span<const uint8_t> code,
                            Metric metric) const {
    Sq8KernelFn kernel = get_sq8_kernel(metric);
    return kernel(query, code, min_vals, scales);
}

void Sq8Codebook::serialize(std::ostream& out) const {
    wal::write_uint32(out, dim);
    for (Dimension d = 0; d < dim; ++d) {
        wal::write_float(out, min_vals[d]);
        wal::write_float(out, scales[d]);
    }
}

Sq8Codebook Sq8Codebook::deserialize(std::istream& in) {
    Sq8Codebook cb;
    cb.dim = wal::read_uint32(in);
    cb.min_vals.resize(cb.dim);
    cb.scales.resize(cb.dim);
    for (Dimension d = 0; d < cb.dim; ++d) {
        cb.min_vals[d] = wal::read_float(in);
        cb.scales[d] = wal::read_float(in);
    }
    return cb;
}

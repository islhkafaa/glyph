#include "quantization/pq.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "distance/dispatch.hpp"
#include "wal/record.hpp"

static std::vector<std::uint32_t> get_sub_dims(Dimension dim, std::uint32_t M_pq) {
    std::vector<std::uint32_t> dims(M_pq);
    std::uint32_t base = dim / M_pq;
    std::uint32_t rem = dim % M_pq;
    for (std::uint32_t i = 0; i < M_pq; ++i) {
        dims[i] = base + (i < rem ? 1 : 0);
    }
    return dims;
}

static std::vector<std::uint32_t> get_sub_offsets(const std::vector<std::uint32_t>& dims) {
    std::vector<std::uint32_t> offsets(dims.size() + 1, 0);
    for (std::size_t i = 0; i < dims.size(); ++i) {
        offsets[i + 1] = offsets[i] + dims[i];
    }
    return offsets;
}

void PqCodebook::train(std::span<const std::vector<float>> vectors, std::uint32_t m_pq) {
    if (vectors.empty()) {
        return;
    }
    dim = static_cast<Dimension>(vectors[0].size());
    M_pq = m_pq;
    ksub = std::min(256U, static_cast<std::uint32_t>(vectors.size()));
    if (ksub == 0) {
        return;
    }

    auto dims = get_sub_dims(dim, M_pq);
    auto offsets = get_sub_offsets(dims);

    centroids.resize(M_pq);

    for (std::uint32_t s = 0; s < M_pq; ++s) {
        std::uint32_t sub_dim = dims[s];
        centroids[s].resize(ksub);

        for (std::uint32_t c = 0; c < ksub; ++c) {
            std::size_t idx = c * (vectors.size() / ksub);
            centroids[s][c].assign(vectors[idx].begin() + offsets[s],
                                   vectors[idx].begin() + offsets[s + 1]);
        }

        for (int iter = 0; iter < 15; ++iter) {
            std::vector<std::vector<float>> new_sums(ksub, std::vector<float>(sub_dim, 0.0f));
            std::vector<std::uint32_t> counts(ksub, 0);

            for (const auto& vec : vectors) {
                float min_d = std::numeric_limits<float>::max();
                std::uint32_t best_c = 0;

                for (std::uint32_t c = 0; c < ksub; ++c) {
                    float dist = 0.0f;
                    for (std::uint32_t d = 0; d < sub_dim; ++d) {
                        float diff = vec[offsets[s] + d] - centroids[s][c][d];
                        dist += diff * diff;
                    }
                    if (dist < min_d) {
                        min_d = dist;
                        best_c = c;
                    }
                }

                for (std::uint32_t d = 0; d < sub_dim; ++d) {
                    new_sums[best_c][d] += vec[offsets[s] + d];
                }
                counts[best_c]++;
            }

            for (std::uint32_t c = 0; c < ksub; ++c) {
                if (counts[c] > 0) {
                    for (std::uint32_t d = 0; d < sub_dim; ++d) {
                        centroids[s][c][d] = new_sums[c][d] / counts[c];
                    }
                }
            }
        }
    }
}

std::vector<uint8_t> PqCodebook::encode(std::span<const float> vec) const {
    auto dims = get_sub_dims(dim, M_pq);
    auto offsets = get_sub_offsets(dims);
    std::vector<uint8_t> code(M_pq);

    for (std::uint32_t s = 0; s < M_pq; ++s) {
        std::uint32_t sub_dim = dims[s];
        float min_d = std::numeric_limits<float>::max();
        std::uint32_t best_c = 0;

        for (std::uint32_t c = 0; c < ksub; ++c) {
            float dist = 0.0f;
            for (std::uint32_t d = 0; d < sub_dim; ++d) {
                float diff = vec[offsets[s] + d] - centroids[s][c][d];
                dist += diff * diff;
            }
            if (dist < min_d) {
                min_d = dist;
                best_c = c;
            }
        }
        code[s] = static_cast<uint8_t>(best_c);
    }
    return code;
}

std::vector<float> PqCodebook::decode(std::span<const uint8_t> code) const {
    auto dims = get_sub_dims(dim, M_pq);
    auto offsets = get_sub_offsets(dims);
    std::vector<float> vec(dim);
    for (std::uint32_t s = 0; s < M_pq; ++s) {
        std::copy(centroids[s][code[s]].begin(), centroids[s][code[s]].end(),
                  vec.begin() + offsets[s]);
    }
    return vec;
}

PqLookup PqCodebook::build_lookup(std::span<const float> query, Metric metric) const {
    auto dims = get_sub_dims(dim, M_pq);
    auto offsets = get_sub_offsets(dims);
    PqLookup lookup;
    lookup.metric = metric;
    lookup.table.resize(M_pq, std::vector<float>(ksub, 0.0f));

    KernelFn dot_kernel = get_kernel(Metric::DotProduct);
    KernelFn l2_sq_kernel = get_dispatch_table().l2_sq;

    if (metric == Metric::Cosine) {
        float query_norm_sq = 0.0f;
        for (float f : query) {
            query_norm_sq += f * f;
        }
        lookup.query_norm = std::sqrt(query_norm_sq);
        lookup.centroid_norms_sq.resize(M_pq, std::vector<float>(ksub, 0.0f));

        for (std::uint32_t s = 0; s < M_pq; ++s) {
            std::uint32_t sub_dim = dims[s];
            std::span<const float> q_sub = query.subspan(offsets[s], sub_dim);
            for (std::uint32_t c = 0; c < ksub; ++c) {
                float dot = -dot_kernel(q_sub, centroids[s][c]);
                float norm_sq = -dot_kernel(centroids[s][c], centroids[s][c]);
                lookup.table[s][c] = dot;
                lookup.centroid_norms_sq[s][c] = norm_sq;
            }
        }
    } else if (metric == Metric::DotProduct) {
        for (std::uint32_t s = 0; s < M_pq; ++s) {
            std::uint32_t sub_dim = dims[s];
            std::span<const float> q_sub = query.subspan(offsets[s], sub_dim);
            for (std::uint32_t c = 0; c < ksub; ++c) {
                lookup.table[s][c] = -dot_kernel(q_sub, centroids[s][c]);
            }
        }
    } else if (metric == Metric::L2) {
        for (std::uint32_t s = 0; s < M_pq; ++s) {
            std::uint32_t sub_dim = dims[s];
            std::span<const float> q_sub = query.subspan(offsets[s], sub_dim);
            for (std::uint32_t c = 0; c < ksub; ++c) {
                lookup.table[s][c] = l2_sq_kernel(q_sub, centroids[s][c]);
            }
        }
    }
    return lookup;
}

float PqCodebook::adc_distance(const PqLookup& lookup, std::span<const uint8_t> code,
                               KernelFn kernel, std::span<const float> query) const {
    if (lookup.metric == Metric::L2) {
        float sum_sq = 0.0f;
        for (std::uint32_t s = 0; s < M_pq; ++s) {
            sum_sq += lookup.table[s][code[s]];
        }
        return std::sqrt(sum_sq);
    } else if (lookup.metric == Metric::DotProduct) {
        float sum_dot = 0.0f;
        for (std::uint32_t s = 0; s < M_pq; ++s) {
            sum_dot += lookup.table[s][code[s]];
        }
        return -sum_dot;
    } else if (lookup.metric == Metric::Cosine) {
        float sum_dot = 0.0f;
        float sum_norm_sq = 0.0f;
        for (std::uint32_t s = 0; s < M_pq; ++s) {
            sum_dot += lookup.table[s][code[s]];
            sum_norm_sq += lookup.centroid_norms_sq[s][code[s]];
        }
        if (lookup.query_norm == 0.0f || sum_norm_sq == 0.0f) {
            return 1.0f;
        }
        return 1.0f - (sum_dot / (lookup.query_norm * std::sqrt(sum_norm_sq)));
    } else {
        auto dims = get_sub_dims(dim, M_pq);
        auto offsets = get_sub_offsets(dims);
        std::vector<float> reconstructed(dim);
        for (std::uint32_t s = 0; s < M_pq; ++s) {
            std::copy(centroids[s][code[s]].begin(), centroids[s][code[s]].end(),
                      reconstructed.begin() + offsets[s]);
        }
        return kernel(query, reconstructed);
    }
}

void PqCodebook::serialize(std::ostream& out) const {
    wal::write_uint32(out, M_pq);
    wal::write_uint32(out, ksub);
    wal::write_uint32(out, dim);
    for (std::uint32_t s = 0; s < M_pq; ++s) {
        for (std::uint32_t c = 0; c < ksub; ++c) {
            wal::write_uint32(out, static_cast<std::uint32_t>(centroids[s][c].size()));
            for (float f : centroids[s][c]) {
                wal::write_float(out, f);
            }
        }
    }
}

PqCodebook PqCodebook::deserialize(std::istream& in) {
    PqCodebook cb;
    cb.M_pq = wal::read_uint32(in);
    cb.ksub = wal::read_uint32(in);
    cb.dim = wal::read_uint32(in);
    cb.centroids.resize(cb.M_pq, std::vector<std::vector<float>>(cb.ksub));
    for (std::uint32_t s = 0; s < cb.M_pq; ++s) {
        for (std::uint32_t c = 0; c < cb.ksub; ++c) {
            std::uint32_t len = wal::read_uint32(in);
            cb.centroids[s][c].resize(len);
            for (std::uint32_t d = 0; d < len; ++d) {
                cb.centroids[s][c][d] = wal::read_float(in);
            }
        }
    }
    return cb;
}

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "distance/kernel.hpp"
#include "hnsw/config.hpp"
#include "hnsw/graph.hpp"
#include "nlohmann/json.hpp"
#include "types.hpp"

std::string format_float(double val, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}

std::string left_align(const std::string& s, std::size_t w) {
    if (s.length() >= w) return s;
    return s + std::string(w - s.length(), ' ');
}

std::string right_align(const std::string& s, std::size_t w) {
    if (s.length() >= w) return s;
    return std::string(w - s.length(), ' ') + s;
}

std::vector<std::vector<float>> load_fvecs(const std::string& path, std::size_t& dim_out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open input file: " << path << std::endl;
        std::exit(1);
    }
    std::vector<std::vector<float>> data;
    std::uint32_t dim = 0;
    while (in.read(reinterpret_cast<char*>(&dim), sizeof(dim))) {
        std::vector<float> vec(dim);
        if (!in.read(reinterpret_cast<char*>(vec.data()), dim * sizeof(float))) {
            break;
        }
        data.push_back(std::move(vec));
    }
    if (!data.empty()) {
        dim_out = data[0].size();
    }
    return data;
}

std::vector<std::vector<float>> generate_synthetic(std::size_t n, std::size_t dim, Metric metric,
                                                   std::uint32_t seed) {
    std::mt19937 gen(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<std::vector<float>> data(n, std::vector<float>(dim));
    for (std::size_t i = 0; i < n; ++i) {
        float norm_sq = 0.0f;
        for (std::size_t d = 0; d < dim; ++d) {
            data[i][d] = dist(gen);
            norm_sq += data[i][d] * data[i][d];
        }
        if (metric == Metric::Cosine || metric == Metric::DotProduct) {
            float norm = std::sqrt(norm_sq);
            if (norm > 1e-9f) {
                for (std::size_t d = 0; d < dim; ++d) {
                    data[i][d] /= norm;
                }
            }
        }
    }
    return data;
}

void normalize_vectors(std::vector<std::vector<float>>& vectors) {
    for (auto& vec : vectors) {
        float norm_sq = 0.0f;
        for (float val : vec) {
            norm_sq += val * val;
        }
        float norm = std::sqrt(norm_sq);
        if (norm > 1e-9f) {
            for (float& val : vec) {
                val /= norm;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::size_t dim = 128;
    std::size_t n_train = 100000;
    std::size_t n_query = 1000;
    std::size_t k = 10;
    std::uint32_t duration = 5;
    std::vector<std::uint32_t> ef_list = {10, 50, 100, 200};
    Metric metric = Metric::L2;
    std::string metric_str = "L2";
    QuantizationMode quant_mode = QuantizationMode::None;
    std::string quant_str = "none";
    std::uint32_t pq_m = 8;
    std::string input_file = "";
    std::string output_file = "bench_results.json";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dim" && i + 1 < argc) {
            dim = std::stoull(argv[++i]);
        } else if (arg == "--n-train" && i + 1 < argc) {
            n_train = std::stoull(argv[++i]);
        } else if (arg == "--n-query" && i + 1 < argc) {
            n_query = std::stoull(argv[++i]);
        } else if (arg == "--k" && i + 1 < argc) {
            k = std::stoull(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = std::stoull(argv[++i]);
        } else if (arg == "--ef-list" && i + 1 < argc) {
            std::string list_str = argv[++i];
            ef_list.clear();
            std::stringstream ss(list_str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                if (!item.empty()) {
                    ef_list.push_back(std::stoul(item));
                }
            }
        } else if (arg == "--metric" && i + 1 < argc) {
            metric_str = argv[++i];
            if (metric_str == "L2" || metric_str == "l2") {
                metric = Metric::L2;
                metric_str = "L2";
            } else if (metric_str == "Cosine" || metric_str == "cosine" || metric_str == "COSINE") {
                metric = Metric::Cosine;
                metric_str = "Cosine";
            } else if (metric_str == "DotProduct" || metric_str == "dotproduct" ||
                       metric_str == "DOTPRODUCT" || metric_str == "dot" || metric_str == "Dot") {
                metric = Metric::DotProduct;
                metric_str = "DotProduct";
            } else if (metric_str == "L1" || metric_str == "l1") {
                metric = Metric::L1;
                metric_str = "L1";
            } else if (metric_str == "Hamming" || metric_str == "hamming") {
                metric = Metric::Hamming;
                metric_str = "Hamming";
            } else {
                std::cerr << "Unknown metric: " << metric_str << std::endl;
                return 1;
            }
        } else if (arg == "--quant" && i + 1 < argc) {
            quant_str = argv[++i];
            if (quant_str == "none" || quant_str == "None") {
                quant_mode = QuantizationMode::None;
                quant_str = "none";
            } else if (quant_str == "sq8" || quant_str == "SQ8") {
                quant_mode = QuantizationMode::SQ8;
                quant_str = "sq8";
            } else if (quant_str == "pq" || quant_str == "PQ") {
                quant_mode = QuantizationMode::PQ;
                quant_str = "pq";
            } else {
                std::cerr << "Unknown quantization mode: " << quant_str << std::endl;
                return 1;
            }
        } else if (arg == "--pq-m" && i + 1 < argc) {
            pq_m = std::stoul(argv[++i]);
        } else if (arg == "--input" && i + 1 < argc) {
            input_file = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        } else {
            std::cerr << "Unknown argument or missing value: " << arg << std::endl;
            return 1;
        }
    }

    std::vector<std::vector<float>> train_vectors;
    std::vector<std::vector<float>> query_vectors;

    if (!input_file.empty()) {
        std::size_t loaded_dim = 0;
        auto all_vectors = load_fvecs(input_file, loaded_dim);
        if (all_vectors.empty()) {
            std::cerr << "Empty or invalid fvecs file: " << input_file << std::endl;
            return 1;
        }
        dim = loaded_dim;
        if (all_vectors.size() < n_train + n_query) {
            std::cerr << "Warning: .fvecs file only contains " << all_vectors.size()
                      << " vectors, but n_train=" << n_train << " and n_query=" << n_query
                      << " require " << (n_train + n_query) << " vectors. Adjusting counts."
                      << std::endl;
            if (all_vectors.size() <= n_query) {
                std::cerr << "Error: File too small even for query vectors." << std::endl;
                return 1;
            }
            n_train = all_vectors.size() - n_query;
        }

        train_vectors.assign(all_vectors.begin(), all_vectors.begin() + n_train);
        query_vectors.assign(all_vectors.begin() + n_train,
                             all_vectors.begin() + n_train + n_query);

        if (metric == Metric::Cosine || metric == Metric::DotProduct) {
            normalize_vectors(train_vectors);
            normalize_vectors(query_vectors);
        }
    } else {
        train_vectors = generate_synthetic(n_train, dim, metric, 42);
        query_vectors = generate_synthetic(n_query, dim, metric, 1337);
    }

    HnswConfig config;
    config.dim = dim;
    config.metric = metric;
    config.quant_mode = quant_mode;
    config.pq_m = pq_m;

    HnswGraph graph(config);

    std::cerr << "Building HNSW index with " << n_train << " vectors..." << std::endl;
    for (std::size_t i = 0; i < n_train; ++i) {
        graph.upsert(static_cast<VectorId>(i), train_vectors[i]);
    }

    if (config.quant_mode != QuantizationMode::None) {
        std::cerr << "Training quantization (" << quant_str << ")..." << std::endl;
        graph.train_quantization();
    }

    std::cerr << "Computing brute-force ground truth for " << n_query << " queries..." << std::endl;
    KernelFn main_dist_fn = get_kernel(metric);
    std::size_t num_threads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::vector<VectorId>> gt_ids(n_query);

    {
        auto worker = [&](std::size_t start_q, std::size_t end_q) {
            KernelFn dist_fn = main_dist_fn;
            for (std::size_t q = start_q; q < end_q; ++q) {
                std::span<const float> query_vec = query_vectors[q];
                std::vector<std::pair<float, VectorId>> dists;
                dists.reserve(n_train);
                for (std::size_t i = 0; i < n_train; ++i) {
                    float d = dist_fn(query_vec, train_vectors[i]);
                    dists.push_back({d, static_cast<VectorId>(i)});
                }
                std::sort(dists.begin(), dists.end(),
                          [](const auto& a, const auto& b) { return a.first < b.first; });
                std::vector<VectorId> top_k_ids;
                top_k_ids.reserve(k);
                for (std::size_t i = 0; i < k && i < dists.size(); ++i) {
                    top_k_ids.push_back(dists[i].second);
                }
                gt_ids[q] = std::move(top_k_ids);
            }
        };

        std::vector<std::thread> threads;
        std::size_t chunk_size = (n_query + num_threads - 1) / num_threads;
        for (std::size_t t = 0; t < num_threads; ++t) {
            std::size_t start_q = t * chunk_size;
            std::size_t end_q = std::min(start_q + chunk_size, n_query);
            if (start_q < end_q) {
                threads.emplace_back(worker, start_q, end_q);
            }
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    struct BenchResultCase {
        std::uint32_t ef;
        double qps;
        double recall;
    };
    std::vector<BenchResultCase> results;

    for (std::uint32_t ef : ef_list) {
        std::cerr << "Running search benchmark for ef=" << ef << "..." << std::endl;

        bool use_duration = (duration > 0);
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration);

        std::size_t query_idx = 0;
        std::size_t total_queries = 0;
        double total_recall = 0.0;

        while (true) {
            if (use_duration) {
                if (std::chrono::steady_clock::now() >= end_time) {
                    break;
                }
            } else {
                if (total_queries >= n_query) {
                    break;
                }
            }

            std::span<const float> query_vec = query_vectors[query_idx];
            auto search_results = graph.search(query_vec, k, ef);

            std::size_t hits = 0;
            const auto& gt = gt_ids[query_idx];
            for (const auto& res : search_results) {
                if (std::find(gt.begin(), gt.end(), res.id) != gt.end()) {
                    hits++;
                }
            }
            total_recall += static_cast<double>(hits) / k;

            total_queries++;
            query_idx = (query_idx + 1) % n_query;
        }

        auto actual_end = std::chrono::steady_clock::now();
        auto actual_duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(actual_end - start_time).count();

        double qps = (actual_duration_ms > 0)
                         ? (static_cast<double>(total_queries) * 1000.0 / actual_duration_ms)
                         : 0.0;
        double mean_recall = (total_queries > 0) ? (total_recall / total_queries) : 0.0;

        results.push_back({ef, qps, mean_recall});
    }

    std::string recall_header = "Recall@" + std::to_string(k);
    std::size_t recall_width = recall_header.length() + 2;
    std::string line_border = "+" + std::string(8, '-') + "+" + std::string(10, '-') + "+" +
                              std::string(recall_width, '-') + "+";

    std::cout << "metric=" << metric_str << "  dim=" << dim << "  n_train=" << n_train
              << "  n_query=" << n_query << "  k=" << k << "  quant=" << quant_str << std::endl;

    std::cout << line_border << std::endl;
    std::cout << "|   ef   |  QPS     | " << recall_header << " |" << std::endl;
    std::cout << line_border << std::endl;

    for (const auto& res : results) {
        std::string ef_str = right_align(std::to_string(res.ef), 7) + " ";
        std::string qps_str = " " + left_align(format_float(res.qps, 1), 8);
        std::string recall_str = right_align(format_float(res.recall, 4), recall_width - 1) + " ";

        std::cout << "|" << ef_str << "|" << qps_str << "|" << recall_str << "|" << std::endl;
    }
    std::cout << line_border << std::endl;

    nlohmann::json j;
    j["config"] = {
        {"dim", dim}, {"n_train", n_train}, {"k", k}, {"metric", metric_str}, {"quant", quant_str}};

    nlohmann::json results_arr = nlohmann::json::array();
    for (const auto& res : results) {
        results_arr.push_back({{"ef", res.ef}, {"qps", res.qps}, {"recall_at_k", res.recall}});
    }
    j["results"] = results_arr;

    std::ofstream out(output_file);
    if (out) {
        out << j.dump(2) << std::endl;
    } else {
        std::cerr << "Failed to write JSON output to " << output_file << std::endl;
    }

    return 0;
}

#include "server/http_service.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>

#include "server/logger.hpp"
#include "server/metrics.hpp"
#include "server/web_assets.hpp"

using json = nlohmann::json;

namespace {

Metric parse_metric(const std::string& name) {
    if (name == "cosine" || name == "Cosine") return Metric::Cosine;
    if (name == "l2" || name == "L2") return Metric::L2;
    if (name == "dot_product" || name == "DotProduct" || name == "dot" || name == "Dot")
        return Metric::DotProduct;
    if (name == "l1" || name == "L1") return Metric::L1;
    if (name == "hamming" || name == "Hamming") return Metric::Hamming;
    throw std::invalid_argument("Unknown metric: " + name);
}

std::string metric_to_string(Metric metric) {
    switch (metric) {
        case Metric::Cosine:
            return "Cosine";
        case Metric::L2:
            return "L2";
        case Metric::DotProduct:
            return "DotProduct";
        case Metric::L1:
            return "L1";
        case Metric::Hamming:
            return "Hamming";
    }
    return "Unknown";
}

MetadataValue json_to_value(const json& j) {
    if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_number()) {
        return j.get<double>();
    } else if (j.is_boolean()) {
        return j.get<bool>();
    }
    throw std::invalid_argument("Invalid metadata value type");
}

json value_to_json(const MetadataValue& val) {
    return std::visit([](const auto& arg) -> json { return arg; }, val);
}

Filter json_to_filter(const json& j) {
    if (j.is_null() || !j.contains("type")) {
        return std::monostate{};
    }
    std::string type = j["type"];
    if (type == "eq") {
        return EqFilter{j.at("key"), json_to_value(j.at("value"))};
    } else if (type == "range") {
        return RangeFilter{j.at("key"), j.at("low").get<double>(), j.at("high").get<double>()};
    } else if (type == "and") {
        std::vector<Filter> conds;
        for (const auto& c : j.at("conditions")) {
            conds.push_back(json_to_filter(c));
        }
        return AndFilter{std::move(conds)};
    } else if (type == "or") {
        std::vector<Filter> conds;
        for (const auto& c : j.at("conditions")) {
            conds.push_back(json_to_filter(c));
        }
        return OrFilter{std::move(conds)};
    } else if (type == "not") {
        return NotFilter{std::make_shared<Filter>(json_to_filter(j.at("condition")))};
    } else if (type == "compare") {
        std::string op_str = j.at("op");
        CompareOp op = CompareOp::Lt;
        if (op_str == "gt")
            op = CompareOp::Gt;
        else if (op_str == "lte")
            op = CompareOp::Lte;
        else if (op_str == "gte")
            op = CompareOp::Gte;
        return CompareFilter{j.at("key"), op, json_to_value(j.at("value"))};
    } else if (type == "in" || type == "notin") {
        std::vector<MetadataValue> vals;
        for (const auto& val_json : j.at("values")) {
            vals.push_back(json_to_value(val_json));
        }
        bool is_not = (type == "notin");
        if (j.contains("not")) {
            is_not = j["not"].get<bool>();
        }
        return InFilter{j.at("key"), std::move(vals), is_not};
    }
    throw std::invalid_argument("Unknown filter type: " + type);
}

void send_error(httplib::Response& res, int status, const std::string& msg) {
    Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
    log_error(msg, {{"status", std::to_string(status)}});
    json err;
    err["error"] = msg;
    res.status = status;
    res.set_content(err.dump(), "application/json");
}

}  // namespace

HttpService::HttpService(CommandHandler& handler, const std::string& host, int port)
    : handler_(handler), host_(host), port_(port), start_time_(std::chrono::steady_clock::now()) {
    setup_routes();
}

HttpService::~HttpService() { stop(); }

void HttpService::start() {
    thread_ = std::make_unique<std::thread>([this]() { svr_.listen(host_, port_); });
}

void HttpService::stop() {
    if (thread_ && thread_->joinable()) {
        svr_.stop();
        thread_->join();
        thread_.reset();
    }
}

void HttpService::setup_routes() {
    svr_.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(INDEX_HTML, "text/html");
    });

    svr_.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        (void)req;
        try {
            auto uptime = std::chrono::steady_clock::now() - start_time_;
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
            json j;
            j["status"] = "ok";
            j["uptime_s"] = static_cast<std::uint64_t>(secs);
            res.status = 200;
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            send_error(res, 500, e.what());
        }
    });

    svr_.Get("/stats", [this](const httplib::Request& req, httplib::Response& res) {
        (void)req;
        try {
            auto snap = Metrics::instance().snapshot();
            auto names = handler_.list_namespaces();
            std::uint64_t total_vectors = 0;
            for (const auto& ns : names) {
                try {
                    total_vectors += handler_.get_namespace(ns).size;
                } catch (...) {
                }
            }
            json j;
            j["namespace_count"] = names.size();
            j["total_vectors"] = total_vectors;
            j["upserts"] = snap.upserts;
            j["searches"] = snap.searches;
            j["deletes"] = snap.deletes;
            j["errors"] = snap.errors;
            res.status = 200;
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            send_error(res, 500, e.what());
        }
    });

    svr_.Get("/namespaces", [this](const httplib::Request& req, httplib::Response& res) {
        (void)req;
        try {
            auto list = handler_.list_namespaces();
            json j = list;
            res.status = 200;
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            send_error(res, 500, e.what());
        }
    });

    svr_.Get(R"(/namespaces/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string name = req.matches[1];
            auto info = handler_.get_namespace(name);
            std::string qmode = "None";
            if (info.config.quant_mode == QuantizationMode::SQ8) {
                qmode = "SQ8";
            } else if (info.config.quant_mode == QuantizationMode::PQ) {
                qmode = "PQ";
            }

            json j;
            j["size"] = info.size;
            j["config"] = {{"dim", info.config.dim},
                           {"m", info.config.M},
                           {"m0", info.config.M0},
                           {"ef_construction", info.config.ef_construction},
                           {"metric", metric_to_string(info.config.metric)},
                           {"max_elements", info.config.max_elements},
                           {"quant_mode", qmode},
                           {"pq_m", info.config.pq_m}};
            res.status = 200;
            res.set_content(j.dump(), "application/json");
        } catch (const std::out_of_range& e) {
            send_error(res, 404, e.what());
        } catch (const std::invalid_argument& e) {
            send_error(res, 400, e.what());
        } catch (const std::exception& e) {
            send_error(res, 500, e.what());
        }
    });

    svr_.Post("/namespaces", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::string name = j.at("name");
            auto config_json = j.at("config");
            HnswConfig config;
            config.dim = config_json.at("dim").get<uint32_t>();
            config.M = config_json.at("m").get<uint32_t>();
            config.M0 = config_json.at("m0").get<uint32_t>();
            config.ef_construction = config_json.at("ef_construction").get<uint32_t>();
            config.metric = parse_metric(config_json.at("metric").get<std::string>());
            config.max_elements = config_json.at("max_elements").get<uint32_t>();

            if (config_json.contains("quant_mode")) {
                std::string mode = config_json.at("quant_mode").get<std::string>();
                if (mode == "none" || mode == "None") {
                    config.quant_mode = QuantizationMode::None;
                } else if (mode == "sq8" || mode == "SQ8") {
                    config.quant_mode = QuantizationMode::SQ8;
                } else if (mode == "pq" || mode == "PQ") {
                    config.quant_mode = QuantizationMode::PQ;
                } else {
                    throw std::invalid_argument("Unknown quantization mode: " + mode);
                }
            }
            if (config_json.contains("pq_m")) {
                config.pq_m = config_json.at("pq_m").get<uint32_t>();
            }

            handler_.create_namespace(name, config);
            res.status = 200;
            res.set_content("{}", "application/json");
        } catch (const json::exception& e) {
            send_error(res, 400, std::string("JSON error: ") + e.what());
        } catch (const std::invalid_argument& e) {
            send_error(res, 400, e.what());
        } catch (const std::exception& e) {
            send_error(res, 500, e.what());
        }
    });

    svr_.Delete(R"(/namespaces/([^/]+))",
                [this](const httplib::Request& req, httplib::Response& res) {
                    try {
                        std::string name = req.matches[1];
                        handler_.drop_namespace(name);
                        res.status = 200;
                        res.set_content("{}", "application/json");
                    } catch (const std::out_of_range& e) {
                        send_error(res, 404, e.what());
                    } catch (const std::exception& e) {
                        send_error(res, 500, e.what());
                    }
                });

    svr_.Post(R"(/namespaces/([^/]+)/vectors)",
              [this](const httplib::Request& req, httplib::Response& res) {
                  try {
                      std::string ns = req.matches[1];
                      auto j = json::parse(req.body);
                      VectorId id = j.at("id").get<VectorId>();
                      std::vector<float> vec = j.at("vector").get<std::vector<float>>();
                      Metadata meta;
                      if (j.contains("metadata") && j["metadata"].is_object()) {
                          for (auto& [k, v] : j["metadata"].items()) {
                              meta[k] = json_to_value(v);
                          }
                      }
                      handler_.upsert(ns, id, vec, std::move(meta));
                      res.status = 200;
                      res.set_content("{}", "application/json");
                  } catch (const json::exception& e) {
                      send_error(res, 400, std::string("JSON error: ") + e.what());
                  } catch (const std::out_of_range& e) {
                      send_error(res, 404, e.what());
                  } catch (const std::invalid_argument& e) {
                      send_error(res, 400, e.what());
                  } catch (const std::exception& e) {
                      send_error(res, 500, e.what());
                  }
              });

    svr_.Post(R"(/namespaces/([^/]+)/vectors/batch)", [this](const httplib::Request& req,
                                                             httplib::Response& res) {
        try {
            std::string ns = req.matches[1];
            auto j = json::parse(req.body);
            auto entries_json = j.at("entries");
            std::vector<UpsertEntry> entries;
            entries.reserve(entries_json.size());
            for (const auto& entry_json : entries_json) {
                VectorId id = entry_json.at("id").get<VectorId>();
                std::vector<float> vec = entry_json.at("vector").get<std::vector<float>>();
                Metadata meta;
                if (entry_json.contains("metadata") && entry_json["metadata"].is_object()) {
                    for (auto& [k, v] : entry_json["metadata"].items()) {
                        meta[k] = json_to_value(v);
                    }
                }
                entries.push_back(UpsertEntry{id, std::move(vec), std::move(meta)});
            }
            handler_.batch_upsert(ns, std::move(entries));
            res.status = 200;
            res.set_content("{}", "application/json");
        } catch (const json::exception& e) {
            send_error(res, 400, std::string("JSON error: ") + e.what());
        } catch (const std::out_of_range& e) {
            send_error(res, 404, e.what());
        } catch (const std::invalid_argument& e) {
            send_error(res, 400, e.what());
        } catch (const std::exception& e) {
            send_error(res, 500, e.what());
        }
    });

    svr_.Delete(R"(/namespaces/([^/]+)/vectors/([0-9]+))",
                [this](const httplib::Request& req, httplib::Response& res) {
                    try {
                        std::string ns = req.matches[1];
                        VectorId id = std::stoull(req.matches[2]);
                        handler_.remove(ns, id);
                        res.status = 200;
                        res.set_content("{}", "application/json");
                    } catch (const std::out_of_range& e) {
                        send_error(res, 404, e.what());
                    } catch (const std::invalid_argument& e) {
                        send_error(res, 400, e.what());
                    } catch (const std::exception& e) {
                        send_error(res, 500, e.what());
                    }
                });

    svr_.Post(R"(/namespaces/([^/]+)/train)",
              [this](const httplib::Request& req, httplib::Response& res) {
                  try {
                      std::string ns = req.matches[1];
                      handler_.train(ns);
                      res.status = 200;
                      res.set_content("{}", "application/json");
                  } catch (const std::out_of_range& e) {
                      send_error(res, 404, e.what());
                  } catch (const std::invalid_argument& e) {
                      send_error(res, 400, e.what());
                  } catch (const std::exception& e) {
                      send_error(res, 500, e.what());
                  }
              });

    svr_.Post(R"(/namespaces/([^/]+)/compact)",
              [this](const httplib::Request& req, httplib::Response& res) {
                  try {
                      std::string ns = req.matches[1];
                      handler_.compact(ns);
                      res.status = 200;
                      res.set_content("{}", "application/json");
                  } catch (const std::out_of_range& e) {
                      send_error(res, 404, e.what());
                  } catch (const std::invalid_argument& e) {
                      send_error(res, 400, e.what());
                  } catch (const std::exception& e) {
                      send_error(res, 500, e.what());
                  }
              });

    svr_.Post(R"(/namespaces/([^/]+)/search)",
              [this](const httplib::Request& req, httplib::Response& res) {
                  try {
                      std::string ns = req.matches[1];
                      auto j = json::parse(req.body);
                      std::vector<float> query = j.at("query").get<std::vector<float>>();
                      uint32_t k = j.at("k").get<uint32_t>();
                      uint32_t ef = j.at("ef").get<uint32_t>();
                      Filter filter = std::monostate{};
                      if (j.contains("filter")) {
                          filter = json_to_filter(j["filter"]);
                      }

                      auto search_results = handler_.search(ns, query, k, ef, filter);

                      json response_json = json::array();
                      for (const auto& hit : search_results) {
                          json hit_json;
                          hit_json["id"] = hit.id;
                          hit_json["distance"] = hit.distance;
                          json meta_json = json::object();
                          for (const auto& [key, value] : hit.meta) {
                              meta_json[key] = value_to_json(value);
                          }
                          hit_json["metadata"] = meta_json;
                          response_json.push_back(hit_json);
                      }

                      res.status = 200;
                      res.set_content(response_json.dump(), "application/json");
                  } catch (const json::exception& e) {
                      send_error(res, 400, std::string("JSON error: ") + e.what());
                  } catch (const std::out_of_range& e) {
                      send_error(res, 404, e.what());
                  } catch (const std::invalid_argument& e) {
                      send_error(res, 400, e.what());
                  } catch (const std::exception& e) {
                      send_error(res, 500, e.what());
                  }
              });
}

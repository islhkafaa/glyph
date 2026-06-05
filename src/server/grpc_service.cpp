#include "server/grpc_service.hpp"

#include <stdexcept>
#include <type_traits>

#include "server/metrics.hpp"

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

QuantizationMode parse_quant_mode(const std::string& name) {
    if (name == "none" || name == "None" || name.empty()) return QuantizationMode::None;
    if (name == "sq8" || name == "SQ8") return QuantizationMode::SQ8;
    if (name == "pq" || name == "PQ") return QuantizationMode::PQ;
    throw std::invalid_argument("Unknown quantization mode: " + name);
}

std::string quant_mode_to_string(QuantizationMode mode) {
    switch (mode) {
        case QuantizationMode::None:
            return "None";
        case QuantizationMode::SQ8:
            return "SQ8";
        case QuantizationMode::PQ:
            return "PQ";
    }
    return "Unknown";
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

MetadataValue proto_to_value(const glyph::Value& proto_val) {
    switch (proto_val.val_case()) {
        case glyph::Value::kStringVal:
            return proto_val.string_val();
        case glyph::Value::kDoubleVal:
            return proto_val.double_val();
        case glyph::Value::kBoolVal:
            return proto_val.bool_val();
        default:
            throw std::invalid_argument("Metadata value not set");
    }
}

glyph::Value value_to_proto(const MetadataValue& val) {
    glyph::Value proto_val;
    std::visit(
        [&proto_val](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                proto_val.set_string_val(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                proto_val.set_double_val(arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                proto_val.set_bool_val(arg);
            }
        },
        val);
    return proto_val;
}

Filter proto_to_filter(const glyph::Filter& proto_filter) {
    switch (proto_filter.filter_type_case()) {
        case glyph::Filter::kEq: {
            const auto& eq = proto_filter.eq();
            return EqFilter{eq.key(), proto_to_value(eq.value())};
        }
        case glyph::Filter::kRange: {
            const auto& range = proto_filter.range();
            return RangeFilter{range.key(), range.low(), range.high()};
        }
        case glyph::Filter::kAnd: {
            const auto& and_proto = proto_filter.and_();
            std::vector<Filter> conds;
            conds.reserve(and_proto.conditions_size());
            for (int i = 0; i < and_proto.conditions_size(); ++i) {
                conds.push_back(proto_to_filter(and_proto.conditions(i)));
            }
            return AndFilter{std::move(conds)};
        }
        case glyph::Filter::kOr: {
            const auto& or_proto = proto_filter.or_();
            std::vector<Filter> conds;
            conds.reserve(or_proto.conditions_size());
            for (int i = 0; i < or_proto.conditions_size(); ++i) {
                conds.push_back(proto_to_filter(or_proto.conditions(i)));
            }
            return OrFilter{std::move(conds)};
        }
        case glyph::Filter::kNot: {
            const auto& not_proto = proto_filter.not_();
            return NotFilter{std::make_shared<Filter>(proto_to_filter(not_proto.condition()))};
        }
        case glyph::Filter::kCompare: {
            const auto& comp = proto_filter.compare();
            CompareOp op = CompareOp::Lt;
            if (comp.op() == "gt")
                op = CompareOp::Gt;
            else if (comp.op() == "lte")
                op = CompareOp::Lte;
            else if (comp.op() == "gte")
                op = CompareOp::Gte;
            return CompareFilter{comp.key(), op, proto_to_value(comp.value())};
        }
        case glyph::Filter::kIn: {
            const auto& in_proto = proto_filter.in();
            std::vector<MetadataValue> vals;
            vals.reserve(in_proto.values_size());
            for (int i = 0; i < in_proto.values_size(); ++i) {
                vals.push_back(proto_to_value(in_proto.values(i)));
            }
            return InFilter{in_proto.key(), std::move(vals), in_proto.is_not()};
        }
        default:
            return std::monostate{};
    }
}

}  // namespace

GlyphServiceImpl::GlyphServiceImpl(CommandHandler& handler)
    : handler_(handler), start_time_(std::chrono::steady_clock::now()) {}

grpc::Status GlyphServiceImpl::CreateNamespace(grpc::ServerContext* context,
                                               const glyph::CreateNamespaceRequest* request,
                                               glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        const auto& config_proto = request->config();
        HnswConfig config;
        config.dim = config_proto.dim();
        config.M = config_proto.m();
        config.M0 = config_proto.m0();
        config.ef_construction = config_proto.ef_construction();
        config.metric = parse_metric(config_proto.metric());
        config.max_elements = config_proto.max_elements();
        config.quant_mode = parse_quant_mode(config_proto.quant_mode());
        if (config_proto.pq_m() > 0) {
            config.pq_m = config_proto.pq_m();
        }

        handler_.create_namespace(request->name(), config);
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::DropNamespace(grpc::ServerContext* context,
                                             const glyph::DropNamespaceRequest* request,
                                             glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        handler_.drop_namespace(request->name());
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Upsert(grpc::ServerContext* context,
                                      const glyph::UpsertRequest* request, glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        std::vector<float> vec(request->vector().begin(), request->vector().end());
        Metadata meta;
        for (const auto& [k, v] : request->metadata()) {
            meta[k] = proto_to_value(v);
        }
        handler_.upsert(request->namespace_(), request->id(), vec, std::move(meta));
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::BatchUpsert(grpc::ServerContext* context,
                                           const glyph::BatchUpsertRequest* request,
                                           glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        std::vector<UpsertEntry> entries;
        entries.reserve(request->entries_size());
        for (const auto& entry : request->entries()) {
            std::vector<float> vec(entry.vector().begin(), entry.vector().end());
            Metadata meta;
            for (const auto& [k, v] : entry.metadata()) {
                meta[k] = proto_to_value(v);
            }
            entries.push_back(UpsertEntry{entry.id(), std::move(vec), std::move(meta)});
        }
        handler_.batch_upsert(request->namespace_(), std::move(entries));
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Delete(grpc::ServerContext* context,
                                      const glyph::DeleteRequest* request, glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        handler_.remove(request->namespace_(), request->id());
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Train(grpc::ServerContext* context,
                                     const glyph::TrainRequest* request, glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        handler_.train(request->namespace_());
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Compact(grpc::ServerContext* context,
                                       const glyph::CompactRequest* request,
                                       glyph::Empty* response) {
    (void)context;
    (void)response;
    try {
        handler_.compact(request->namespace_());
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Search(grpc::ServerContext* context,
                                      const glyph::SearchRequest* request,
                                      glyph::SearchResponse* response) {
    (void)context;
    try {
        std::vector<float> query(request->query().begin(), request->query().end());
        Filter filter = proto_to_filter(request->filter());

        auto results =
            handler_.search(request->namespace_(), query, request->k(), request->ef(), filter);

        for (const auto& res : results) {
            auto* hit = response->add_hits();
            hit->set_id(res.id);
            hit->set_distance(res.distance);
            auto* meta_map = hit->mutable_metadata();
            for (const auto& [k, v] : res.meta) {
                (*meta_map)[k] = value_to_proto(v);
            }
        }
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::ListNamespaces(grpc::ServerContext* context,
                                              const glyph::Empty* request,
                                              glyph::ListNamespacesResponse* response) {
    (void)context;
    (void)request;
    try {
        auto names = handler_.list_namespaces();
        for (const auto& name : names) {
            response->add_names(name);
        }
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::GetNamespace(grpc::ServerContext* context,
                                            const glyph::GetNamespaceRequest* request,
                                            glyph::NamespaceInfoProto* response) {
    (void)context;
    try {
        auto info = handler_.get_namespace(request->name());
        auto* config_proto = response->mutable_config();
        config_proto->set_dim(info.config.dim);
        config_proto->set_m(info.config.M);
        config_proto->set_m0(info.config.M0);
        config_proto->set_ef_construction(info.config.ef_construction);
        config_proto->set_metric(metric_to_string(info.config.metric));
        config_proto->set_max_elements(info.config.max_elements);
        config_proto->set_quant_mode(quant_mode_to_string(info.config.quant_mode));
        config_proto->set_pq_m(info.config.pq_m);
        response->set_size(info.size);
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Health(grpc::ServerContext* context, const glyph::Empty* request,
                                      glyph::HealthResponse* response) {
    (void)context;
    (void)request;
    try {
        response->set_status("ok");
        auto duration = std::chrono::steady_clock::now() - start_time_;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        response->set_uptime_s(static_cast<std::uint64_t>(seconds));
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

grpc::Status GlyphServiceImpl::Stats(grpc::ServerContext* context, const glyph::Empty* request,
                                     glyph::StatsResponse* response) {
    (void)context;
    (void)request;
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
        response->set_namespace_count(names.size());
        response->set_total_vectors(total_vectors);
        response->set_upserts(snap.upserts);
        response->set_searches(snap.searches);
        response->set_deletes(snap.deletes);
        response->set_errors(snap.errors);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        Metrics::instance().errors.fetch_add(1, std::memory_order_relaxed);
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

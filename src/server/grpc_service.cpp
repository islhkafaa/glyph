#include "server/grpc_service.hpp"

#include <stdexcept>
#include <type_traits>

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
        default:
            return std::monostate{};
    }
}

}  // namespace

GlyphServiceImpl::GlyphServiceImpl(CommandHandler& handler) : handler_(handler) {}

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

        handler_.create_namespace(request->name(), config);
        return grpc::Status::OK;
    } catch (const std::out_of_range& e) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
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
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
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
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
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
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
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
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
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
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    } catch (const std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (...) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Unknown error");
    }
}

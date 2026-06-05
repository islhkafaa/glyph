#pragma once

#include <grpcpp/grpcpp.h>

#include <chrono>

#include "glyph.grpc.pb.h"
#include "server/handler.hpp"

class GlyphServiceImpl final : public glyph::GlyphService::Service {
   public:
    explicit GlyphServiceImpl(CommandHandler& handler);

    grpc::Status CreateNamespace(grpc::ServerContext* context,
                                 const glyph::CreateNamespaceRequest* request,
                                 glyph::Empty* response) override;

    grpc::Status DropNamespace(grpc::ServerContext* context,
                               const glyph::DropNamespaceRequest* request,
                               glyph::Empty* response) override;

    grpc::Status Upsert(grpc::ServerContext* context, const glyph::UpsertRequest* request,
                        glyph::Empty* response) override;

    grpc::Status BatchUpsert(grpc::ServerContext* context, const glyph::BatchUpsertRequest* request,
                             glyph::Empty* response) override;

    grpc::Status Delete(grpc::ServerContext* context, const glyph::DeleteRequest* request,
                        glyph::Empty* response) override;

    grpc::Status Train(grpc::ServerContext* context, const glyph::TrainRequest* request,
                       glyph::Empty* response) override;

    grpc::Status Compact(grpc::ServerContext* context, const glyph::CompactRequest* request,
                         glyph::Empty* response) override;

    grpc::Status Search(grpc::ServerContext* context, const glyph::SearchRequest* request,
                        glyph::SearchResponse* response) override;

    grpc::Status ListNamespaces(grpc::ServerContext* context, const glyph::Empty* request,
                                glyph::ListNamespacesResponse* response) override;

    grpc::Status GetNamespace(grpc::ServerContext* context,
                              const glyph::GetNamespaceRequest* request,
                              glyph::NamespaceInfoProto* response) override;

    grpc::Status Health(grpc::ServerContext* context, const glyph::Empty* request,
                        glyph::HealthResponse* response) override;

    grpc::Status Stats(grpc::ServerContext* context, const glyph::Empty* request,
                       glyph::StatsResponse* response) override;

   private:
    CommandHandler& handler_;
    std::chrono::steady_clock::time_point start_time_;
};

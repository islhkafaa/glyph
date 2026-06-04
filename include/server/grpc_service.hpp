#pragma once

#include <grpcpp/grpcpp.h>

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

    grpc::Status Search(grpc::ServerContext* context, const glyph::SearchRequest* request,
                        glyph::SearchResponse* response) override;

   private:
    CommandHandler& handler_;
};

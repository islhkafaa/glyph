#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "glyph.grpc.pb.h"
#include "persistence/engine.hpp"
#include "server/grpc_service.hpp"
#include "server/handler.hpp"

class GrpcServerTest : public ::testing::Test {
   protected:
    std::filesystem::path temp_dir;
    std::unique_ptr<StorageEngine> engine;
    std::unique_ptr<CommandHandler> handler;
    std::unique_ptr<GlyphServiceImpl> service;
    std::unique_ptr<grpc::Server> server;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<glyph::GlyphService::Stub> stub;

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(100000000, 999999999);
        temp_dir = std::filesystem::temp_directory_path() /
                   ("glyph_grpc_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir);

        engine = std::make_unique<StorageEngine>(temp_dir);
        handler = std::make_unique<CommandHandler>(*engine);
        service = std::make_unique<GlyphServiceImpl>(*handler);

        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:50052", grpc::InsecureServerCredentials());
        builder.RegisterService(service.get());
        server = builder.BuildAndStart();

        channel = grpc::CreateChannel("127.0.0.1:50052", grpc::InsecureChannelCredentials());
        stub = glyph::GlyphService::NewStub(channel);
    }

    void TearDown() override {
        stub.reset();
        channel.reset();
        if (server) {
            server->Shutdown();
        }
        service.reset();
        handler.reset();
        engine.reset();
        std::filesystem::remove_all(temp_dir);
    }
};

TEST_F(GrpcServerTest, CreateAndSearchRoundTrip) {
    grpc::ClientContext context1;
    glyph::CreateNamespaceRequest create_req;
    create_req.set_name("ns1");
    auto* config = create_req.mutable_config();
    config->set_dim(4);
    config->set_m(8);
    config->set_m0(16);
    config->set_ef_construction(100);
    config->set_metric("l2");
    config->set_max_elements(1000);
    glyph::Empty empty_resp;

    grpc::Status status = stub->CreateNamespace(&context1, create_req, &empty_resp);
    ASSERT_TRUE(status.ok()) << status.error_message();

    grpc::ClientContext context2;
    glyph::UpsertRequest upsert_req;
    upsert_req.set_namespace_("ns1");
    upsert_req.set_id(1);
    upsert_req.add_vector(1.0f);
    upsert_req.add_vector(0.0f);
    upsert_req.add_vector(0.0f);
    upsert_req.add_vector(0.0f);

    status = stub->Upsert(&context2, upsert_req, &empty_resp);
    ASSERT_TRUE(status.ok()) << status.error_message();

    grpc::ClientContext context3;
    glyph::SearchRequest search_req;
    search_req.set_namespace_("ns1");
    search_req.add_query(1.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.set_k(1);
    search_req.set_ef(10);
    glyph::SearchResponse search_resp;

    status = stub->Search(&context3, search_req, &search_resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    ASSERT_EQ(search_resp.hits_size(), 1);
    EXPECT_EQ(search_resp.hits(0).id(), 1);
    EXPECT_LT(search_resp.hits(0).distance(), 1e-5);
}

TEST_F(GrpcServerTest, DeleteViaGrpc) {
    grpc::ClientContext context1;
    glyph::CreateNamespaceRequest create_req;
    create_req.set_name("ns2");
    auto* config = create_req.mutable_config();
    config->set_dim(4);
    config->set_m(8);
    config->set_m0(16);
    config->set_ef_construction(100);
    config->set_metric("l2");
    config->set_max_elements(1000);
    glyph::Empty empty_resp;

    grpc::Status status = stub->CreateNamespace(&context1, create_req, &empty_resp);
    ASSERT_TRUE(status.ok());

    grpc::ClientContext context2;
    glyph::UpsertRequest upsert_req;
    upsert_req.set_namespace_("ns2");
    upsert_req.set_id(2);
    upsert_req.add_vector(0.0f);
    upsert_req.add_vector(1.0f);
    upsert_req.add_vector(0.0f);
    upsert_req.add_vector(0.0f);

    status = stub->Upsert(&context2, upsert_req, &empty_resp);
    ASSERT_TRUE(status.ok());

    grpc::ClientContext context3;
    glyph::DeleteRequest delete_req;
    delete_req.set_namespace_("ns2");
    delete_req.set_id(2);
    status = stub->Delete(&context3, delete_req, &empty_resp);
    ASSERT_TRUE(status.ok());

    grpc::ClientContext context4;
    glyph::SearchRequest search_req;
    search_req.set_namespace_("ns2");
    search_req.add_query(0.0f);
    search_req.add_query(1.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.set_k(1);
    search_req.set_ef(10);
    glyph::SearchResponse search_resp;

    status = stub->Search(&context4, search_req, &search_resp);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(search_resp.hits_size(), 0);
}

TEST_F(GrpcServerTest, UnknownNamespaceReturnsNotFound) {
    grpc::ClientContext context;
    glyph::SearchRequest search_req;
    search_req.set_namespace_("does_not_exist");
    search_req.add_query(1.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.set_k(1);
    search_req.set_ef(10);
    glyph::SearchResponse search_resp;

    grpc::Status status = stub->Search(&context, search_req, &search_resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST_F(GrpcServerTest, BatchUpsertAndSearch) {
    grpc::ClientContext context1;
    glyph::CreateNamespaceRequest create_req;
    create_req.set_name("ns3");
    auto* config = create_req.mutable_config();
    config->set_dim(4);
    config->set_m(8);
    config->set_m0(16);
    config->set_ef_construction(100);
    config->set_metric("l2");
    config->set_max_elements(1000);
    glyph::Empty empty_resp;

    grpc::Status status = stub->CreateNamespace(&context1, create_req, &empty_resp);
    ASSERT_TRUE(status.ok());

    grpc::ClientContext context2;
    glyph::BatchUpsertRequest batch_req;
    batch_req.set_namespace_("ns3");

    for (uint64_t i = 10; i < 15; ++i) {
        auto* entry = batch_req.add_entries();
        entry->set_id(i);
        entry->add_vector(static_cast<float>(i));
        entry->add_vector(0.0f);
        entry->add_vector(0.0f);
        entry->add_vector(0.0f);
    }

    status = stub->BatchUpsert(&context2, batch_req, &empty_resp);
    ASSERT_TRUE(status.ok());

    grpc::ClientContext context3;
    glyph::SearchRequest search_req;
    search_req.set_namespace_("ns3");
    search_req.add_query(12.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.set_k(1);
    search_req.set_ef(10);
    glyph::SearchResponse search_resp;

    status = stub->Search(&context3, search_req, &search_resp);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(search_resp.hits_size(), 1);
    EXPECT_EQ(search_resp.hits(0).id(), 12);
}

TEST_F(GrpcServerTest, FilteredSearchViaGrpc) {
    grpc::ClientContext context1;
    glyph::CreateNamespaceRequest create_req;
    create_req.set_name("ns4");
    auto* config = create_req.mutable_config();
    config->set_dim(4);
    config->set_m(8);
    config->set_m0(16);
    config->set_ef_construction(100);
    config->set_metric("l2");
    config->set_max_elements(1000);
    glyph::Empty empty_resp;

    grpc::Status status = stub->CreateNamespace(&context1, create_req, &empty_resp);
    ASSERT_TRUE(status.ok());

    {
        grpc::ClientContext context2;
        glyph::UpsertRequest upsert_req;
        upsert_req.set_namespace_("ns4");
        upsert_req.set_id(100);
        upsert_req.add_vector(1.0f);
        upsert_req.add_vector(0.0f);
        upsert_req.add_vector(0.0f);
        upsert_req.add_vector(0.0f);
        glyph::Value val;
        val.set_string_val("a");
        (*upsert_req.mutable_metadata())["tag"] = val;
        status = stub->Upsert(&context2, upsert_req, &empty_resp);
        ASSERT_TRUE(status.ok());
    }

    {
        grpc::ClientContext context2;
        glyph::UpsertRequest upsert_req;
        upsert_req.set_namespace_("ns4");
        upsert_req.set_id(101);
        upsert_req.add_vector(1.1f);
        upsert_req.add_vector(0.0f);
        upsert_req.add_vector(0.0f);
        upsert_req.add_vector(0.0f);
        glyph::Value val;
        val.set_string_val("b");
        (*upsert_req.mutable_metadata())["tag"] = val;
        status = stub->Upsert(&context2, upsert_req, &empty_resp);
        ASSERT_TRUE(status.ok());
    }

    grpc::ClientContext context3;
    glyph::SearchRequest search_req;
    search_req.set_namespace_("ns4");
    search_req.add_query(1.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.add_query(0.0f);
    search_req.set_k(1);
    search_req.set_ef(10);

    auto* filter = search_req.mutable_filter();
    auto* eq = filter->mutable_eq();
    eq->set_key("tag");
    eq->mutable_value()->set_string_val("b");

    glyph::SearchResponse search_resp;
    status = stub->Search(&context3, search_req, &search_resp);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(search_resp.hits_size(), 1);
    EXPECT_EQ(search_resp.hits(0).id(), 101);
}

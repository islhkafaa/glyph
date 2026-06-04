#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <random>
#include <string>
#define HTTPLIB_HEADER_ONLY
#include <httplib.h>

#include <nlohmann/json.hpp>

#include "persistence/engine.hpp"
#include "server/handler.hpp"
#include "server/http_service.hpp"

using json = nlohmann::json;

class HttpServerTest : public ::testing::Test {
   protected:
    std::filesystem::path temp_dir;
    std::unique_ptr<StorageEngine> engine;
    std::unique_ptr<CommandHandler> handler;
    std::unique_ptr<HttpService> service;
    std::unique_ptr<httplib::Client> client;

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dist(100000000, 999999999);
        temp_dir = std::filesystem::temp_directory_path() /
                   ("glyph_http_test_" + std::to_string(dist(gen)));
        std::filesystem::create_directories(temp_dir);

        engine = std::make_unique<StorageEngine>(temp_dir);
        handler = std::make_unique<CommandHandler>(*engine);
        service = std::make_unique<HttpService>(*handler, "127.0.0.1", 8081);
        service->start();

        client = std::make_unique<httplib::Client>("127.0.0.1", 8081);
    }

    void TearDown() override {
        client.reset();
        if (service) {
            service->stop();
        }
        service.reset();
        handler.reset();
        engine.reset();
        std::filesystem::remove_all(temp_dir);
    }
};

TEST_F(HttpServerTest, CreateAndSearchRoundTrip) {
    json create_body = {{"name", "ns1"},
                        {"config",
                         {{"dim", 4},
                          {"m", 8},
                          {"m0", 16},
                          {"ef_construction", 100},
                          {"metric", "l2"},
                          {"max_elements", 1000}}}};
    auto res = client->Post("/namespaces", create_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json upsert_body = {
        {"id", 1}, {"vector", {1.0f, 0.0f, 0.0f, 0.0f}}, {"metadata", {{"info", "test_http"}}}};
    res = client->Post("/namespaces/ns1/vectors", upsert_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json search_body = {{"query", {1.0f, 0.0f, 0.0f, 0.0f}}, {"k", 1}, {"ef", 10}};
    res = client->Post("/namespaces/ns1/search", search_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    auto resp_json = json::parse(res->body);
    ASSERT_TRUE(resp_json.is_array());
    ASSERT_EQ(resp_json.size(), 1);
    EXPECT_EQ(resp_json[0]["id"].get<uint64_t>(), 1);
    EXPECT_LT(resp_json[0]["distance"].get<float>(), 1e-5);
    EXPECT_EQ(resp_json[0]["metadata"]["info"].get<std::string>(), "test_http");
}

TEST_F(HttpServerTest, DeleteViaHttp) {
    json create_body = {{"name", "ns2"},
                        {"config",
                         {{"dim", 4},
                          {"m", 8},
                          {"m0", 16},
                          {"ef_construction", 100},
                          {"metric", "l2"},
                          {"max_elements", 1000}}}};
    auto res = client->Post("/namespaces", create_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json upsert_body = {{"id", 2}, {"vector", {0.0f, 1.0f, 0.0f, 0.0f}}};
    res = client->Post("/namespaces/ns2/vectors", upsert_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    res = client->Delete("/namespaces/ns2/vectors/2");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json search_body = {{"query", {0.0f, 1.0f, 0.0f, 0.0f}}, {"k", 1}, {"ef", 10}};
    res = client->Post("/namespaces/ns2/search", search_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    auto resp_json = json::parse(res->body);
    ASSERT_TRUE(resp_json.is_array());
    EXPECT_EQ(resp_json.size(), 0);
}

TEST_F(HttpServerTest, MissingNamespaceReturns404) {
    json search_body = {{"query", {1.0f, 0.0f, 0.0f, 0.0f}}, {"k", 1}, {"ef", 10}};
    auto res =
        client->Post("/namespaces/does_not_exist/search", search_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 404);
}

TEST_F(HttpServerTest, BatchUpsertAndSearch) {
    json create_body = {{"name", "ns3"},
                        {"config",
                         {{"dim", 4},
                          {"m", 8},
                          {"m0", 16},
                          {"ef_construction", 100},
                          {"metric", "l2"},
                          {"max_elements", 1000}}}};
    auto res = client->Post("/namespaces", create_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json batch_body = {{"entries", json::array()}};
    for (uint64_t i = 10; i < 15; ++i) {
        batch_body["entries"].push_back(
            {{"id", i}, {"vector", {static_cast<float>(i), 0.0f, 0.0f, 0.0f}}});
    }

    res = client->Post("/namespaces/ns3/vectors/batch", batch_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json search_body = {{"query", {12.0f, 0.0f, 0.0f, 0.0f}}, {"k", 1}, {"ef", 10}};
    res = client->Post("/namespaces/ns3/search", search_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    auto resp_json = json::parse(res->body);
    ASSERT_TRUE(resp_json.is_array());
    ASSERT_EQ(resp_json.size(), 1);
    EXPECT_EQ(resp_json[0]["id"].get<uint64_t>(), 12);
}

TEST_F(HttpServerTest, FilteredSearchViaHttp) {
    json create_body = {{"name", "ns4"},
                        {"config",
                         {{"dim", 4},
                          {"m", 8},
                          {"m0", 16},
                          {"ef_construction", 100},
                          {"metric", "l2"},
                          {"max_elements", 1000}}}};
    auto res = client->Post("/namespaces", create_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json upsert_body1 = {
        {"id", 100}, {"vector", {1.0f, 0.0f, 0.0f, 0.0f}}, {"metadata", {{"tag", "a"}}}};
    res = client->Post("/namespaces/ns4/vectors", upsert_body1.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json upsert_body2 = {
        {"id", 101}, {"vector", {1.1f, 0.0f, 0.0f, 0.0f}}, {"metadata", {{"tag", "b"}}}};
    res = client->Post("/namespaces/ns4/vectors", upsert_body2.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    json search_body = {{"query", {1.0f, 0.0f, 0.0f, 0.0f}},
                        {"k", 1},
                        {"ef", 10},
                        {"filter", {{"type", "eq"}, {"key", "tag"}, {"value", "b"}}}};
    res = client->Post("/namespaces/ns4/search", search_body.dump(), "application/json");
    ASSERT_NE(res, nullptr);
    ASSERT_EQ(res->status, 200);

    auto resp_json = json::parse(res->body);
    ASSERT_TRUE(resp_json.is_array());
    ASSERT_EQ(resp_json.size(), 1);
    EXPECT_EQ(resp_json[0]["id"].get<uint64_t>(), 101);
}

#include "server/handler.hpp"

#include "server/metrics.hpp"

CommandHandler::CommandHandler(StorageEngine& engine) : engine_(engine) {}

void CommandHandler::create_namespace(std::string_view name, HnswConfig config) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    engine_.create_namespace(name, config);
}

void CommandHandler::drop_namespace(std::string_view name) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    engine_.drop_namespace(name);
}

void CommandHandler::upsert(std::string_view ns, VectorId id, std::span<const float> vec,
                            Metadata meta) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    engine_.upsert(ns, id, vec, std::move(meta));
    Metrics::instance().upserts.fetch_add(1, std::memory_order_relaxed);
}

void CommandHandler::batch_upsert(std::string_view ns, std::vector<UpsertEntry> entries) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    for (auto& entry : entries) {
        engine_.upsert(ns, entry.id, entry.vec, std::move(entry.meta));
    }
    Metrics::instance().upserts.fetch_add(entries.size(), std::memory_order_relaxed);
}

void CommandHandler::remove(std::string_view ns, VectorId id) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    engine_.remove(ns, id);
    Metrics::instance().deletes.fetch_add(1, std::memory_order_relaxed);
}

void CommandHandler::train(std::string_view ns) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    engine_.registry().get(ns).train();
}

void CommandHandler::compact(std::string_view ns) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    engine_.registry().get(ns).compact();
    engine_.checkpoint();
}

std::vector<Collection::SearchResult> CommandHandler::search(std::string_view ns,
                                                             std::span<const float> query,
                                                             std::uint32_t k, std::uint32_t ef,
                                                             const Filter& filter) {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    auto results = engine_.registry().get(ns).search(query, k, ef, filter);
    Metrics::instance().searches.fetch_add(1, std::memory_order_relaxed);
    return results;
}

std::vector<std::string> CommandHandler::list_namespaces() const {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    return engine_.registry().list();
}

CommandHandler::NamespaceInfo CommandHandler::get_namespace(std::string_view name) const {
    Metrics::instance().total_requests.fetch_add(1, std::memory_order_relaxed);
    const auto& col = engine_.registry().get(name);
    return NamespaceInfo{col.config(), col.size()};
}

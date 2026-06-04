#include "server/handler.hpp"

CommandHandler::CommandHandler(StorageEngine& engine) : engine_(engine) {}

void CommandHandler::create_namespace(std::string_view name, HnswConfig config) {
    engine_.create_namespace(name, config);
}

void CommandHandler::drop_namespace(std::string_view name) { engine_.drop_namespace(name); }

void CommandHandler::upsert(std::string_view ns, VectorId id, std::span<const float> vec,
                            Metadata meta) {
    engine_.upsert(ns, id, vec, std::move(meta));
}

void CommandHandler::batch_upsert(std::string_view ns, std::vector<UpsertEntry> entries) {
    for (auto& entry : entries) {
        engine_.upsert(ns, entry.id, entry.vec, std::move(entry.meta));
    }
}

void CommandHandler::remove(std::string_view ns, VectorId id) { engine_.remove(ns, id); }

std::vector<Collection::SearchResult> CommandHandler::search(std::string_view ns,
                                                             std::span<const float> query,
                                                             std::uint32_t k, std::uint32_t ef,
                                                             const Filter& filter) {
    return engine_.registry().get(ns).search(query, k, ef, filter);
}

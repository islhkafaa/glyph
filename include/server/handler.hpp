#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "collection/collection.hpp"
#include "hnsw/config.hpp"
#include "metadata/filter.hpp"
#include "metadata/value.hpp"
#include "persistence/engine.hpp"

class CommandHandler {
   public:
    explicit CommandHandler(StorageEngine& engine);

    void create_namespace(std::string_view name, HnswConfig config);
    void drop_namespace(std::string_view name);
    void upsert(std::string_view ns, VectorId id, std::span<const float> vec, Metadata meta);
    void batch_upsert(std::string_view ns, std::vector<UpsertEntry> entries);
    void remove(std::string_view ns, VectorId id);
    std::vector<Collection::SearchResult> search(std::string_view ns, std::span<const float> query,
                                                 std::uint32_t k, std::uint32_t ef,
                                                 const Filter& filter);

   private:
    StorageEngine& engine_;
};

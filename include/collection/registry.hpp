#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "collection/collection.hpp"

class NamespaceRegistry {
   public:
    void create(std::string_view name, HnswConfig config);
    void register_collection(std::string_view name, Collection col);
    void drop(std::string_view name);

    Collection& get(std::string_view name);
    const Collection& get(std::string_view name) const;

    bool exists(std::string_view name) const;
    std::vector<std::string> list() const;

    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);

   private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<Collection>> namespaces_;
};

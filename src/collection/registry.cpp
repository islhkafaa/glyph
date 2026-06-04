#include "collection/registry.hpp"

#include <mutex>
#include <stdexcept>

#include "wal/record.hpp"

void NamespaceRegistry::create(std::string_view name, HnswConfig config) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    std::string name_str(name);
    if (namespaces_.find(name_str) != namespaces_.end()) {
        return;
    }
    namespaces_[name_str] = std::make_unique<Collection>(config);
}

void NamespaceRegistry::drop(std::string_view name) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    namespaces_.erase(std::string(name));
}

Collection& NamespaceRegistry::get(std::string_view name) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = namespaces_.find(std::string(name));
    if (it == namespaces_.end()) {
        throw std::out_of_range("Namespace not found");
    }
    return *(it->second);
}

const Collection& NamespaceRegistry::get(std::string_view name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = namespaces_.find(std::string(name));
    if (it == namespaces_.end()) {
        throw std::out_of_range("Namespace not found");
    }
    return *(it->second);
}

bool NamespaceRegistry::exists(std::string_view name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return namespaces_.find(std::string(name)) != namespaces_.end();
}

std::vector<std::string> NamespaceRegistry::list() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(namespaces_.size());
    for (const auto& pair : namespaces_) {
        names.push_back(pair.first);
    }
    return names;
}

void NamespaceRegistry::serialize(std::ostream& out) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    wal::write_uint32(out, static_cast<std::uint32_t>(namespaces_.size()));
    for (const auto& [name, col_ptr] : namespaces_) {
        wal::write_string(out, name);
        col_ptr->serialize(out);
    }
}

void NamespaceRegistry::deserialize(std::istream& in) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    namespaces_.clear();
    std::uint32_t count = wal::read_uint32(in);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::string name = wal::read_string(in);
        Collection col = Collection::deserialize(in);
        namespaces_[name] = std::make_unique<Collection>(std::move(col));
    }
}

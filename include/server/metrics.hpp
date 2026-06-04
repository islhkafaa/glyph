#pragma once

#include <atomic>
#include <cstdint>

struct MetricsSnapshot {
    std::uint64_t upserts;
    std::uint64_t searches;
    std::uint64_t deletes;
    std::uint64_t errors;
    std::uint64_t total_requests;
};

class Metrics {
   public:
    static Metrics& instance() {
        static Metrics inst;
        return inst;
    }

    std::atomic<std::uint64_t> upserts{0};
    std::atomic<std::uint64_t> searches{0};
    std::atomic<std::uint64_t> deletes{0};
    std::atomic<std::uint64_t> errors{0};
    std::atomic<std::uint64_t> total_requests{0};

    MetricsSnapshot snapshot() const {
        return MetricsSnapshot{
            upserts.load(std::memory_order_relaxed), searches.load(std::memory_order_relaxed),
            deletes.load(std::memory_order_relaxed), errors.load(std::memory_order_relaxed),
            total_requests.load(std::memory_order_relaxed)};
    }

   private:
    Metrics() = default;
    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;
};

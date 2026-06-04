#include "metadata/filter.hpp"

#include <type_traits>

bool matches(const Metadata& doc, const Filter& filter) {
    return std::visit(
        [&doc](const auto& f) -> bool {
            using T = std::decay_t<decltype(f)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return true;
            } else if constexpr (std::is_same_v<T, EqFilter>) {
                auto it = doc.find(f.key);
                if (it == doc.end()) {
                    return false;
                }
                return it->second == f.value;
            } else if constexpr (std::is_same_v<T, RangeFilter>) {
                auto it = doc.find(f.key);
                if (it == doc.end()) {
                    return false;
                }
                if (std::holds_alternative<double>(it->second)) {
                    double val = std::get<double>(it->second);
                    return val >= f.lo && val <= f.hi;
                }
                return false;
            }
            return false;
        },
        filter);
}

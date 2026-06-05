#include "metadata/filter.hpp"

#include <algorithm>
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
            } else if constexpr (std::is_same_v<T, AndFilter>) {
                for (const auto& cond : f.conditions) {
                    if (!matches(doc, cond)) {
                        return false;
                    }
                }
                return true;
            } else if constexpr (std::is_same_v<T, OrFilter>) {
                for (const auto& cond : f.conditions) {
                    if (matches(doc, cond)) {
                        return true;
                    }
                }
                return false;
            } else if constexpr (std::is_same_v<T, NotFilter>) {
                if (!f.condition) {
                    return false;
                }
                return !matches(doc, *f.condition);
            } else if constexpr (std::is_same_v<T, CompareFilter>) {
                auto it = doc.find(f.key);
                if (it == doc.end()) {
                    return false;
                }
                const auto& doc_val = it->second;
                const auto& filter_val = f.value;
                if (doc_val.index() != filter_val.index()) {
                    return false;
                }
                return std::visit(
                    [&f](const auto& val1, const auto& val2) -> bool {
                        using T1 = std::decay_t<decltype(val1)>;
                        using T2 = std::decay_t<decltype(val2)>;
                        if constexpr (std::is_same_v<T1, T2>) {
                            switch (f.op) {
                                case CompareOp::Lt:
                                    return val1 < val2;
                                case CompareOp::Gt:
                                    return val1 > val2;
                                case CompareOp::Lte:
                                    return val1 <= val2;
                                case CompareOp::Gte:
                                    return val1 >= val2;
                            }
                        }
                        return false;
                    },
                    doc_val, filter_val);
            } else if constexpr (std::is_same_v<T, InFilter>) {
                auto it = doc.find(f.key);
                if (it == doc.end()) {
                    return false;
                }
                bool found =
                    std::find(f.values.begin(), f.values.end(), it->second) != f.values.end();
                return f.is_not ? !found : found;
            }
            return false;
        },
        static_cast<const FilterBase&>(filter));
}

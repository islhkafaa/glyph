#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "metadata/value.hpp"

struct Filter;

struct EqFilter {
    std::string key;
    MetadataValue value;
};

struct RangeFilter {
    std::string key;
    double lo;
    double hi;
};

struct AndFilter {
    std::vector<Filter> conditions;
};

struct OrFilter {
    std::vector<Filter> conditions;
};

struct NotFilter {
    std::shared_ptr<Filter> condition;
};

enum class CompareOp { Lt, Gt, Lte, Gte };

struct CompareFilter {
    std::string key;
    CompareOp op;
    MetadataValue value;
};

struct InFilter {
    std::string key;
    std::vector<MetadataValue> values;
    bool is_not = false;
};

using FilterBase = std::variant<std::monostate, EqFilter, RangeFilter, AndFilter, OrFilter,
                                NotFilter, CompareFilter, InFilter>;

struct Filter : FilterBase {
    using FilterBase::FilterBase;
};

bool matches(const Metadata& doc, const Filter& filter);

#pragma once

#include <string>
#include <variant>

#include "metadata/value.hpp"

struct EqFilter {
    std::string key;
    MetadataValue value;
};

struct RangeFilter {
    std::string key;
    double lo;
    double hi;
};

using Filter = std::variant<std::monostate, EqFilter, RangeFilter>;

bool matches(const Metadata& doc, const Filter& filter);

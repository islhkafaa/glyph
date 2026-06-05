#pragma once

#include <cstdint>
#include <span>

#include "distance/kernel.hpp"

using Sq8KernelFn = float (*)(std::span<const float> query, std::span<const uint8_t> code,
                              std::span<const float> min_vals, std::span<const float> scales);

struct DispatchTable {
    KernelFn cosine;
    KernelFn l2;
    KernelFn dot_product;
    KernelFn l1;
    KernelFn hamming;
    KernelFn l2_sq;
};

struct Sq8DispatchTable {
    Sq8KernelFn cosine;
    Sq8KernelFn l2;
    Sq8KernelFn dot_product;
    Sq8KernelFn l1;
    Sq8KernelFn hamming;
};

const DispatchTable& get_dispatch_table();
const Sq8DispatchTable& get_sq8_dispatch_table();
Sq8KernelFn get_sq8_kernel(Metric metric);
void build_dispatch_table();

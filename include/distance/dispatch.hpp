#pragma once

#include "distance/kernel.hpp"

struct DispatchTable {
    KernelFn cosine;
    KernelFn l2;
    KernelFn dot_product;
    KernelFn l1;
    KernelFn hamming;
};

const DispatchTable& get_dispatch_table();
void build_dispatch_table();

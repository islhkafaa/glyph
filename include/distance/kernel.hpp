#pragma once

#include <concepts>
#include <span>

#include "distance/metric.hpp"

using KernelFn = float (*)(std::span<const float>, std::span<const float>);

template <typename F>
concept DistanceKernel = requires(F f, std::span<const float> a, std::span<const float> b) {
    { f(a, b) } -> std::same_as<float>;
};

KernelFn get_kernel(Metric metric);

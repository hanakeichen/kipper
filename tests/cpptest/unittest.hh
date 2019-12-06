#pragma once

#include <gtest/gtest.h>
#include <kipper/kipper.hh>
#include <numeric>

constexpr double kNaN = std::numeric_limits<double>().quiet_NaN();
constexpr double kDoubleMin = std::numeric_limits<double>::min();
constexpr double kDoubleMax = std::numeric_limits<double>::max();
constexpr int32_t kInt32Min = std::numeric_limits<int32_t>().min();
constexpr int32_t kInt32Max = std::numeric_limits<int32_t>().max();
constexpr int64_t kInt64Min = std::numeric_limits<int64_t>().min();
constexpr int64_t kInt64Max = std::numeric_limits<int64_t>().max();
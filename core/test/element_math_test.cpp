// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/math.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementAbsTest, unit) {
  units::Unit m(units::m);
  EXPECT_EQ(element::abs(m), units::abs(m));
}

TEST(ElementAbsTest, value) {
  EXPECT_EQ(element::abs(-1.23), std::abs(-1.23));
  EXPECT_EQ(element::abs(-1.23456789f), std::abs(-1.23456789f));
}

TEST(ElementAbsTest, value_and_variance) {
  const ValueAndVariance x(-2.0, 1.0);
  EXPECT_EQ(element::abs(x), abs(x));
}

TEST(ElementAbsTest, supported_types) {
  auto supported = decltype(element::abs)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementNormTest, unit) {
  const units::Unit s(units::s);
  const units::Unit m2(units::m * units::m);
  const units::Unit dimless(units::dimensionless);
  EXPECT_EQ(element::norm(m2), m2);
  EXPECT_EQ(element::norm(s), s);
  EXPECT_EQ(element::norm(dimless), dimless);
}

TEST(ElementNormTest, value) {
  Eigen::Vector3d v1(0, 3, 4);
  Eigen::Vector3d v2(3, 0, -4);
  EXPECT_EQ(element::norm(v1), 5);
  EXPECT_EQ(element::norm(v2), 5);
}

TEST(ElementSqrtTest, unit) {
  const units::Unit m2(units::m * units::m);
  EXPECT_EQ(element::sqrt(m2), units::sqrt(m2));
}

TEST(ElementSqrtTest, value) {
  EXPECT_EQ(element::sqrt(1.23), std::sqrt(1.23));
  EXPECT_EQ(element::sqrt(1.23456789f), std::sqrt(1.23456789f));
}

TEST(ElementSqrtTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::sqrt(x), sqrt(x));
}

TEST(ElementSqrtTest, supported_types) {
  auto supported = decltype(element::sqrt)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementDotTest, unit) {
  const units::Unit m(units::m);
  const units::Unit m2(units::m * units::m);
  const units::Unit dimless(units::dimensionless);
  EXPECT_EQ(element::dot(m, m), m2);
  EXPECT_EQ(element::dot(dimless, dimless), dimless);
}

TEST(ElementDotTest, value) {
  Eigen::Vector3d v1(0, 3, -4);
  Eigen::Vector3d v2(1, 1, -1);
  EXPECT_EQ(element::dot(v1, v1), 25);
  EXPECT_EQ(element::dot(v2, v2), 3);
}

TEST(ElementReciprocalTest, unit) {
  const units::Unit one_over_m(units::one / units::m);
  EXPECT_EQ(element::reciprocal(one_over_m), units::m);
  const units::Unit one_over_s(units::one / units::s);
  EXPECT_EQ(element::reciprocal(units::s), one_over_s);
}

TEST(ElementReciprocalTest, value) {
  EXPECT_EQ(element::reciprocal(1.23), 1 / 1.23);
  EXPECT_EQ(element::reciprocal(1.23456789f), 1 / 1.23456789f);
}

TEST(ElementReciprocalTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::reciprocal(x), 1 / x);
}

TEST(ElementExpTest, value) {
  EXPECT_EQ(element::exp(1.23), std::exp(1.23));
  EXPECT_EQ(element::exp(1.23456789f), std::exp(1.23456789f));
}

TEST(ElementExpTest, unit) {
  EXPECT_EQ(element::exp(units::dimensionless), units::dimensionless);
}

TEST(ElementExpTest, bad_unit) { EXPECT_ANY_THROW(element::exp(units::m)); }

TEST(ElementLogTest, value) {
  EXPECT_EQ(element::log(1.23), std::log(1.23));
  EXPECT_EQ(element::log(1.23456789f), std::log(1.23456789f));
}

TEST(ElementLogTest, unit) {
  EXPECT_EQ(element::log(units::dimensionless), units::dimensionless);
}

TEST(ElementLogTest, bad_unit) { EXPECT_ANY_THROW(element::log(units::m)); }

TEST(ElementLog10Test, value) {
  EXPECT_EQ(element::log10(1.23), std::log10(1.23));
  EXPECT_EQ(element::log10(1.23456789f), std::log10(1.23456789f));
}

TEST(ElementLog10Test, unit) {
  EXPECT_EQ(element::log10(units::dimensionless), units::dimensionless);
}

TEST(ElementLog10Test, bad_unit) { EXPECT_ANY_THROW(element::log10(units::m)); }

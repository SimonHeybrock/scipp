// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include "scipp/core/eigen.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "test_macros.h"
#include <gtest/gtest.h>

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::units;

template <typename T> class IsCloseTest : public ::testing::Test {};
using TestTypes = ::testing::Types<double, float, int64_t, int32_t>;

TYPED_TEST_SUITE(IsCloseTest, TestTypes);

TYPED_TEST(IsCloseTest, atol_when_variable_equal) {
  const auto a = makeVariable<TypeParam>(Values{1});
  const auto rtol = makeVariable<TypeParam>(Values{0});
  const auto atol = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(isclose(a, a, rtol, atol), true * units::one);
}

TYPED_TEST(IsCloseTest, atol_when_variables_within_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{0});
  const auto b = makeVariable<TypeParam>(Values{1});
  const auto rtol = makeVariable<TypeParam>(Values{0});
  const auto atol = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(isclose(a, b, rtol, atol), true * units::one);
}

TYPED_TEST(IsCloseTest, atol_when_variables_outside_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{0});
  const auto b = makeVariable<TypeParam>(Values{2});
  const auto rtol = makeVariable<TypeParam>(Values{0});
  const auto atol = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(isclose(a, b, rtol, atol), false * units::one);
}

TYPED_TEST(IsCloseTest, rtol_when_variables_within_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{8});
  const auto b = makeVariable<TypeParam>(Values{9});
  // tol = atol + rtol * b = 1
  const auto rtol = makeVariable<double>(Values{1.0 / 9});
  const auto atol = makeVariable<TypeParam>(Values{0});
  EXPECT_EQ(isclose(a, b, rtol, atol), true * units::one);
}

TYPED_TEST(IsCloseTest, rtol_when_variables_outside_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{7});
  const auto b = makeVariable<TypeParam>(Values{9});
  // tol = atol + rtol * b = 1
  const auto rtol = makeVariable<double>(Values{1.0 / 9});
  const auto atol = makeVariable<TypeParam>(Values{0});
  EXPECT_EQ(isclose(a, b, rtol, atol), false * units::one);
}

TEST(IsCloseTest, with_vectors) {
  const auto u =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{0, 0, 0}});
  const auto v =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{1, 1, 1}});
  const auto w =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{1, 1, 1.0001}});
  const auto rtol = 0.0 * units::one;
  const auto atol = 1.0 * units::one;
  EXPECT_EQ(isclose(u, u, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, v, rtol, atol),

            makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(v, w, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, w, rtol, atol), makeVariable<bool>(Values{false}));
}
TEST(IsCloseTest, works_for_counts) {
  const auto a = makeVariable<double>(Values{1}, Variances{1}, units::counts);
  const auto rtol = 1e-5 * units::one;
  const auto atol = 0.0 * units::counts;
  EXPECT_NO_THROW_DISCARD(isclose(a, a, rtol, atol));
}

TEST(IsCloseTest, compare_variances_only) {
  // Tests setup so that value comparison does not affect output (a, b value
  // same)
  const auto a = makeVariable<double>(Values{10.0}, Variances{0.0});
  const auto b = makeVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_EQ(isclose(a, b, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{1.0})),
            true * units::one);
  EXPECT_EQ(isclose(a, b, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * units::one);
}

TEST(IsCloseTest, compare_values_and_variances) {
  // Tests setup so that value comparison does not affect output (a, b value
  // same)
  const auto w = makeVariable<double>(Values{10.0}, Variances{0.0});
  const auto x = makeVariable<double>(Values{9.0}, Variances{0.0});
  const auto y = makeVariable<double>(Values{10.0}, Variances{1.0});
  const auto z = makeVariable<double>(Values{9.0}, Variances{1.0});
  // sanity check no mismatch
  EXPECT_EQ(isclose(w, w, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            true * units::one);
  // mismatch value only
  EXPECT_EQ(isclose(w, x, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * units::one);
  // mismatch variance only
  EXPECT_EQ(isclose(w, y, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * units::one);
  // mismatch value and variance
  EXPECT_EQ(isclose(w, z, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * units::one);

  // same as above but looser tolerance
  EXPECT_EQ(isclose(w, z, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{1.0})),
            true * units::one);
}

TEST(IsCloseTest, rtol_units) {
  auto unit = scipp::units::m;
  const auto a = makeVariable<double>(Values{1.0}, Variances{1.0}, unit);
  // This is fine
  EXPECT_EQ(isclose(a, a, 1.0 * scipp::units::one, 1.0 * unit),
            true * scipp::units::one);
  // Now rtol has units m
  EXPECT_THROW_DISCARD(isclose(a, a, 1.0 * unit, 1.0 * unit),
                       except::UnitError);
}

TEST(ComparisonTest, variances_test) {
  const auto a = makeVariable<float>(Values{1.0}, Variances{1.0});
  const auto b = makeVariable<float>(Values{2.0}, Variances{2.0});
  EXPECT_EQ(less(a, b), true * units::one);
  EXPECT_EQ(less_equal(a, b), true * units::one);
  EXPECT_EQ(greater(a, b), false * units::one);
  EXPECT_EQ(greater_equal(a, b), false * units::one);
  EXPECT_EQ(equal(a, b), false * units::one);
  EXPECT_EQ(not_equal(a, b), true * units::one);
}

TEST(ComparisonTest, less_units_test) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.0, 3.0});
  b.setUnit(units::m);
  EXPECT_THROW([[maybe_unused]] auto out = less(a, b), std::runtime_error);
}

namespace {
const auto a = 1.0 * units::m;
const auto b = 2.0 * units::m;
const auto true_ = true * units::one;
const auto false_ = false * units::one;
TEST(ComparisonTest, less_test) {
  EXPECT_EQ(less(a, b), true_);
  EXPECT_EQ(less(b, a), false_);
  EXPECT_EQ(less(a, a), false_);
}
TEST(ComparisonTest, greater_test) {
  EXPECT_EQ(greater(a, b), false_);
  EXPECT_EQ(greater(b, a), true_);
  EXPECT_EQ(greater(a, a), false_);
}
TEST(ComparisonTest, greater_equal_test) {
  EXPECT_EQ(greater_equal(a, b), false_);
  EXPECT_EQ(greater_equal(b, a), true_);
  EXPECT_EQ(greater_equal(a, a), true_);
}
TEST(ComparisonTest, less_equal_test) {
  EXPECT_EQ(less_equal(a, b), true_);
  EXPECT_EQ(less_equal(b, a), false_);
  EXPECT_EQ(less_equal(a, a), true_);
}
TEST(ComparisonTest, equal_test) {
  EXPECT_EQ(equal(a, b), false_);
  EXPECT_EQ(equal(b, a), false_);
  EXPECT_EQ(equal(a, a), true_);
}
TEST(ComparisonTest, not_equal_test) {
  EXPECT_EQ(not_equal(a, b), true_);
  EXPECT_EQ(not_equal(b, a), true_);
  EXPECT_EQ(not_equal(a, a), false_);
}
} // namespace

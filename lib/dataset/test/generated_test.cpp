// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/less.h"
#include "scipp/dataset/reciprocal.h"
#include "scipp/variable/less.h"
#include "scipp/variable/reciprocal.h"

#include "test_data_arrays.h"

using namespace scipp;

namespace {
void check_meta(const DataArray &out, const DataArray &a) {
  EXPECT_FALSE(out.data().is_same(a.data()));
  EXPECT_EQ(out.coords(), a.coords());
  EXPECT_EQ(out.masks(), a.masks());
  EXPECT_EQ(out.attrs(), a.attrs());
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &a.coords());
  EXPECT_NE(&out.masks(), &a.masks());
  EXPECT_NE(&out.attrs(), &a.attrs());
  EXPECT_TRUE(out.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  // Masks are NOT shallow-copied, just like data
  EXPECT_FALSE(out.masks()["mask"].is_same(a.masks()["mask"]));
  EXPECT_TRUE(out.attrs()[Dim("attr")].is_same(a.attrs()[Dim("attr")]));
}
} // namespace

TEST(GeneratedUnaryTest, DataArray) {
  const auto array = make_data_array_1d();
  const auto out = reciprocal(array);
  EXPECT_EQ(out.data(), reciprocal(array.data()));
  check_meta(out, array);
}

class GeneratedBinaryTest : public ::testing::Test {
protected:
  DataArray a = make_data_array_1d(1);
  DataArray b = make_data_array_1d(2);
};

TEST_F(GeneratedBinaryTest, DataArray_Variable) {
  const auto var = b.data();
  // Using `less` as an example of a generated binary function
  const auto out = less(a, var);
  EXPECT_EQ(out.data(), less(a.data(), var));
  EXPECT_FALSE(out.data().is_same(var));
  check_meta(out, a);
}

TEST_F(GeneratedBinaryTest, Variable_DataArray) {
  const auto var = b.data();
  // Using `less` as an example of a generated binary function
  const auto out = less(var, a);
  EXPECT_EQ(out.data(), less(var, a.data()));
  EXPECT_FALSE(out.data().is_same(var));
  check_meta(out, a);
}

class GeneratedBinaryDataArrayTest : public ::testing::Test {
protected:
  DataArray a = make_data_array_1d(1);
  DataArray b = make_data_array_1d(2);
  // Using `less` as an example of a generated binary function
  DataArray out = less(a, b);
};

TEST_F(GeneratedBinaryDataArrayTest, DataArray_DataArray) {
  EXPECT_FALSE(out.data().is_same(a.data()));
  EXPECT_FALSE(out.data().is_same(b.data()));
  EXPECT_EQ(out.data(), less(a.data(), b.data()));
  EXPECT_EQ(out.coords(), a.coords()); // because both inputs have same coords
  EXPECT_NE(out.masks(), a.masks());
  EXPECT_NE(out.attrs(), a.attrs());
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &a.coords());
  EXPECT_NE(&out.masks(), &a.masks());
  EXPECT_NE(&out.attrs(), &a.attrs());
}

TEST_F(GeneratedBinaryDataArrayTest, coord_union) {
  b.coords().set(Dim("aux"), copy(b.coords()[Dim::X]));
  out = less(a, b);
  // Coords are shared
  EXPECT_TRUE(out.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  EXPECT_TRUE(out.coords()[Dim("aux")].is_same(b.coords()[Dim("aux")]));
}

TEST_F(GeneratedBinaryDataArrayTest, mask_or) {
  // Masks are NOT shared
  EXPECT_FALSE(out.masks()["mask"].is_same(a.masks()["mask"]));
  EXPECT_FALSE(out.masks()["mask"].is_same(b.masks()["mask"]));
  EXPECT_EQ(out.masks()["mask"], a.masks()["mask"] | b.masks()["mask"]);
  // masks only in one input are deep-copied
  EXPECT_FALSE(out.masks()["mask1"].is_same(a.masks()["mask1"]));
  EXPECT_FALSE(out.masks()["mask2"].is_same(b.masks()["mask2"]));
  EXPECT_EQ(out.masks()["mask1"], a.masks()["mask1"]);
  EXPECT_EQ(out.masks()["mask2"], b.masks()["mask2"]);
}

TEST_F(GeneratedBinaryDataArrayTest, mask_is_deep_copied_even_if_same) {
  EXPECT_FALSE(less(a, a).masks()["mask"].is_same(a.masks()["mask"]));
}

TEST_F(GeneratedBinaryDataArrayTest, attr_intersection) {
  EXPECT_TRUE(a.attrs().contains(Dim("attr1")));
  EXPECT_TRUE(b.attrs().contains(Dim("attr2")));
  // Attrs are shared
  EXPECT_TRUE(out.attrs()[Dim("attr")].is_same(a.attrs()[Dim("attr")]));
  EXPECT_FALSE(out.attrs().contains(Dim("attr1")));
  EXPECT_FALSE(out.attrs().contains(Dim("attr2")));
}

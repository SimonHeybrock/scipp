// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "dimensions.h"

using namespace scipp::core;

TEST(Dimensions, footprint) {
  EXPECT_EQ(sizeof(Dimensions), 64ul);
  // TODO Do we want to align this? Need to run benchmarks when implementation
  // is more mature.
  // EXPECT_EQ(std::alignment_of<Dimensions>(), 64);
}

TEST(Dimensions, construct) {
  EXPECT_NO_THROW(Dimensions());
  EXPECT_NO_THROW(Dimensions{});
  EXPECT_NO_THROW((Dimensions{Dim::X, 1}));
  EXPECT_NO_THROW((Dimensions({Dim::X, 1})));
  EXPECT_NO_THROW((Dimensions({{Dim::X, 1}, {Dim::Y, 1}})));
}

TEST(Dimensions, operator_equals) {
  EXPECT_EQ((Dimensions({Dim::X, 1})), (Dimensions({Dim::X, 1})));
}

TEST(Dimensions, count_and_volume) {
  Dimensions dims;
  EXPECT_EQ(dims.count(), 0);
  EXPECT_EQ(dims.volume(), 1);
  dims.add(Dim::Tof, 3);
  EXPECT_EQ(dims.count(), 1);
  EXPECT_EQ(dims.volume(), 3);
  dims.add(Dim::Q, 2);
  EXPECT_EQ(dims.count(), 2);
  EXPECT_EQ(dims.volume(), 6);
}

TEST(Dimensions, offset_from_list_init) {
  // Leftmost is outer dimension, rightmost is inner dimension.
  Dimensions dims{{Dim::Q, 2}, {Dim::Tof, 3}};
  EXPECT_EQ(dims.offset(Dim::Tof), 1);
  EXPECT_EQ(dims.offset(Dim::Q), 3);
}

TEST(Dimensions, offset) {
  Dimensions dims;
  dims.add(Dim::Tof, 3);
  dims.add(Dim::Q, 2);
  EXPECT_EQ(dims.offset(Dim::Tof), 1);
  EXPECT_EQ(dims.offset(Dim::Q), 3);
}

TEST(Dimensions, erase) {
  Dimensions dims;
  dims.add(Dim::X, 2);
  dims.add(Dim::Y, 3);
  dims.add(Dim::Z, 4);
  dims.erase(Dim::Y);
  EXPECT_TRUE(dims.contains(Dim::X));
  EXPECT_FALSE(dims.contains(Dim::Y));
  EXPECT_TRUE(dims.contains(Dim::Z));
  EXPECT_EQ(dims.volume(), 8);
  EXPECT_EQ(dims.index(Dim::Z), 0);
  EXPECT_EQ(dims.index(Dim::X), 1);
}

TEST(Dimensions, erase_inner) {
  Dimensions dims;
  dims.add(Dim::X, 2);
  dims.add(Dim::Y, 3);
  dims.add(Dim::Z, 4);
  dims.erase(Dim::X);
  EXPECT_FALSE(dims.contains(Dim::X));
  EXPECT_TRUE(dims.contains(Dim::Y));
  EXPECT_TRUE(dims.contains(Dim::Z));
  EXPECT_EQ(dims.volume(), 12);
}

TEST(Dimensions, contains_other) {
  Dimensions a;
  a.add(Dim::Tof, 3);
  a.add(Dim::Q, 2);

  EXPECT_TRUE(a.contains(Dimensions{}));
  EXPECT_TRUE(a.contains(a));
  EXPECT_TRUE(a.contains(Dimensions(Dim::Q, 2)));
  EXPECT_FALSE(a.contains(Dimensions(Dim::Q, 3)));

  Dimensions b;
  b.add(Dim::Q, 2);
  b.add(Dim::Tof, 3);
  // Order does not matter.
  EXPECT_TRUE(a.contains(b));
}

TEST(Dimensions, isContiguousIn) {
  Dimensions parent({{Dim::Z, 2}, {Dim::Y, 3}, {Dim::X, 4}});

  EXPECT_TRUE(parent.isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({Dim::X, 0}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 1}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 2}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 4}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({Dim::X, 5}).isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({{Dim::Y, 0}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Y, 1}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Y, 2}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Y, 3}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Y, 4}, {Dim::X, 4}}).isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({{Dim::Z, 0}, {Dim::Y, 3}, {Dim::X, 4}})
                  .isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Z, 1}, {Dim::Y, 3}, {Dim::X, 4}})
                  .isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Z, 2}, {Dim::Y, 3}, {Dim::X, 4}})
                  .isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Z, 3}, {Dim::Y, 3}, {Dim::X, 4}})
                   .isContiguousIn(parent));

  EXPECT_FALSE(Dimensions({Dim::Y, 3}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({Dim::Z, 2}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Z, 2}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Z, 2}, {Dim::Y, 3}}).isContiguousIn(parent));
}

TEST(SparseDimensions, sparse) {
  Dimensions dims;
  EXPECT_FALSE(dims.sparse());
}

TEST(SparseDimensions, empty_add) {
  Dimensions dims;
  ASSERT_NO_THROW(dims.add(Dim::X, Extent::Sparse));
  ASSERT_THROW(dims.volume(), except::SparseDimensionError);
}

TEST(SparseDimensions, sparse_must_be_inner) {
  Dimensions dims(Dim::X, 2);
  ASSERT_THROW(dims.add(Dim::Y, Extent::Sparse), except::SparseDimensionError);
  ASSERT_NO_THROW(dims.addInner(Dim::Y, Extent::Sparse));
  EXPECT_TRUE(dims.sparse());
  ASSERT_THROW(dims.volume(), except::SparseDimensionError);
  ASSERT_THROW(dims.addInner(Dim::Z, 2), except::SparseDimensionError);
  ASSERT_NO_THROW(dims.add(Dim::Z, 2));
  EXPECT_TRUE(dims.sparse());
  ASSERT_THROW(dims.volume(), except::SparseDimensionError);
}

TEST(SparseDimensions, create_initializer_list) {
  ASSERT_NO_THROW((Dimensions{{Dim::X, 2}, {Dim::Y, Extent::Sparse}}));
  ASSERT_THROW((Dimensions{{Dim::Y, Extent::Sparse}, {Dim::X, 2}}),
               except::SparseDimensionError);
}

TEST(SparseDimensions, nonSparseArea) {
  Dimensions dense{{Dim::X, 2}, {Dim::Y, 3}};
  EXPECT_EQ(dense.nonSparseArea(), 6);
  Dimensions sparse{{Dim::X, 2}, {Dim::Y, Extent::Sparse}};
  EXPECT_EQ(sparse.nonSparseArea(), 2);
}

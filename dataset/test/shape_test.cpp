// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"
#include "test_util.h"

#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/shape.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::dataset;

TEST(ResizeTest, data_array_1d) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  DataArray a(var);
  a.coords().set(Dim::X, var);
  a.attrs().set(Dim::Y, var);
  a.masks().set("mask", var);
  DataArray expected(makeVariable<double>(Dims{Dim::X}, Shape{3}));
  EXPECT_EQ(resize(a, Dim::X, 3), expected);
}

TEST(ResizeTest, data_array_2d) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 2},
                                        Values{1, 2, 3, 4, 5, 6});
  auto x = var.slice({Dim::Y, 0});
  auto y = var.slice({Dim::X, 0});
  DataArray a(var);
  a.coords().set(Dim::X, x);
  a.coords().set(Dim::Y, y);
  a.attrs().set(Dim("unaligned-x"), x);
  a.attrs().set(Dim("unaligned-y"), y);
  a.masks().set("mask-x", x);
  a.masks().set("mask-y", y);

  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 2}));
  expected.coords().set(Dim::X, x);
  expected.attrs().set(Dim("unaligned-x"), x);
  expected.masks().set("mask-x", x);

  EXPECT_EQ(resize(a, Dim::Y, 1), expected);
  EXPECT_NE(resize(a, Dim::Y, 1).masks()["mask-x"].values<double>().data(),
            expected.masks()["mask-x"].values<double>().data());

  Dataset d({{"a", a}});
  Dataset expected_d({{"a", expected}});
  EXPECT_EQ(resize(d, Dim::Y, 1), expected_d);
  EXPECT_NE(resize(d, Dim::Y, 1)["a"].masks()["mask-x"].values<double>().data(),
            expected_d["a"].masks()["mask-x"].values<double>().data());
}

TEST(ReshapeTest, fold_x) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X, fold(arange(Dim::X, 6), Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}) +
                  0.1 * units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, fold_y) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::X, 6}, {Dim::Row, 2}, {Dim::Time, 2}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::Y, fold(arange(Dim::Y, 4), Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}) +
                  0.2 * units::one);
  expected.coords().set(Dim::X, a.coords()[Dim::X]);

  EXPECT_EQ(fold(a, Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}), expected);
}

TEST(ReshapeTest, fold_into_3_dims) {
  const auto var = arange(Dim::X, 24);
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 24) + 0.1 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Time, 2}, {Dim::Y, 3}, {Dim::Z, 4}});
  DataArray expected(rshp);
  expected.coords().set(Dim::X, rshp + 0.1 * units::one);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Time, 2}, {Dim::Y, 3}, {Dim::Z, 4}}),
            expected);
}

TEST(ReshapeTest, flatten) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  const auto rshp = arange(Dim::Z, 24);
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.1, 0.1, 0.1, 0.1, 1.1, 1.1, 1.1, 1.1,
                                  2.1, 2.1, 2.1, 2.1, 3.1, 3.1, 3.1, 3.1,
                                  4.1, 4.1, 4.1, 4.1, 5.1, 5.1, 5.1, 5.1}));
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, flatten_bad_dim_order) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  EXPECT_THROW_MSG_DISCARD(
      flatten(a, std::vector<Dim>{Dim::Y, Dim::X}, Dim::Z),
      except::DimensionError,
      "Can only flatten a contiguous set of dimensions in the correct order");
}

TEST(ReshapeTest, round_trip) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  auto reshaped = fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            a);
}

TEST(ReshapeTest, fold_x_binedges_x) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Row, Dim::Time}, Shape{2, 4},
                           Values{0.1, 1.1, 2.1, 3.1, 3.1, 4.1, 5.1, 6.1}));
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, fold_y_binedges_y) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 5) + 0.2 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::X, 6}, {Dim::Row, 2}, {Dim::Time, 2}});
  DataArray expected(rshp);
  expected.coords().set(Dim::X, a.coords()[Dim::X]);
  expected.coords().set(
      Dim::Y, makeVariable<double>(Dims{Dim::Row, Dim::Time}, Shape{2, 3},
                                   Values{0.2, 1.2, 2.2, 2.2, 3.2, 4.2}));

  EXPECT_EQ(fold(a, Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}), expected);
}

TEST(ReshapeTest, flatten_binedges_1d) {
  DataArray a(arange(Dim::X, 4), {{Dim::Z, arange(Dim::X, 5)}});
  const auto flat = flatten(a, std::vector<Dim>{Dim::X}, Dim::Y);
  auto expected = copy(a);
  expected.rename(Dim::X, Dim::Y);
  EXPECT_EQ(flat, expected);
}

TEST(ReshapeTest, flatten_binedges_x_fails) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  // Throws because x coord has mismatching bin edges.
  EXPECT_THROW_DISCARD(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z),
                       except::BinEdgeError);
}

TEST(ReshapeTest, flatten_binedges_y_fails) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 5) + 0.2 * units::one);

  // Throws because y coord has mismatching bin edges.
  EXPECT_THROW_DISCARD(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z),
                       except::BinEdgeError);
}

TEST(ReshapeTest, round_trip_binedges) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  auto reshaped = fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            a);
}

TEST(ReshapeTest, fold_x_with_attrs) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);
  a.attrs().set(Dim("attr_x"), arange(Dim::X, 6) + 0.3 * units::one);
  a.attrs().set(Dim("attr_y"), arange(Dim::Y, 4) + 0.4 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X, fold(arange(Dim::X, 6), Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}) +
                  0.1 * units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);
  expected.attrs().set(Dim("attr_x"), fold(arange(Dim::X, 6), Dim::X,
                                           {{Dim::Row, 2}, {Dim::Time, 3}}) +
                                          0.3 * units::one);
  expected.attrs().set(Dim("attr_y"), a.attrs()[Dim("attr_y")]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, flatten_with_attrs) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);
  a.attrs().set(Dim("attr_x"), arange(Dim::X, 6) + 0.3 * units::one);
  a.attrs().set(Dim("attr_y"), arange(Dim::Y, 4) + 0.4 * units::one);

  const auto rshp = arange(Dim::Z, 24);
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.1, 0.1, 0.1, 0.1, 1.1, 1.1, 1.1, 1.1,
                                  2.1, 2.1, 2.1, 2.1, 3.1, 3.1, 3.1, 3.1,
                                  4.1, 4.1, 4.1, 4.1, 5.1, 5.1, 5.1, 5.1}));
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));
  expected.attrs().set(
      Dim("attr_x"),
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.3, 0.3, 0.3, 0.3, 1.3, 1.3, 1.3, 1.3,
                                  2.3, 2.3, 2.3, 2.3, 3.3, 3.3, 3.3, 3.3,
                                  4.3, 4.3, 4.3, 4.3, 5.3, 5.3, 5.3, 5.3}));
  expected.attrs().set(
      Dim("attr_y"),
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.4, 1.4, 2.4, 3.4, 0.4, 1.4, 2.4, 3.4,
                                  0.4, 1.4, 2.4, 3.4, 0.4, 1.4, 2.4, 3.4,
                                  0.4, 1.4, 2.4, 3.4, 0.4, 1.4, 2.4, 3.4}));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, fold_x_with_2d_coord) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X,
                 fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}}) +
                     0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(Dim::X,
                        fold(arange(Dim::X, 24), Dim::X,
                             {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}}) +
                            0.1 * units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, flatten_with_2d_coord) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X,
                 fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}}) +
                     0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);

  const auto rshp = arange(Dim::Z, 24);
  DataArray expected(rshp);
  expected.coords().set(Dim::X, arange(Dim::Z, 24) + 0.1 * units::one);
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, fold_x_with_masks) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);
  a.masks().set("mask_x", makeVariable<bool>(
                              Dims{Dim::X}, Shape{6},
                              Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                             Values{true, true, false, true}));
  a.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X, fold(arange(Dim::X, 6), Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}) +
                  0.1 * units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);
  expected.masks().set(
      "mask_x",
      makeVariable<bool>(Dims{Dim::Row, Dim::Time}, Shape{2, 3},
                         Values{true, true, true, false, false, false}));
  expected.masks().set("mask_y",
                       makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                          Values{true, true, false, true}));
  expected.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::Row, Dim::Time, Dim::Y}, Shape{2, 3, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, flatten_with_masks) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);
  a.masks().set("mask_x", makeVariable<bool>(
                              Dims{Dim::X}, Shape{6},
                              Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                             Values{true, true, false, true}));
  a.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  const auto rshp = arange(Dim::Z, 24);
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.1, 0.1, 0.1, 0.1, 1.1, 1.1, 1.1, 1.1,
                                  2.1, 2.1, 2.1, 2.1, 3.1, 3.1, 3.1, 3.1,
                                  4.1, 4.1, 4.1, 4.1, 5.1, 5.1, 5.1, 5.1}));
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));

  expected.masks().set(
      "mask_x",
      makeVariable<bool>(Dims{Dim::Z}, Shape{24},
                         Values{true,  true,  true,  true,  true,  true,
                                true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                false, false, false, false, false, false}));
  expected.masks().set(
      "mask_y", makeVariable<bool>(
                    Dims{Dim::Z}, Shape{24},
                    Values{true, true, false, true, true, true, false, true,
                           true, true, false, true, true, true, false, true,
                           true, true, false, true, true, true, false, true}));
  expected.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::Z}, Shape{24},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, round_trip_with_all) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * units::one);
  a.coords().set(Dim::Z,
                 fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}}) +
                     0.5 * units::one);
  a.attrs().set(Dim("attr_x"), arange(Dim::X, 6) + 0.3 * units::one);
  a.attrs().set(Dim("attr_y"), arange(Dim::Y, 4) + 0.4 * units::one);
  a.masks().set("mask_x", makeVariable<bool>(
                              Dims{Dim::X}, Shape{6},
                              Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                             Values{true, true, false, true}));
  a.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));
  auto reshaped = fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            a);
}

class TransposeTest : public ::testing::Test {
protected:
  TransposeTest() : a(xy) {
    a.coords().set(Dim::X, x);
    a.coords().set(Dim::Y, y);
    a.masks().set("mask-x", x);
    a.masks().set("mask-y", y);
    a.masks().set("mask-xy", xy);
  }
  Variable xy = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 2},
                                     Values{1, 2, 3, 4, 5, 6});
  Variable x = xy.slice({Dim::Y, 0});
  Variable y = xy.slice({Dim::X, 0});
  DataArray a;
};

TEST_F(TransposeTest, data_array_2d) {
  auto transposed = transpose(a);
  EXPECT_EQ(transposed.data(), transpose(a.data()));
  transposed.setData(a.data());
  EXPECT_EQ(transposed, a);
  EXPECT_EQ(transpose(a, {Dim::X, Dim::Y}), transpose(a));
  EXPECT_EQ(transpose(a, {Dim::Y, Dim::X}), a);
}

TEST_F(TransposeTest, data_array_2d_meta_data) {
  Variable edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3},
                                        Values{1, 2, 3, 4, 5, 6, 7, 8, 9});
  // Note: The 2-d coord must not be transposed, since this would break the
  // association with its dimension. Mask may in principle be transposed but is
  // not right now.
  a.coords().set(Dim("edges"), edges);
  a.masks().set("mask", xy);
  a.attrs().set(Dim("attr"), xy);
  auto transposed = transpose(a);
  EXPECT_EQ(transposed.data(), transpose(a.data()));
  transposed.setData(a.data());
  EXPECT_EQ(transposed, a);
  EXPECT_EQ(transpose(a, {Dim::X, Dim::Y}), transpose(a));
  EXPECT_EQ(transpose(a, {Dim::Y, Dim::X}), a);
}

TEST_F(TransposeTest, dataset_no_order) {
  Dataset d;
  d.setData("a", a);
  d.setData("b", transpose(a));
  d.setData("c", a.slice({Dim::X, 0}));
  // Slightly unusual but "simple" behavior if no dim order given
  auto transposed = transpose(d);
  EXPECT_EQ(transposed["a"], d["b"]);
  EXPECT_EQ(transposed["b"], d["a"]);
  EXPECT_EQ(transposed["c"], d["c"]);
}

TEST_F(TransposeTest, dataset_2d) {
  Dataset d;
  d.setData("a", a);
  d.setData("b", transpose(a));
  auto transposed = transpose(d, {Dim::X, Dim::Y});
  EXPECT_EQ(transposed["a"], d["b"]);
  EXPECT_EQ(transposed["b"], d["b"]);
  d.setData("c", a.slice({Dim::X, 0}));
  EXPECT_THROW_DISCARD(transpose(d, {Dim::X, Dim::Y}), except::DimensionError);
}

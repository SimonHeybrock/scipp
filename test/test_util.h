// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include "scipp/variable/variable.h"

using namespace scipp;

inline Variable arange(const Dim dim, const scipp::index shape) {
  std::vector<double> values(shape);
  for (scipp::index i = 0; i < shape; ++i)
    values[i] = i;
  return makeVariable<double>(Dims{dim}, Shape{shape}, Values(values));
}

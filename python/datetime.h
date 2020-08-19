// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include <iomanip>
#include <sstream>

#include <chrono>
#include <cmath>
#include <ctime>
#include <datetime.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "scipp/core/dtype.h"

namespace py = pybind11;

// type caster: scipp::core::time_point <-> NumPy-datetime64
namespace pybind11 {
namespace detail {
template <> struct type_caster<scipp::core::time_point> {
public:
  PYBIND11_TYPE_CASTER(scipp::core::time_point, _("numpy.datetime64"));

  // Conversion part 1 (Python -> C++)
  bool load(py::handle src, bool convert) {
    if (!convert)
      return false;

    if (!src)
      return false;
    // Get timestamp (ns precision) from the numpy object
    int64_t timestamp = src.attr("astype")("int").cast<int64_t>();
    value = scipp::core::time_point(timestamp);

    return true;
  }

  /// Conversion part 2 (C++ -> Python)
  static py::handle cast(const scipp::core::time_point &src,
                         py::return_value_policy, py::handle var_h) {
    int64_t epoch = src.time_since_epoch();
    auto unit = var_h.attr("unit").cast<py::str>();
    py::object np = py::module::import("numpy");
    py::object nd64 = np.attr("datetime64")(epoch, unit);

    return PyUnicode_FromString(c_str(py::str(nd64)));
  }
};
} // namespace detail
} // namespace pybind11

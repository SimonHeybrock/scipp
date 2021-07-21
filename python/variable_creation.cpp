// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/creation.h"

#include "dtype.h"
#include "py_object.h"
#include "pybind11.h"

using namespace scipp;

namespace py = pybind11;

template <class T> struct MakeZeros {
  static variable::Variable apply(const std::vector<Dim> &dims,
                                  const std::vector<scipp::index> &shape,
                                  const units::Unit &unit,
                                  const bool with_variances) {
    return with_variances
               ? makeVariable<T>(Dimensions{dims, shape}, unit, Values{},
                                 Variances{})
               : makeVariable<T>(Dimensions{dims, shape}, unit, Values{});
  }
};

void init_creation(py::module &m) {
  m.def(
      "empty",
      [](const std::vector<Dim> &dims, const std::vector<scipp::index> &shape,
         const units::Unit &unit, const py::object &dtype,
         const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        py::gil_scoped_release release;
        return variable::empty(Dimensions(dims, shape), unit, dtype_,
                               with_variances);
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = units::one,
      py::arg("dtype") = py::none(), py::arg("with_variances") = std::nullopt);
  m.def(
      "zeros",
      [](const std::vector<Dim> &dims, const std::vector<scipp::index> &shape,
         const units::Unit &unit, const py::object &dtype,
         const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        py::gil_scoped_release release;
        return core::CallDType<
            double, float, int64_t, int32_t, bool, scipp::core::time_point,
            std::string, Eigen::Vector3d,
            Eigen::Matrix3d>::apply<MakeZeros>(dtype_, dims, shape, unit,
                                               with_variances);
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = units::one,
      py::arg("dtype") = py::none(), py::arg("with_variances") = std::nullopt);
  m.def(
      "ones",
      [](const std::vector<Dim> &dims, const std::vector<scipp::index> &shape,
         const units::Unit &unit, const py::object &dtype,
         const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        py::gil_scoped_release release;
        return variable::ones(Dimensions(dims, shape), unit, dtype_,
                              with_variances);
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = units::one,
      py::arg("dtype") = py::none(), py::arg("with_variances") = std::nullopt);

  m.def(
      "fill",
      [](const std::vector<Dim> &dims, const std::vector<scipp::index> &shape,
         const units::Unit &unit, const py::object &fillValue,
         const py::object &fillVariance, const py::object &dtype) {

        py::gil_scoped_release release;

        const auto dtype_ = common_dtype(fillValue, fillVariance, scipp_dtype(dtype));

        const auto make_prototype = [&]() {
          return Variable{dtype_, Dimensions{}, unit, Values{py::float_(fillValue)},
                          Variances{py::float_(fillVariance)}};
        };

        return copy(make_prototype().broadcast(Dimensions{dims, shape}));
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = units::one, py::arg("value") = std::nullopt, 
          py::arg("variance") = std::nullopt, py::arg("dtype") = std::nullopt);
        }

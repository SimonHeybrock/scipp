// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "numpy.h"
#include "pybind11.h"
#include "scipp/core/dtype.h"
#include "scipp/core/slice.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/slice.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/variable.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

template <class T> auto dim_extent(const T &object, const Dim dim) {
  if constexpr (std::is_same_v<T, Dataset>) {
    scipp::index extent = -1;
    if (object.sizes().contains(dim))
      extent = object.sizes().at(dim);
    return extent;
  } else {
    return object.dims()[dim];
  }
}

template <class T>
auto from_py_slice(const T &source,
                   const std::tuple<Dim, const py::slice> &index) {
  const auto &[dim, indices] = index;
  size_t start, stop, step, slicelength;
  const auto size = dim_extent(source, dim);
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  if (slicelength == 0) {
    stop = start; // Propagate vanishing slice length downstream.
  }
  return Slice(dim, start, stop);
}

template <class View> struct SetData {
  template <class T> struct Impl {
    static void apply(View &slice, const py::object &obj) {
      if (slice.hasVariances())
        throw std::runtime_error("Data object contains variances, to set data "
                                 "values use the `values` property or provide "
                                 "a tuple of values and variances.");

      copy_array_into_view(cast_to_array_like<T>(obj, slice.unit()),
                           slice.template values<T>(), slice.dims());
    }
  };
};

template <class T>
auto get_slice(T &self, const std::tuple<Dim, scipp::index> &index) {
  auto [dim, i] = index;
  auto sz = dim_extent(self, dim);
  if (i < -sz || i >= sz) // index is out of range
    throw std::runtime_error(
        "The requested index " + std::to_string(i) +
        " is out of range. Dimension size is " + std::to_string(sz) +
        " and the allowed range is [" + std::to_string(-sz) + ":" +
        std::to_string(sz - 1) + "].");
  if (i < 0)
    i = sz + i;
  return Slice(dim, i);
}

template <class T>
auto get_slice_range(T &self, const std::tuple<Dim, const py::slice> &index) {
  auto [dim, py_slice] = index;
  if constexpr (std::is_same_v<T, DataArray> || std::is_same_v<T, Dataset>) {
    try {
      auto start = py::getattr(py_slice, "start");
      auto stop = py::getattr(py_slice, "stop");
      if (!start.is_none() || !stop.is_none()) { // Means default slice : is
                                                 // treated as index slice
        auto start_var = start.is_none()
                             ? Variable{}
                             : py::getattr(py_slice, "start").cast<Variable>();
        auto stop_var = stop.is_none()
                            ? Variable{}
                            : py::getattr(py_slice, "stop").cast<Variable>();
        auto step = py::getattr(py_slice, "step");
        if (!step.is_none()) {
          throw std::runtime_error(
              "Step cannot be specified for value based slicing.");
        }
        return std::make_from_tuple<Slice>(get_slice_params(
            self.dims(), self.coords()[dim], start_var, stop_var));
      }
    } catch (const py::cast_error &) {
    }
  }
  return from_py_slice(self, index);
}

template <class T>
auto getitem(T &self, const std::tuple<Dim, scipp::index> &index) {
  return self.slice(get_slice(self, index));
}

template <class T>
auto getitem(T &self, const std::tuple<Dim, py::slice> &index) {
  return self.slice(get_slice_range(self, index));
}

template <class T> auto getitem(T &self, const py::ellipsis &) {
  return self.slice({});
}

// Helpers wrapped in struct to avoid unresolvable overloads.
template <class T> struct slicer {
  static auto get_slice_by_value(T &self,
                                 const std::tuple<Dim, Variable> &value) {
    auto [dim, i] = value;
    return std::make_from_tuple<Slice>(
        get_slice_params(self.dims(), self.coords()[dim], i));
  }

  static auto get_by_value(T &self, const std::tuple<Dim, Variable> &value) {
    return self.slice(get_slice_by_value(self, value));
  }

  static void set_from_numpy(T &&slice, const py::object &obj) {
    core::CallDType<double, float, int64_t, int32_t,
                    bool>::apply<SetData<T>::template Impl>(slice.dtype(),
                                                            slice, obj);
  }

  template <class Other>
  static void set_from_view(T &self, const std::tuple<Dim, scipp::index> &index,
                            const Other &data) {
    self.setSlice(get_slice(self, index), data);
  }

  template <class Other>
  static void set_from_view(T &self,
                            const std::tuple<Dim, const py::slice> &index,
                            const Other &data) {
    self.setSlice(get_slice_range(self, index), data);
  }

  template <class Other>
  static void set_from_view(T &self, const py::ellipsis &, const Other &data) {
    self.setSlice(Slice{}, data);
  }

  template <class Other>
  static void set_by_value(T &self, const std::tuple<Dim, Variable> &value,
                           const Other &data) {
    self.setSlice(slicer<T>::get_slice_by_value(self, value), data);
  }

  // Manually dispatch based on the object we are assigning from in order to
  // cast it correctly to a scipp view, numpy array or fallback std::vector.
  // This needs to happen partly based on the dtype which cannot be encoded
  // in the Python bindings directly.
  template <class IndexOrRange>
  static void set(T &self, const IndexOrRange &index, const py::object &data) {
    if constexpr (std::is_same_v<T, Dataset>)
      if (py::isinstance<Dataset>(data))
        return set_from_view(self, index, py::cast<Dataset>(data));

    if constexpr (!std::is_same_v<T, Variable>)
      if (py::isinstance<DataArray>(data))
        return set_from_view(self, index, py::cast<DataArray>(data));

    if (py::isinstance<Variable>(data))
      return set_from_view(self, index, py::cast<Variable>(data));

    if constexpr (!std::is_same_v<T, Dataset>)
      return set_from_numpy(getitem(self, index), data);

    std::ostringstream oss;
    oss << "Cannot to assign a " << py::str(data.get_type())
        << " to a slice of a " << py::type_id<T>();
    throw py::type_error(oss.str());
  }
};

template <class T, class... Ignored>
void bind_slice_methods(pybind11::class_<T, Ignored...> &c) {
  c.def("__getitem__", [](T &self, const std::tuple<Dim, scipp::index> &index) {
    return getitem(self, index);
  });
  c.def("__getitem__", [](T &self, const std::tuple<Dim, py::slice> &index) {
    return getitem(self, index);
  });
  c.def("__getitem__", [](T &self, const py::ellipsis &index) {
    return getitem(self, index);
  });
  c.def("__setitem__", &slicer<T>::template set<std::tuple<Dim, scipp::index>>);
  c.def("__setitem__", &slicer<T>::template set<std::tuple<Dim, py::slice>>);
  c.def("__setitem__", &slicer<T>::template set<py::ellipsis>);
  if constexpr (std::is_same_v<T, DataArray>) {
    c.def("__getitem__", &slicer<T>::get_by_value);
    c.def("__setitem__", &slicer<T>::template set_by_value<Variable>);
    c.def("__setitem__", &slicer<T>::template set_by_value<DataArray>);
  }
  if constexpr (std::is_same_v<T, Dataset>) {
    c.def("__getitem__", &slicer<T>::get_by_value);
    c.def("__setitem__", &slicer<T>::template set_by_value<Dataset>);
  } else {
    c.def("__len__", [](const T &self) {
      if (self.dims().ndim() == 0)
        throw except::TypeError("len() of unsized object");
      return self.dims().size(0);
    });
  }
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/groupby.h"
#include "scipp/dataset/dataset.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

// template <class T>
// Docstring docstring_groupby(const std::string &op) {
//   return       Docstring()
//           .description("Element-wise " + op + "over the specified dimension
//           within a group.") .raises("If the unit is not a plane-angle unit,
//           or if the atan2 "
//                   "function cannot be computed on the dtype, e.g., if it is
//                   an " "integer.")
//           .returns("The atan2 values of the input.")

//       Element-wise mean over the specified dimension within a group.

//       :param dim: Dimension to sum over when computing the mean
//       :type dim: Dim
//       :return: Mean over each group, combined along dimension specified when
//       calling :py:func:`scipp.groupby` :rtype: DataArray or Dataset)");

// }

template <class T> void bind_groupby(py::module &m, const std::string &name) {
  m.def("groupby",
        py::overload_cast<const typename T::const_view_type &, const Dim>(
            &groupby),
        py::arg("data"), py::arg("group"),
        py::call_guard<py::gil_scoped_release>(),
        R"(
        Group dataset or data array based on values of specified labels.

        :param data: Input dataset or data array
        :param group: Name of labels to use for grouping
        :type data: DataArray or Dataset
        :type group: str
        :return: GroupBy helper object.
        :rtype: GroupByDataArray or GroupByDataset)");
  m.def("groupby",
        py::overload_cast<const typename T::const_view_type &, const Dim,
                          const VariableConstView &>(&groupby),
        py::arg("data"), py::arg("group"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>(),
        R"(
        Group dataset or data array based on values of specified labels.

        :param data: Input dataset or data array
        :param group: Name of labels to use for grouping
        :param bins: Bins for grouping label values
        :type data: DataArray or Dataset
        :type group: str
        :type bins: VariableConstView
        :return: GroupBy helper object.
        :rtype: GroupByDataArray or GroupByDataset)");

  py::class_<GroupBy<T>> groupBy(m, name.c_str(), R"(
    GroupBy object implementing to split-apply-combine mechanism.)");

  groupBy.def("flatten", &GroupBy<T>::flatten, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Flatten specified dimension into event lists.

      This is a event-data equivalent to calling ``sum`` on dense data.
      In particular, summing the result of histogrammed data yields the same result as histgramming data that has been flattened.

      :param dim: Dimension to flatten
      :type dim: Dim
      :return: Flattened event data for each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("mean", &GroupBy<T>::mean, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise mean over the specified dimension within a group.

      :param dim: Dimension to sum over when computing the mean
      :type dim: Dim
      :return: Mean over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("sum", &GroupBy<T>::sum, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise sum over the specified dimension within a group.

      :param dim: Dimension to sum over
      :type dim: Dim
      :return: Sum over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("all", &GroupBy<T>::all, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise AND over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: AND over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("any", &GroupBy<T>::any, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise OR over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: OR over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("max", &GroupBy<T>::max, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise max over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: Max over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");

  groupBy.def("min", &GroupBy<T>::min, py::arg("dim"),
              py::call_guard<py::gil_scoped_release>(), R"(
      Element-wise min over the specified dimension within a group.

      :param dim: Dimension to reduce
      :type dim: Dim
      :return: Min over each group, combined along dimension specified when calling :py:func:`scipp.groupby`
      :rtype: DataArray or Dataset)");
}

void init_groupby(py::module &m) {
  bind_groupby<DataArray>(m, "GroupByDataArray");
  bind_groupby<Dataset>(m, "GroupByDataset");
}

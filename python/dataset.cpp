// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/generated_comparison.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/map_view.h"
#include "scipp/dataset/math.h"
#include "scipp/dataset/rebin.h"
#include "scipp/dataset/sort.h"
#include "scipp/dataset/util.h"

#include "bind_data_access.h"
#include "bind_data_array.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "detail.h"
#include "docstring.h"
#include "pybind11.h"
#include "rename.h"
#include "view.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T, class... Ignored>
void bind_dataset_coord_properties(py::class_<T, Ignored...> &c) {
  // For some reason the return value policy and/or keep-alive policy do not
  // work unless we wrap things in py::cpp_function.
  c.def_property_readonly(
      "coords",
      py::cpp_function([](T &self) { return self.coords(); },
                       py::return_value_policy::move, py::keep_alive<0, 1>()),
      R"(
      Dict of coordinates.)");
  // Metadata for dataset is same as `coords` since dataset cannot have attrs
  // (unaligned coords).
  c.def_property_readonly("meta",
                          py::cpp_function([](T &self) { return self.meta(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of coordinates.)");
}

template <class T, class... Ignored>
void bind_dataset_view_methods(py::class_<T, Ignored...> &c) {
  bind_common_operators(c);
  c.def("__len__", &T::size);
  c.def(
      "__iter__",
      [](const T &self) {
        return py::make_iterator(self.keys_begin(), self.keys_end(),
                                 py::return_value_policy::move);
      },
      py::return_value_policy::move, py::keep_alive<0, 1>());
  c.def(
      "keys", [](T &self) { return keys_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(view on self's keys)");
  c.def(
      "values", [](T &self) { return values_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(view on self's values)");
  c.def(
      "items", [](T &self) { return items_view(self); },
      py::return_value_policy::move, py::keep_alive<0, 1>(),
      R"(view on self's items)");
  c.def(
      "__getitem__",
      [](T &self, const std::string &name) { return self[name]; },
      py::keep_alive<0, 1>());
  c.def("__contains__", &T::contains);
  c.def_property_readonly(
      "dims",
      [](const T &self) {
        std::vector<std::string> dims;
        for (const auto &dim : self.dimensions()) {
          dims.push_back(dim.first.name());
        }
        return dims;
      },
      R"(List of dimensions.)", py::return_value_policy::move);
  c.def_property_readonly(
      "shape",
      [](const T &self) {
        std::vector<int64_t> shape;
        for (const auto &dim : self.dimensions()) {
          shape.push_back(dim.second);
        }
        return shape;
      },
      R"(List of shapes.)", py::return_value_policy::move);
}

template <class T, class... Ignored>
void bind_data_array(py::class_<T, Ignored...> &c) {
  bind_data_array_properties(c);
  bind_common_operators(c);
  bind_data_properties(c);
  bind_slice_methods(c);
  bind_in_place_binary<DataArrayView>(c);
  bind_in_place_binary<VariableConstView>(c);
  bind_binary<Dataset>(c);
  bind_binary<DatasetView>(c);
  bind_binary<DataArrayView>(c);
  bind_binary<VariableConstView>(c);
  bind_comparison<DataArrayConstView>(c);
  bind_comparison<VariableConstView>(c);
  bind_unary(c);
  bind_logical<DataArray>(c);
  bind_logical<Variable>(c);
}

template <class T> void bind_rebin(py::module &m) {
  m.def("rebin",
        py::overload_cast<const typename T::const_view_type &, const Dim,
                          const VariableConstView &>(&rebin),
        py::arg("x"), py::arg("dim"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>());
}

void init_dataset(py::module &m) {
  py::class_<Slice>(m, "Slice");

  bind_helper_view<items_view, Dataset>(m, "Dataset");
  bind_helper_view<items_view, DatasetView>(m, "DatasetView");
  bind_helper_view<str_items_view, CoordsView>(m, "CoordsView");
  bind_helper_view<items_view, MasksView>(m, "MasksView");
  bind_helper_view<keys_view, Dataset>(m, "Dataset");
  bind_helper_view<keys_view, DatasetView>(m, "DatasetView");
  bind_helper_view<str_keys_view, CoordsView>(m, "CoordsView");
  bind_helper_view<keys_view, MasksView>(m, "MasksView");
  bind_helper_view<values_view, Dataset>(m, "Dataset");
  bind_helper_view<values_view, DatasetView>(m, "DatasetView");
  bind_helper_view<values_view, CoordsView>(m, "CoordsView");
  bind_helper_view<values_view, MasksView>(m, "MasksView");

  bind_mutable_view_no_dim<CoordsView, CoordsConstView>(m, "Coords");
  bind_mutable_view<MasksView, MasksConstView>(m, "Masks");

  py::class_<DataArray> dataArray(m, "DataArray", R"(
    Named variable with associated coords, masks, and attributes.)");
  py::options options;
  options.disable_function_signatures();
  dataArray
      .def(
          py::init([](VariableConstView data,
                      std::map<Dim, VariableConstView> coords,
                      std::map<std::string, VariableConstView> masks,
                      std::map<Dim, VariableConstView> unaligned_coords,
                      const std::string &name) {
            return DataArray{Variable{data}, coords, masks, unaligned_coords,
                             name};
          }),
          py::arg("data"),
          py::arg("coords") = std::map<Dim, VariableConstView>{},
          py::arg("masks") = std::map<std::string, VariableConstView>{},
          py::arg("attrs") = std::map<Dim, VariableConstView>{},
          py::arg("name") = std::string{},
          R"(__init__(self, data: Variable, coords: Dict[str, Variable] = {}, masks: Dict[str, Variable] = {}, attrs: Dict[str, Variable] = {}, name: str = '') -> None

          DataArray initialiser.

          :param data: Data and optionally variances.
          :param coords: Coordinates referenced by dimension.
          :param masks: Masks referenced by name.
          :param attrs: Attributes referenced by dimension.
          :param name: Name of DataArray.
          :type data: Variable
          :type coords: Dict[str, Variable]
          :type masks: Dict[str, Variable]
          :type attrs: Dict[str, Variable]
          :type name: str
          )")
      .def("__sizeof__", [](const DataArrayConstView &array) {
        return size_of(array, true);
      });
  options.enable_function_signatures();
  py::class_<DataArrayConstView>(m, "DataArrayConstView")
      .def(py::init<const DataArray &>())
      .def("__sizeof__", [](const DataArrayConstView &array) {
        return size_of(array, true);
      });

  py::class_<DataArrayView, DataArrayConstView> dataArrayView(
      m, "DataArrayView", R"(
        View for DataArray, representing a sliced view onto a DataArray, or an item of a Dataset;
        Mostly equivalent to DataArray, see there for details.)");

  options.disable_function_signatures();
  dataArrayView.def(py::init<DataArray &>(), py::arg("dataArray"),
                    R"(__init__(self, dataArray: DataArray) -> None

                    DataArrayView initialiser.

                    :param dataArray: Viewed DataArray.
                    :type dataArray: DataArray
                    )");
  options.enable_function_signatures();

  bind_data_array(dataArray);
  bind_data_array(dataArrayView);

  py::class_<DatasetConstView>(m, "DatasetConstView")
      .def(py::init<const Dataset &>())
      .def("__sizeof__", py::overload_cast<const DatasetConstView &>(&size_of));
  py::class_<DatasetView, DatasetConstView> datasetView(m, "DatasetView",
                                                        R"(
        View for Dataset, representing a sliced view onto a Dataset;
        Mostly equivalent to Dataset, see there for details.)");

  options.disable_function_signatures();
  datasetView.def(py::init<Dataset &>(), py::arg("dataset"),
                  R"(__init__(dataset: Dataset) -> None
                    Initialises from viewed Dataset.
                    )");

  py::class_<Dataset> dataset(m, "Dataset", R"(
  Dict of data arrays with aligned dimensions.)");

  dataset.def(
      py::init(
          [](const std::map<std::string,
                            std::variant<VariableConstView, DataArrayConstView>>
                 &data,
             const std::map<Dim, VariableConstView> &coords) {
            Dataset d;
            for (auto &&[dim, coord] : coords)
              d.setCoord(dim, std::move(coord));

            for (auto &&[name, item] : data) {
              auto visitor = [&d, name = name](auto &object) {
                d.setData(std::string(name), std::move(object));
              };
              std::visit(visitor, item);
            }
            return d;
          }),
      py::arg("data") =
          std::map<std::string,
                   std::variant<VariableConstView, DataArrayConstView>>{},
      py::arg("coords") = std::map<Dim, VariableConstView>{},
      R"(__init__(self, data: Dict[str, Union[Variable, DataArray]] = {}, coords: Dict[str, Variable] = {}) -> None

              Dataset initialiser.

             :param data: Dictionary of name and data pairs.
             :param coords: Dictionary of name and coord pairs.
             :type data: Dict[str, Union[Variable, DataArray]]
             :type coords: Dict[str, Variable]
             )");
  options.enable_function_signatures();

  dataset
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const VariableConstView &data) { self.setData(name, data); })
      .def(
          "__setitem__",
          [](Dataset &self, const std::string &name, Moveable<Variable> &mvar) {
            self.setData(name, std::move(mvar.value));
          })
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              const DataArrayConstView &data) { self.setData(name, data); })
      .def("__setitem__",
           [](Dataset &self, const std::string &name,
              Moveable<DataArray> &mdat) {
             self.setData(name, std::move(mdat.value));
           })
      .def("__delitem__", &Dataset::erase,
           py::call_guard<py::gil_scoped_release>())
      .def("clear", &Dataset::clear,
           R"(Removes all data, preserving coordinates.)")
      .def("__sizeof__", py::overload_cast<const DatasetConstView &>(&size_of));
  datasetView.def(
      "__setitem__",
      [](const DatasetView &self, const std::string &name,
         const DataArrayConstView &data) { self[name].assign(data); });

  bind_dataset_view_methods(dataset);
  bind_dataset_view_methods(datasetView);

  bind_dataset_coord_properties(dataset);
  bind_dataset_coord_properties(datasetView);

  bind_slice_methods(dataset);
  bind_slice_methods(datasetView);

  bind_in_place_binary<Dataset>(dataset);
  bind_in_place_binary<DatasetView>(dataset);
  bind_in_place_binary<DataArrayView>(dataset);
  bind_in_place_binary<VariableConstView>(dataset);
  bind_in_place_binary<Dataset>(datasetView);
  bind_in_place_binary<DatasetView>(datasetView);
  bind_in_place_binary<VariableConstView>(datasetView);
  bind_in_place_binary<DataArrayView>(datasetView);
  bind_in_place_binary_scalars(dataset);
  bind_in_place_binary_scalars(datasetView);
  bind_in_place_binary_scalars(dataArray);
  bind_in_place_binary_scalars(dataArrayView);

  bind_binary<Dataset>(dataset);
  bind_binary<DatasetView>(dataset);
  bind_binary<DataArrayView>(dataset);
  bind_binary<VariableConstView>(dataset);
  bind_binary<Dataset>(datasetView);
  bind_binary<DatasetView>(datasetView);
  bind_binary<DataArrayView>(datasetView);
  bind_binary<VariableConstView>(datasetView);

  dataArray.def("rename_dims", &rename_dims<DataArray>, py::arg("dims_dict"),
                "Rename dimensions.");
  dataset.def("rename_dims", &rename_dims<Dataset>, py::arg("dims_dict"),
              "Rename dimensions.");

  m.def(
      "merge",
      [](const DatasetConstView &lhs, const DatasetConstView &rhs) {
        return dataset::merge(lhs, rhs);
      },
      py::arg("lhs"), py::arg("rhs"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Union of two datasets.")
          .raises("If there are conflicting items with different content.")
          .returns("A new dataset that contains the union of all data items, "
                   "coords, masks and attributes.")
          .rtype("Dataset")
          .param("lhs", "First Dataset", "Dataset")
          .param("rhs", "Second Dataset", "Dataset")
          .c_str());

  m.def(
      "combine_masks",
      [](const MasksConstView &msk, const std::vector<Dim> &labels,
         const std::vector<scipp::index> &shape) {
        return dataset::masks_merge_if_contained(msk,
                                                 Dimensions(labels, shape));
      },
      py::arg("masks"), py::arg("labels"), py::arg("shape"),
      py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description(
              "Combine all masks into a single one following the OR operation. "
              "This requires a masks view as an input, followed by the "
              "dimension labels and shape of the Variable/DataArray. The "
              "labels and the shape are used to create a Dimensions object. "
              "The function then iterates through the masks view and combines "
              "only the masks that have all their dimensions contained in the "
              "Variable/DataArray Dimensions.")
          .returns("A new variable that contains the union of all masks.")
          .rtype("Variable")
          .param("masks", "Masks view of the dataset's masks.", "MaskView")
          .param("labels", "A list of dimension labels.", "list")
          .param("shape", "A list of dimension extents.", "list")
          .c_str());

  m.def(
      "reciprocal",
      [](const DataArrayConstView &self) { return reciprocal(self); },
      py::arg("x"), py::call_guard<py::gil_scoped_release>(),
      Docstring()
          .description("Element-wise reciprocal.")
          .raises("If the dtype has no reciprocal, e.g., if it is a string.")
          .returns("The reciprocal values of the input.")
          .rtype("DataArray")
          .param("x", "Input data array.", "DataArray")
          .c_str());

  bind_astype(dataArray);
  bind_astype(dataArrayView);

  bind_rebin<DataArray>(m);
  bind_rebin<Dataset>(m);

  py::implicitly_convertible<DataArray, DataArrayConstView>();
  py::implicitly_convertible<DataArray, DataArrayView>();
  py::implicitly_convertible<Dataset, DatasetConstView>();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable_factory.h"

#include "bind_operators.h"
#include "detail.h"
#include "pybind11.h"
#include "view.h"

namespace py = pybind11;
using namespace scipp;

template <template <class> class View, class T>
void bind_helper_view(py::module &m, const std::string &name) {
  std::string suffix;
  if (std::is_same_v<View<T>, items_view<T>> ||
      std::is_same_v<View<T>, str_items_view<T>>)
    suffix = "_items_view";
  if (std::is_same_v<View<T>, values_view<T>>)
    suffix = "_values_view";
  if (std::is_same_v<View<T>, keys_view<T>> ||
      std::is_same_v<View<T>, str_keys_view<T>>)
    suffix = "_keys_view";
  py::class_<View<T>>(m, (name + suffix).c_str())
      .def(py::init([](T &obj) { return View{obj}; }))
      .def("__len__", &View<T>::size)
      .def(
          "__iter__",
          [](View<T> &self) {
            return py::make_iterator(self.begin(), self.end(),
                                     py::return_value_policy::move);
          },
          py::return_value_policy::move, py::keep_alive<0, 1>());
}

template <class Other, class T, class... Ignored>
void bind_common_mutable_view_operators(pybind11::class_<T, Ignored...> &view) {
  view.def("__len__", &T::size)
      .def("__getitem__", &T::operator[], py::return_value_policy::move,
           py::keep_alive<0, 1>())
      .def("__setitem__",
           [](T &self, const typename T::key_type key,
              const VariableConstView &var) {
             if (self.contains(key) && !is_bins(self[key]) &&
                 self[key].dims().ndim() == var.dims().ndim() &&
                 self[key].dims().contains(var.dims())) {
               self[key].assign(var);
             } else
               self.set(key, var);
           })
      // This additional setitem allows us to do things like
      // d.coords["a"] = scipp.detail.move(scipp.Variable())
      .def("__setitem__",
           [](T &self, const typename T::key_type key,
              Moveable<Variable> &mvar) {
             self.set(key, std::move(mvar.value));
           })
      .def("__delitem__", &T::erase, py::call_guard<py::gil_scoped_release>())
      .def(
          "values", [](T &self) { return values_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's values)")
      .def("__contains__", &T::contains);
}

template <class T, class ConstT>
void bind_mutable_view(py::module &m, const std::string &name) {
  py::class_<ConstT>(m, (name + "ConstView").c_str());
  py::class_<T, ConstT> view(m, (name + "View").c_str());
  bind_common_mutable_view_operators<T>(view);
  bind_inequality_to_operator<T>(view);
  view.def(
          "__iter__",
          [](T &self) {
            return py::make_iterator(self.keys_begin(), self.keys_end(),
                                     py::return_value_policy::move);
          },
          py::keep_alive<0, 1>())
      .def(
          "keys", [](T &self) { return keys_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def(
          "items", [](T &self) { return items_view(self); },
          py::return_value_policy::move, py::keep_alive<0, 1>(),
          R"(view on self's items)");
}

template <class T, class ConstT>
void bind_mutable_view_no_dim(py::module &m, const std::string &name) {
  py::class_<ConstT>(m, (name + "ConstView").c_str());
  py::class_<T, ConstT> view(m, (name + "View").c_str());
  bind_common_mutable_view_operators<T>(view);
  bind_inequality_to_operator<T>(view);
  view.def(
          "__iter__",
          [](T &self) {
            auto keys_view = str_keys_view(self);
            return py::make_iterator(keys_view.begin(), keys_view.end(),
                                     py::return_value_policy::move);
          },
          py::keep_alive<0, 1>())
      .def(
          "keys", [](T &self) { return str_keys_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def(
          "items", [](T &self) { return str_items_view(self); },
          py::return_value_policy::move, py::keep_alive<0, 1>(),
          R"(view on self's items)");
}

template <class T, class... Ignored>
void bind_data_array_properties(py::class_<T, Ignored...> &c) {
  if constexpr (std::is_same_v<T, DataArray>)
    c.def_property("name", &T::name, &T::setName,
                   R"(The name of the held data.)");
  else
    c.def_property_readonly("name", &T::name, R"(The name of the held data.)");
  c.def_property(
      "data",
      py::cpp_function([](T &self) { return self.data(); },
                       py::return_value_policy::move, py::keep_alive<0, 1>()),
      [](T &self, const VariableConstView &data) {
        if constexpr (std::is_convertible_v<T, DataArrayView>)
          self.data().assign(data);
        else // bins_view
          self.setData(data);
      },
      R"(Underlying data item.)");
  c.def_property_readonly(
      "coords",
      py::cpp_function([](T &self) { return self.coords(); },
                       py::return_value_policy::move, py::keep_alive<0, 1>()),
      R"(
      Dict of aligned coords.)");
  c.def_property_readonly("meta",
                          py::cpp_function([](T &self) { return self.meta(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of coords and attrs.)");
  c.def_property_readonly("attrs",
                          py::cpp_function([](T &self) { return self.attrs(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of attrs.)");
  c.def_property_readonly("masks",
                          py::cpp_function([](T &self) { return self.masks(); },
                                           py::return_value_policy::move,
                                           py::keep_alive<0, 1>()),
                          R"(
      Dict of masks.)");
}

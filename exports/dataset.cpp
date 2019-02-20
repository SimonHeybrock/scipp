/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <variant>

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "dataset.h"
#include "except.h"
#include "tag_util.h"
#include "zip_view.h"

namespace py = pybind11;

template <class T> struct mutable_span_methods {
  static void add(py::class_<gsl::span<T>> &span) {
    span.def("__setitem__", [](gsl::span<T> &self, const gsl::index i,
                               const T value) { self[i] = value; });
  }
};
template <class T> struct mutable_span_methods<const T> {
  static void add(py::class_<gsl::span<const T>> &) {}
};

template <class T> void declare_span(py::module &m, const std::string &suffix) {
  py::class_<gsl::span<T>> span(m, (std::string("span_") + suffix).c_str());
  span.def("__getitem__", &gsl::span<T>::operator[],
           py::return_value_policy::reference)
      .def("size", &gsl::span<T>::size)
      .def("__len__", &gsl::span<T>::size)
      .def("__iter__", [](const gsl::span<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
  mutable_span_methods<T>::add(span);
}

template <class... Keys>
void declare_VariableZipProxy(py::module &m, const std::string &suffix,
                              const Keys &... keys) {
  using Proxy = decltype(zip(std::declval<Dataset &>(), keys...));
  py::class_<Proxy> proxy(m,
                          (std::string("VariableZipProxy_") + suffix).c_str());
  proxy.def("__len__", &Proxy::size)
      .def("__getitem__", &Proxy::operator[])
      .def("__iter__",
           [](const Proxy &self) {
             return py::make_iterator(self.begin(), self.end());
           },
           // WARNING The py::keep_alive is really important. It prevents
           // deletion of the VariableZipProxy when its iterators are still in
           // use. This is necessary due to the underlying implementation, which
           // used ranges::view::zip based on temporary gsl::range.
           py::keep_alive<0, 1>());
}

template <class... Fields>
void declare_ItemZipProxy(py::module &m, const std::string &suffix) {
  using Proxy = ItemZipProxy<Fields...>;
  py::class_<Proxy> proxy(m, (std::string("ItemZipProxy_") + suffix).c_str());
  proxy.def("__len__", &Proxy::size).def("__iter__", [](const Proxy &self) {
    return py::make_iterator(self.begin(), self.end());
  });
}

template <class... Fields>
void declare_ranges_pair(py::module &m, const std::string &suffix) {
  using Proxy = ranges::v3::common_pair<Fields &...>;
  py::class_<Proxy> proxy(m, (std::string("ranges_v3_common_pair_") + suffix).c_str());
  proxy.def("first", [](const Proxy &self) { return std::get<0>(self); })
      .def("second", [](const Proxy &self) { return std::get<1>(self); });
}

template <class T>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<VariableView<T>> view(
      m, (std::string("VariableView_") + suffix).c_str());
  view.def("__getitem__", &VariableView<T>::operator[],
           py::return_value_policy::reference)
      .def("__setitem__", [](VariableView<T> &self, const gsl::index i,
                             const T value) { self[i] = value; })
      .def("__len__", &VariableView<T>::size)
      .def("__iter__", [](const VariableView<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
}

/// Helper to pass "default" dtype.
struct Empty {
  char dummy;
};

DType convertDType(const py::dtype type) {
  if (type.is(py::dtype::of<double>()))
    return dtype<double>;
  if (type.is(py::dtype::of<float>()))
    return dtype<float>;
  // See https://github.com/pybind/pybind11/pull/1329, int64_t not matching
  // numpy.int64 correctly.
  if (type.is(py::dtype::of<std::int64_t>()) ||
      (type.kind() == 'i' && type.itemsize() == 8))
    return dtype<int64_t>;
  if (type.is(py::dtype::of<int32_t>()))
    return dtype<int32_t>;
  if (type.is(py::dtype::of<bool>()))
    return dtype<bool>;
  throw std::runtime_error("unsupported dtype");
}

namespace detail {
template <class T> struct MakeVariable {
  static Variable apply(const Tag tag, const std::vector<Dim> &labels,
                        py::array data) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> dataT(data);
    const py::buffer_info info = dataT.request();
    Dimensions dims(labels, info.shape);
    auto *ptr = (T *)info.ptr;
    return ::makeVariable<T>(tag, dims, ptr, ptr + dims.volume());
  }
};

template <class T> struct MakeVariableDefaultInit {
  static Variable apply(const Tag tag, const std::vector<Dim> &labels,
                        const py::tuple &shape) {
    Dimensions dims(labels, shape.cast<std::vector<gsl::index>>());
    return ::makeVariable<T>(tag, dims);
  }
};

Variable makeVariable(const Tag tag, const std::vector<Dim> &labels,
                      py::array &data,
                      py::dtype dtype = py::dtype::of<Empty>()) {
  const py::buffer_info info = data.request();
  // Use custom dtype, otherwise dtype of data.
  const auto dtypeTag = dtype.is(py::dtype::of<Empty>())
                            ? convertDType(data.dtype())
                            : convertDType(dtype);
  return CallDType<double, float, int64_t, int32_t, char,
                   bool>::apply<detail::MakeVariable>(dtypeTag, tag, labels,
                                                      data);
}

Variable makeVariableDefaultInit(const Tag tag, const std::vector<Dim> &labels,
                                 const py::tuple &shape,
                                 py::dtype dtype = py::dtype::of<Empty>()) {
  // TODO Numpy does not support strings, how can we specify std::string as a
  // dtype? The same goes for other, more complex item types we need for
  // variables. Do we need an overload with a dtype arg that does not use
  // py::dtype?
  const auto dtypeTag = dtype.is(py::dtype::of<Empty>()) ? defaultDType(tag)
                                                         : convertDType(dtype);
  return CallDType<double, float, int64_t, int32_t, char, bool,
                   typename Data::EventTofs_t::type>::
      apply<detail::MakeVariableDefaultInit>(dtypeTag, tag, labels, shape);
}

namespace Key {
using Tag = Tag;
using TagName = std::pair<Tag, const std::string &>;
auto get(const Key::Tag &key) {
  static const std::string empty;
  return std::tuple(key, empty);
}
auto get(const Key::TagName &key) { return key; }
} // namespace Key

template <class K>
void insertDefaultInit(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::tuple> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  auto var = makeVariableDefaultInit(tag, labels, array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class K>
void insert_ndarray(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::array &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  const auto dtypeTag = convertDType(array.dtype());
  auto var = CallDType<double, float, int64_t, int32_t, char,
                       bool>::apply<detail::MakeVariable>(dtypeTag, tag, labels,
                                                          array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

// Note the concretely typed py::array_t. If we use py::array it will not match
// plain Python arrays.
template <class T, class K>
void insert_conv(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::array_t<T> &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  // TODO This is converting back and forth between py::array and py::array_t,
  // can we do this in a better way?
  auto var = detail::MakeVariable<T>::apply(tag, labels, array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class T, class K>
void insert_1D(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, std::vector<T> &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  Dimensions dims{labels, {static_cast<gsl::index>(array.size())}};
  auto var = ::makeVariable<T>(tag, dims, array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class Var, class K>
void insert(Dataset &self, const K &key, const Var &var) {
  const auto & [ tag, name ] = Key::get(key);
  if (self.contains(tag, name))
    if (self(tag, name) == var)
      return;
  Variable copy(var);
  copy.setTag(tag);
  if (!name.empty())
    copy.setName(name);
  self.insert(std::move(copy));
}

// Add size factor.
template <class T>
std::vector<gsl::index> numpy_strides(const std::vector<gsl::index> &s) {
  std::vector<gsl::index> strides(s.size());
  gsl::index elemSize = sizeof(T);
  for (size_t i = 0; i < strides.size(); ++i) {
    strides[i] = elemSize * s[i];
  }
  return strides;
}

template <class T> struct SetData {
  static void apply(const VariableSlice &slice, const py::array &data) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> dataT(data);

    const auto &dims = slice.dimensions();
    const py::buffer_info info = dataT.request();
    const auto &shape = dims.shape();
    if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                    shape.end()))
      throw std::runtime_error(
          "Shape mismatch when setting data from numpy array.");

    auto buf = slice.span<T>();
    auto *ptr = (T *)info.ptr;
    std::copy(ptr, ptr + slice.size(), buf.begin());
  }
};

template <class T, class K>
void setData(T &self, const K &key, const py::array &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto slice = self(tag, name);
  CallDType<double, float, int64_t, int32_t, char,
            bool>::apply<detail::SetData>(slice.dtype(), slice, data);
}

VariableSlice pySlice(VariableSlice &view,
                      const std::tuple<Dim, const py::slice> &index) {
  const Dim dim = std::get<Dim>(index);
  const auto indices = std::get<const py::slice>(index);
  size_t start, stop, step, slicelength;
  const auto size = view.dimensions()[dim];
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return view(dim, start, stop);
}

void setVariableSlice(VariableSlice &self,
                      const std::tuple<Dim, gsl::index> &index,
                      const py::array &data) {
  auto slice = self(std::get<Dim>(index), std::get<gsl::index>(index));
  CallDType<double, float, int64_t, int32_t, char,
            bool>::apply<detail::SetData>(slice.dtype(), slice, data);
}

void setVariableSliceRange(VariableSlice &self,
                           const std::tuple<Dim, const py::slice> &index,
                           const py::array &data) {
  auto slice = pySlice(self, index);
  CallDType<double, float, int64_t, int32_t, char,
            bool>::apply<detail::SetData>(slice.dtype(), slice, data);
}
} // namespace detail

template <class T> struct MakePyBufferInfoT {
  static py::buffer_info apply(VariableSlice &view) {
    return py::buffer_info(
        view.template span<T>().data(), /* Pointer to buffer */
        sizeof(T),                      /* Size of one scalar */
        py::format_descriptor<
            std::conditional_t<std::is_same_v<T, bool>, bool, T>>::
            format(),              /* Python struct-style format descriptor */
        view.dimensions().count(), /* Number of dimensions */
        view.dimensions().shape(), /* Buffer dimensions */
        detail::numpy_strides<T>(
            view.strides()) /* Strides (in bytes) for each index */
    );
  }
};

py::buffer_info make_py_buffer_info(VariableSlice &view) {
  return CallDType<double, float, int64_t, int32_t, char,
                   bool>::apply<MakePyBufferInfoT>(view.dtype(), view);
}

template <class T, class Var> auto as_py_array_t(py::object &obj, Var &view) {
  // TODO Should `Variable` also have a `strides` method?
  const auto strides = VariableSlice(view).strides();
  using py_T = std::conditional_t<std::is_same_v<T, bool>, bool, T>;
  return py::array_t<py_T>{view.dimensions().shape(),
                           detail::numpy_strides<T>(strides),
                           (py_T *)view.template span<T>().data(), obj};
}

template <class Var, class... Ts>
std::variant<py::array_t<Ts>...> as_py_array_t_variant(py::object &obj) {
  auto &view = obj.cast<Var &>();
  switch (view.dtype()) {
  case dtype<double>:
    return {as_py_array_t<double>(obj, view)};
  case dtype<float>:
    return {as_py_array_t<float>(obj, view)};
  case dtype<int64_t>:
    return {as_py_array_t<int64_t>(obj, view)};
  case dtype<int32_t>:
    return {as_py_array_t<int32_t>(obj, view)};
  case dtype<char>:
    return {as_py_array_t<char>(obj, view)};
  case dtype<bool>:
    return {as_py_array_t<bool>(obj, view)};
  default:
    throw std::runtime_error("not implemented for this type.");
  }
}

template <class Var, class... Ts>
std::variant<std::conditional_t<std::is_same_v<Var, Variable>,
                                gsl::span<underlying_type_t<Ts>>,
                                VariableView<underlying_type_t<Ts>>>...>
as_VariableView_variant(Var &view) {
  switch (view.dtype()) {
  case dtype<double>:
    return {view.template span<double>()};
  case dtype<float>:
    return {view.template span<float>()};
  case dtype<int64_t>:
    return {view.template span<int64_t>()};
  case dtype<int32_t>:
    return {view.template span<int32_t>()};
  case dtype<char>:
    return {view.template span<char>()};
  case dtype<bool>:
    return {view.template span<bool>()};
  case dtype<std::string>:
    return {view.template span<std::string>()};
  case dtype<boost::container::small_vector<double, 8>>:
    return {view.template span<boost::container::small_vector<double, 8>>()};
  case dtype<Dataset>:
    return {view.template span<Dataset>()};
  default:
    throw std::runtime_error("not implemented for this type.");
  }
}

using small_vector = boost::container::small_vector<double, 8>;
PYBIND11_MAKE_OPAQUE(small_vector);

PYBIND11_MODULE(dataset, m) {
  py::bind_vector<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");

  py::enum_<Dim>(m, "Dim")
      .value("Component", Dim::Component)
      .value("DeltaE", Dim::DeltaE)
      .value("Detector", Dim::Detector)
      .value("DetectorScan", Dim::DetectorScan)
      .value("Energy", Dim::Energy)
      .value("Event", Dim::Event)
      .value("Invalid", Dim::Invalid)
      .value("Monitor", Dim::Monitor)
      .value("Polarization", Dim::Polarization)
      .value("Position", Dim::Position)
      .value("Q", Dim::Q)
      .value("Row", Dim::Row)
      .value("Run", Dim::Run)
      .value("Spectrum", Dim::Spectrum)
      .value("Temperature", Dim::Temperature)
      .value("Time", Dim::Time)
      .value("Tof", Dim::Tof)
      .value("X", Dim::X)
      .value("Y", Dim::Y)
      .value("Z", Dim::Z);

  py::class_<Tag>(m, "Tag").def(py::self == py::self);

  // Runtime tags are sufficient in Python, not exporting Tag child classes.
  auto coord_tags = m.def_submodule("Coord");
  coord_tags.attr("Monitor") = Tag(Coord::Monitor);
  coord_tags.attr("DetectorInfo") = Tag(Coord::DetectorInfo);
  coord_tags.attr("ComponentInfo") = Tag(Coord::ComponentInfo);
  coord_tags.attr("X") = Tag(Coord::X);
  coord_tags.attr("Y") = Tag(Coord::Y);
  coord_tags.attr("Z") = Tag(Coord::Z);
  coord_tags.attr("Tof") = Tag(Coord::Tof);
  coord_tags.attr("Energy") = Tag(Coord::Energy);
  coord_tags.attr("DeltaE") = Tag(Coord::DeltaE);
  coord_tags.attr("Ei") = Tag(Coord::Ei);
  coord_tags.attr("Ef") = Tag(Coord::Ef);
  coord_tags.attr("DetectorId") = Tag(Coord::DetectorId);
  coord_tags.attr("SpectrumNumber") = Tag(Coord::SpectrumNumber);
  coord_tags.attr("DetectorGrouping") = Tag(Coord::DetectorGrouping);
  coord_tags.attr("RowLabel") = Tag(Coord::RowLabel);
  coord_tags.attr("Polarization") = Tag(Coord::Polarization);
  coord_tags.attr("Temperature") = Tag(Coord::Temperature);
  coord_tags.attr("FuzzyTemperature") = Tag(Coord::FuzzyTemperature);
  coord_tags.attr("Time") = Tag(Coord::Time);
  coord_tags.attr("TimeInterval") = Tag(Coord::TimeInterval);
  coord_tags.attr("Mask") = Tag(Coord::Mask);
  coord_tags.attr("Position") = Tag(Coord::Position);

  auto data_tags = m.def_submodule("Data");
  data_tags.attr("Tof") = Tag(Data::Tof);
  data_tags.attr("PulseTime") = Tag(Data::PulseTime);
  data_tags.attr("Value") = Tag(Data::Value);
  data_tags.attr("Variance") = Tag(Data::Variance);
  data_tags.attr("StdDev") = Tag(Data::StdDev);
  data_tags.attr("Events") = Tag(Data::Events);
  data_tags.attr("EventTofs") = Tag(Data::EventTofs);
  data_tags.attr("EventPulseTimes") = Tag(Data::EventPulseTimes);

  auto attr_tags = m.def_submodule("Attr");
  attr_tags.attr("ExperimentLog") = Tag(Attr::ExperimentLog);

  declare_span<double>(m, "double");
  declare_span<float>(m, "float");
  declare_span<Bool>(m, "bool");
  declare_span<const double>(m, "double_const");
  declare_span<const std::string>(m, "string_const");
  declare_span<const Dim>(m, "Dim_const");
  declare_span<Dataset>(m, "Dataset");

  declare_VariableView<double>(m, "double");
  declare_VariableView<float>(m, "float");
  declare_VariableView<int64_t>(m, "int64");
  declare_VariableView<int32_t>(m, "int32");
  declare_VariableView<std::string>(m, "string");
  declare_VariableView<char>(m, "char");
  declare_VariableView<Bool>(m, "bool");
  declare_VariableView<boost::container::small_vector<double, 8>>(m, "SmallVectorDouble8");
  declare_VariableView<Dataset>(m, "Dataset");

  declare_VariableZipProxy(m, "", Access::Key(Data::EventTofs),
                           Access::Key(Data::EventPulseTimes));
  declare_ItemZipProxy<small_vector, small_vector>(m, "");
  declare_ranges_pair<double, double>(m, "double_double");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("__repr__",
           [](const Dimensions &self) {
             std::string out = "Dimensions = " + dataset::to_string(self, ".");
             return out;
           })
      .def("__len__", &Dimensions::count)
      .def("__contains__", [](const Dimensions &self,
                              const Dim dim) { return self.contains(dim); })
      .def_property_readonly("labels", &Dimensions::labels)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dim>(&Dimensions::operator[], py::const_));

  PYBIND11_NUMPY_DTYPE(Empty, dummy);

  py::class_<Variable>(m, "Variable")
      .def(py::init(&detail::makeVariableDefaultInit), py::arg("tag"),
           py::arg("labels"), py::arg("shape"),
           py::arg("dtype") = py::dtype::of<Empty>())
      // TODO Need to add overload for std::vector<std::string>, etc., see
      // Dataset.__setitem__
      .def(py::init(&detail::makeVariable), py::arg("tag"), py::arg("labels"),
           py::arg("data"), py::arg("dtype") = py::dtype::of<Empty>())
      .def(py::init<const VariableSlice &>())
      .def_property_readonly("tag", &Variable::tag)
      .def_property("name", [](const Variable &self) { return self.name(); },
                    &Variable::setName)
      .def_property_readonly("is_coord", &Variable::isCoord)
      .def_property_readonly(
          "dimensions", [](const Variable &self) { return self.dimensions(); })
      .def_property_readonly(
          "numpy", &as_py_array_t_variant<Variable, double, float, int64_t,
                                          int32_t, char, bool>)
      .def_property_readonly(
          "data",
          &as_VariableView_variant<
              Variable, double, float, int64_t, int32_t, char, bool,
              std::string, boost::container::small_vector<double, 8>, Dataset>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def("__repr__",
           [](const Variable &self) { return dataset::to_string(self, "."); });

  py::class_<VariableSlice> view(m, "VariableSlice", py::buffer_protocol());
  view.def_buffer(&make_py_buffer_info);
  view.def_property_readonly(
          "dimensions",
          [](const VariableSlice &self) { return self.dimensions(); },
          py::return_value_policy::copy)
      .def("__len__",
           [](const VariableSlice &self) {
             const auto &dims = self.dimensions();
             if (dims.count() == 0)
               throw std::runtime_error("len() of unsized object.");
             return dims.shape()[0];
           })
      .def_property_readonly("is_coord", &VariableSlice::isCoord)
      .def_property_readonly("tag", &VariableSlice::tag)
      .def_property_readonly("name", &VariableSlice::name)
      .def("__getitem__",
           [](VariableSlice &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__", &detail::pySlice)
      .def("__getitem__",
           [](VariableSlice &self, const std::map<Dim, const gsl::index> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice(item.first, item.second);
             return slice;
           })
      .def("__setitem__", &detail::setVariableSlice)
      .def("__setitem__", &detail::setVariableSliceRange)
      .def_property_readonly(
          "numpy", &as_py_array_t_variant<VariableSlice, double, float, int64_t,
                                          int32_t, char, bool>)
      .def_property_readonly(
          "data",
          &as_VariableView_variant<
              VariableSlice, double, float, int64_t, int32_t, char, bool,
              std::string, boost::container::small_vector<double, 8>, Dataset>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def("__iadd__", [](VariableSlice &a, Variable &b) { return a += b; },
           py::is_operator())
      .def("__isub__", [](VariableSlice &a, Variable &b) { return a -= b; },
           py::is_operator())
      .def("__imul__", [](VariableSlice &a, Variable &b) { return a *= b; },
           py::is_operator())
      .def("__repr__", [](const VariableSlice &self) {
        return dataset::to_string(self, ".");
      });

  py::class_<DatasetSlice>(m, "DatasetView")
      .def(py::init<Dataset &>())
      .def("__len__", &DatasetSlice::size)
      .def("__iter__",
           [](DatasetSlice &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("__contains__", &DatasetSlice::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const DatasetSlice &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](DatasetSlice &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__",
           [](DatasetSlice &self,
              const std::tuple<Dim, const py::slice> &index) {
             const Dim dim = std::get<Dim>(index);
             const auto indices = std::get<const py::slice>(index);
             size_t start, stop, step, slicelength;
             gsl::index size = self.dimensions()[dim];
             if (!indices.compute(size, &start, &stop, &step, &slicelength))
               throw py::error_already_set();
             if (step != 1)
               throw std::runtime_error("Step must be 1");
             return self(dim, start, stop);
           })
      .def("__getitem__",
           [](DatasetSlice &self, const Tag &tag) { return self(tag); })
      .def(
          "__getitem__",
          [](DatasetSlice &self, const std::pair<Tag, const std::string> &key) {
            return self(key.first, key.second);
          })
      .def("__setitem__", detail::setData<DatasetSlice, detail::Key::Tag>)
      .def("__setitem__", detail::setData<DatasetSlice, detail::Key::TagName>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def(py::init<const DatasetSlice &>())
      .def("__len__", &Dataset::size)
      .def("__repr__",
           [](const Dataset &self) {
             auto out = dataset::to_string(self, ".");
             return out;
           })
      .def("__iter__",
           [](Dataset &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("__contains__", &Dataset::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const Dataset &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__delitem__",
           py::overload_cast<const Tag, const std::string &>(&Dataset::erase),
           py::arg("tag"), py::arg("name") = "")
      .def("__delitem__",
           [](Dataset &self,
              const std::tuple<const Tag, const std::string &> &key) {
             self.erase(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__",
           [](Dataset &self, const std::tuple<Dim, const py::slice> &index) {
             const Dim dim = std::get<Dim>(index);
             const auto indices = std::get<const py::slice>(index);
             size_t start, stop, step, slicelength;
             const auto size = self.dimensions()[dim];
             if (!indices.compute(size, &start, &stop, &step, &slicelength))
               throw py::error_already_set();
             if (step != 1)
               throw std::runtime_error("Step must be 1");
             return self(dim, start, stop);
           })
      .def("__getitem__",
           [](Dataset &self, const Tag &tag) { return self(tag); })
      .def("__getitem__",
           [](Dataset &self, const std::pair<Tag, const std::string> &key) {
             return self(key.first, key.second);
           })
      .def("__getitem__",
           [](Dataset &self, const std::string &name) { return self[name]; })
      // Careful: The order of overloads is really important here, otherwise
      // DatasetSlice matches the overload below for py::array_t. I have not
      // understood all details of this yet though. See also
      // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#overload-resolution-order.
      .def("__setitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index,
              const DatasetSlice &other) {
             auto slice =
                 self(std::get<Dim>(index), std::get<gsl::index>(index));
             if (slice == other)
               return;
             throw std::runtime_error("Non-self-assignment of Dataset slices "
                                      "is not implemented yet.\n");
           })

      // A) No dimensions argument, this will set data of existing item.
      .def("__setitem__", detail::setData<Dataset, detail::Key::Tag>)
      .def("__setitem__", detail::setData<Dataset, detail::Key::TagName>)

      // B) Variants with dimensions, inserting new item.
      // 0. Insertion with default init. TODO Should this be removed?
      .def("__setitem__", detail::insertDefaultInit<detail::Key::Tag>)
      .def("__setitem__", detail::insertDefaultInit<detail::Key::TagName>)
      // 1. Insertion from numpy.ndarray
      .def("__setitem__", detail::insert_ndarray<detail::Key::Tag>)
      .def("__setitem__", detail::insert_ndarray<detail::Key::TagName>)
      // 2. Insertion attempting forced conversion to array of double. This
      //    is handled by automatic conversion by pybind11 when using
      //    py::array_t. Handles also scalar data. See also the
      //    py::array::forcecast argument, we need to minimize implicit (and
      //    potentially expensive conversion). If we wanted to avoid some
      //    conversion we need to provide explicit variants for specific types,
      //    same as or similar to insert_1D in case 3. below.
      .def("__setitem__", detail::insert_conv<double, detail::Key::Tag>)
      .def("__setitem__", detail::insert_conv<double, detail::Key::TagName>)
      // 3. Insertion of numpy-incompatible data. py::array_t does not support
      //    non-POD types like std::string, so we need to handle them
      //    separately.
      .def("__setitem__", detail::insert_1D<std::string, detail::Key::Tag>)
      .def("__setitem__", detail::insert_1D<std::string, detail::Key::TagName>)
      .def("__setitem__", detail::insert_1D<Dataset, detail::Key::Tag>)
      .def("__setitem__", detail::insert_1D<Dataset, detail::Key::TagName>)
      // 4. Insertion from Variable or Variable slice.
      .def("__setitem__", detail::insert<Variable, detail::Key::Tag>)
      .def("__setitem__", detail::insert<Variable, detail::Key::TagName>)
      .def("__setitem__", detail::insert<VariableSlice, detail::Key::Tag>)
      .def("__setitem__", detail::insert<VariableSlice, detail::Key::TagName>)

      // TODO Make sure we have all overloads covered to avoid implicit
      // conversion of DatasetSlice to Dataset.
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def("__iadd__",
           [](Dataset &self, const DatasetSlice &other) {
             return self += other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__isub__",
           [](Dataset &self, const DatasetSlice &other) {
             return self -= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__imul__",
           [](Dataset &self, const DatasetSlice &other) {
             return self *= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__add__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self + other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__sub__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self - other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__mul__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self * other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def("dimensions", [](const Dataset &self) { return self.dimensions(); })
      .def("zip", [](Dataset &self) {
        return zip(self, Access::Key(Data::EventTofs),
                   Access::Key(Data::EventPulseTimes));
      });

  py::implicitly_convertible<DatasetSlice, Dataset>();

  m.def("split",
        py::overload_cast<const Dataset &, const Dim,
                          const std::vector<gsl::index> &>(&split),
        py::call_guard<py::gil_scoped_release>());
  m.def("concatenate",
        py::overload_cast<const Dataset &, const Dataset &, const Dim>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>());
  m.def("rebin", py::overload_cast<const Dataset &, const Variable &>(&rebin),
        py::call_guard<py::gil_scoped_release>());
  m.def(
      "sort",
      py::overload_cast<const Dataset &, const Tag, const std::string &>(&sort),
      py::arg("dataset"), py::arg("tag"), py::arg("name") = "",
      py::call_guard<py::gil_scoped_release>());
  m.def("filter", py::overload_cast<const Dataset &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
  m.def("sum", py::overload_cast<const Dataset &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>());
  m.def("mean", py::overload_cast<const Dataset &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>());
}

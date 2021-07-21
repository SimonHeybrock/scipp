// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/variable/variable.h"

#include "scipp/core/dtype.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable_concept.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

/// Construct from parent with same dtype, unit, and hasVariances but new dims.
///
/// In the case of bucket variables the buffer size is set to zero.
Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_dims(dims), m_strides(dims),
      m_object(parent.data().makeDefaultFromParent(dims.volume())) {}

// TODO there is no size check here!
Variable::Variable(const Dimensions &dims, VariableConceptHandle data)
    : m_dims(dims), m_strides(dims), m_object(std::move(data)) {}

Variable::Variable(const llnl::units::precise_measurement &m)
    : Variable(m.value() * units::Unit(m.units())) {}

namespace {
void check_nested_in_assign(const Variable &lhs, const Variable &rhs) {
  if (!rhs.is_valid() || rhs.dtype() != dtype<Variable>) {
    return;
  }
  // In principle we should also check when the RHS contains DataArrays or
  // Datasets. But those are copied when stored in Variables,
  // so no check needed here.
  for (const auto &nested : rhs.values<Variable>()) {
    if (&lhs == &nested) {
      throw std::invalid_argument("Cannot assign Variable, the right hand side "
                                  "contains a reference to the left hand side. "
                                  "Reference cycles are not allowed.");
    }
    check_nested_in_assign(lhs, nested);
  }
}
} // namespace

Variable &Variable::operator=(const Variable &other) {
  return *this = Variable(other);
}

Variable &Variable::operator=(Variable &&other) {
  check_nested_in_assign(*this, other);
  m_dims = other.m_dims;
  m_strides = other.m_strides;
  m_offset = other.m_offset;
  m_object = std::move(other.m_object);
  m_readonly = other.m_readonly;
  return *this;
}

void Variable::setDataHandle(VariableConceptHandle object) {
  if (object->size() != m_object->size())
    throw except::DimensionError("Cannot replace by model of different size.");
  m_object = object;
}

const Dimensions &Variable::dims() const {
  if (!is_valid())
    throw std::runtime_error("invalid variable");
  return m_dims;
}

DType Variable::dtype() const { return data().dtype(); }

bool Variable::hasVariances() const { return data().hasVariances(); }

void Variable::expectCanSetUnit(const units::Unit &unit) const {
  if (this->unit() != unit && is_slice())
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

const units::Unit &Variable::unit() const { return m_object->unit(); }

void Variable::setUnit(const units::Unit &unit) {
  expectWritable();
  expectCanSetUnit(unit);
  m_object->setUnit(unit);
}

bool Variable::operator==(const Variable &other) const {
  if (is_same(other))
    return true;
  if (!is_valid() || !other.is_valid())
    return is_valid() == other.is_valid();
  // Note: Not comparing strides
  if (unit() != other.unit())
    return false;
  if (dims() != other.dims())
    return false;
  if (dtype() != other.dtype())
    return false;
  if (hasVariances() != other.hasVariances())
    return false;
  if (dims().volume() == 0 && dims() == other.dims())
    return true;
  return dims() == other.dims() && data().equals(*this, other);
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

const VariableConcept &Variable::data() const & { return *m_object; }

VariableConcept &Variable::data() & {
  expectWritable();
  return *m_object;
}

const VariableConceptHandle &Variable::data_handle() const { return m_object; }

scipp::span<const scipp::index> Variable::strides() const {
  return scipp::span<const scipp::index>(&*m_strides.begin(),
                                         &*m_strides.begin() + dims().ndim());
}

scipp::index Variable::offset() const { return m_offset; }

core::ElementArrayViewParams Variable::array_params() const noexcept {
  return {m_offset, dims(), m_strides, {}};
}

Variable Variable::slice(const Slice params) const {
  core::expect::validSlice(dims(), params);
  Variable out(*this);
  if (params == Slice{})
    return out;
  const auto dim = params.dim();
  const auto begin = params.begin();
  const auto end = params.end();
  const auto index = out.m_dims.index(dim);
  out.m_offset += begin * m_strides[index];
  if (end == -1) {
    out.m_strides.erase(index);
    out.m_dims.erase(dim);
  } else
    out.m_dims.resize(dim, end - begin);
  return out;
}

void Variable::validateSlice(const Slice &s, const Variable &data) const {
  core::expect::validSlice(this->dims(), s);
  if (variableFactory().hasVariances(data) !=
      variableFactory().hasVariances(*this)) {
    auto variances_message = [](const auto &variable) {
      return "does" +
             std::string(variableFactory().hasVariances(variable) ? ""
                                                                  : " NOT") +
             " have variances.";
    };
    throw except::VariancesError("Invalid slice operation. Slice " +
                                 variances_message(data) + "Variable " +
                                 variances_message(*this));
  }
  if (variableFactory().elem_unit(data) != variableFactory().elem_unit(*this))
    throw except::UnitError(
        "Invalid slice operation. Slice has unit: " +
        to_string(variableFactory().elem_unit(data)) +
        " Variable has unit: " + to_string(variableFactory().elem_unit(*this)));
  if (variableFactory().elem_dtype(data) != variableFactory().elem_dtype(*this))
    throw except::TypeError("Invalid slice operation. Slice has dtype " +
                            to_string(variableFactory().elem_dtype(data)) +
                            ". Variable has dtype " +
                            to_string(variableFactory().elem_dtype(*this)));
}

Variable &Variable::setSlice(const Slice params, const Variable &data) {
  validateSlice(params, data);
  copy(data, slice(params));
  return *this;
}

Variable Variable::broadcast(const Dimensions &target) const {
  expect::includes(target, dims());
  auto out = target.volume() == dims().volume() ? *this : as_const();
  out.m_dims = target;
  scipp::index i = 0;
  for (const auto &d : target.labels())
    out.m_strides[i++] = dims().contains(d) ? m_strides[dims().index(d)] : 0;
  return out;
}

Variable Variable::fold(const Dim dim, const Dimensions &target) const {
  auto out(*this);
  out.m_dims = core::fold(dims(), dim, target);
  const Strides substrides(target);
  scipp::index i_out = 0;
  for (scipp::index i_in = 0; i_in < dims().ndim(); ++i_in) {
    if (dims().label(i_in) == dim)
      for (scipp::index i_target = 0; i_target < target.ndim(); ++i_target)
        out.m_strides[i_out++] = m_strides[i_in] * substrides[i_target];
    else
      out.m_strides[i_out++] = m_strides[i_in];
  }
  return out;
}

Variable Variable::transpose(const std::vector<Dim> &order) const {
  auto transposed(*this);
  transposed.m_strides = core::transpose(m_strides, dims(), order);
  transposed.m_dims = core::transpose(dims(), order);
  return transposed;
}

void Variable::rename(const Dim from, const Dim to) {
  m_dims.replace_key(from, to);
}

bool Variable::is_valid() const noexcept { return m_object.operator bool(); }

bool Variable::is_slice() const {
  // TODO Is this condition sufficient?
  return m_offset != 0 || m_dims.volume() != data().size();
}

bool Variable::is_readonly() const noexcept { return m_readonly; }

bool Variable::is_same(const Variable &other) const noexcept {
  return std::tie(m_dims, m_strides, m_offset, m_object) ==
         std::tie(other.m_dims, other.m_strides, other.m_offset,
                  other.m_object);
}

void Variable::setVariances(const Variable &v) {
  expectWritable();
  if (is_slice())
    throw except::VariancesError(
        "Cannot add variances via sliced view of Variable.");
  if (v.is_valid()) {
    core::expect::equals(unit(), v.unit());
    core::expect::equals(dims(), v.dims());
  }
  data().setVariances(v);
}

namespace detail {
void throw_keyword_arg_constructor_bad_dtype(const DType dtype) {
  throw except::TypeError("Cannot create the Variable with type " +
                          to_string(dtype) +
                          " with such values and/or variances.");
}

void expect0D(const Dimensions &dims) {
  core::expect::equals(dims, Dimensions());
}

} // namespace detail

Variable Variable::bin_indices() const {
  auto out{*this};
  out.m_object = data().bin_indices();
  return out;
}

Variable Variable::as_const() const {
  Variable out(*this);
  out.m_readonly = true;
  return out;
}

void Variable::expectWritable() const {
  if (m_readonly)
    throw except::VariableError("Read-only flag is set, cannot mutate data.");
}
} // namespace scipp::variable

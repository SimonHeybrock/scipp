// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/core/except.h"
#include "scipp/core/sizes.h"

namespace scipp::core {

namespace {
template <class T> void expectUnique(const T &map, const Dim label) {
  if (map.contains(label))
    throw except::DimensionError("Duplicate dimension.");
}

template <class T> void expectExtendable(const T &map) {
  if (map.size() == T::capacity)
    throw except::DimensionError(
        "Maximum number of allowed dimensions exceeded.");
}

template <class T> std::string to_string(const T &map) {
  std::string repr("[");
  for (const auto &key : map)
    repr += to_string(key) + ":" + std::to_string(map[key]) + ", ";
  repr += "]";
  return repr;
}

template <class T>
void throw_dimension_not_found_error(const T &expected, Dim actual) {
  throw except::DimensionError{"Expected dimension to be in " +
                               to_string(expected) + ", got " +
                               to_string(actual) + '.'};
}

} // namespace

template <class Key, class Value, int16_t Capacity>
bool small_stable_map<Key, Value, Capacity>::operator==(
    const small_stable_map &other) const noexcept {
  if (size() != other.size())
    return false;
  for (const auto &key : *this)
    if (!other.contains(key) || at(key) != other.at(key))
      return false;
  return true;
}

template <class Key, class Value, int16_t Capacity>
bool small_stable_map<Key, Value, Capacity>::operator!=(
    const small_stable_map &other) const noexcept {
  return !operator==(other);
}

template <class Key, class Value, int16_t Capacity>
typename std::array<Key, Capacity>::const_iterator
small_stable_map<Key, Value, Capacity>::find(const Key &key) const {
  return std::find(begin(), end(), key);
}

template <class Key, class Value, int16_t Capacity>
bool small_stable_map<Key, Value, Capacity>::contains(const Key &key) const {
  return find(key) != end();
}

template <class Key, class Value, int16_t Capacity>
scipp::index
small_stable_map<Key, Value, Capacity>::index(const Key &key) const {
  const auto it = find(key);
  if (it == end())
    throw_dimension_not_found_error(*this, key);
  return std::distance(begin(), it);
}

template <class Key, class Value, int16_t Capacity>
const Value &
small_stable_map<Key, Value, Capacity>::operator[](const Key &key) const {
  return at(key);
}

template <class Key, class Value, int16_t Capacity>
const Value &small_stable_map<Key, Value, Capacity>::at(const Key &key) const {
  return m_values[index(key)];
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::assign(const Key &key,
                                                    const Value &value) {
  m_values[index(key)] = value;
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::insert_left(const Key &key,
                                                         const Value &value) {
  expectUnique(*this, key);
  expectExtendable(*this);
  for (scipp::index i = size(); i > 0; --i) {
    m_keys[i] = m_keys[i - 1];
    m_values[i] = m_values[i - 1];
  }
  m_keys[0] = key;
  m_values[0] = value;
  m_size++;
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::insert_right(const Key &key,
                                                          const Value &value) {
  expectUnique(*this, key);
  expectExtendable(*this);
  m_keys[m_size] = key;
  m_values[m_size] = value;
  m_size++;
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::erase(const Key &key) {
  for (scipp::index i = index(key); i < size() - 1; ++i) {
    m_keys[i] = m_keys[i + 1];
    m_values[i] = m_values[i + 1];
  }
  m_size--;
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::clear() noexcept {
  m_size = 0;
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::replace_key(const Key &key,
                                                         const Key &new_key) {
  if (key != new_key)
    expectUnique(*this, new_key);
  m_keys[index(key)] = new_key;
}

template class small_stable_map<Dim, scipp::index, NDIM_MAX>;

void Sizes::set(const Dim dim, const scipp::index size) {
  expect::validDim(dim);
  expect::validExtent(size);
  if (contains(dim) && operator[](dim) != size)
    throw except::DimensionError(
        "Inconsistent size for dim '" + to_string(dim) + "', given " +
        std::to_string(at(dim)) + ", requested " + std::to_string(size));
  if (!contains(dim))
    insert_right(dim, size);
}

void Sizes::resize(const Dim dim, const scipp::index size) {
  expect::validExtent(size);
  assign(dim, size);
}

/// Return true if all dimensions of other contained in *this, with same size.
bool Sizes::includes(const Sizes &sizes) const {
  return std::all_of(sizes.begin(), sizes.end(), [&](const auto &dim) {
    return contains(dim) && at(dim) == sizes[dim];
  });
}

Sizes Sizes::slice(const Slice &params) const {
  core::expect::validSlice(*this, params);
  Sizes sliced(*this);
  if (params == Slice{})
    return sliced;
  if (params.isRange())
    sliced.resize(params.dim(), params.end() - params.begin());
  else
    sliced.erase(params.dim());
  return sliced;
}

Sizes concatenate(const Sizes &a, const Sizes &b, const Dim dim) {
  Sizes out = a.contains(dim) ? a.slice({dim, 0}) : a;
  out.set(dim, (a.contains(dim) ? a[dim] : 1) + (b.contains(dim) ? b[dim] : 1));
  return out;
}

Sizes merge(const Sizes &a, const Sizes &b) {
  auto out(a);
  for (const auto &dim : b)
    out.set(dim, b[dim]);
  return out;
}

bool is_edges(const Sizes &sizes, const Sizes &dataSizes, const Dim dim) {
  if (dim == Dim::Invalid)
    return false;
  for (const auto &d : dataSizes)
    if (d != dim && !(sizes.contains(d) && sizes[d] == dataSizes[d]))
      return false;
  const auto size = dataSizes[dim];
  return size == (sizes.contains(dim) ? sizes[dim] + 1 : 2);
}

std::string to_string(const Sizes &sizes) {
  std::string repr("Sizes[");
  for (const auto &dim : sizes)
    repr += to_string(dim) + ":" + std::to_string(sizes[dim]) + ", ";
  repr += "]";
  return repr;
}

} // namespace scipp::core

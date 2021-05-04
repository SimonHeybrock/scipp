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
} // namespace

template <class Key, class Value, int16_t MaxSize, class Except>
bool small_map<Key, Value, MaxSize, Except>::operator==(
    const small_map &other) const noexcept {
  if (size() != other.size())
    return false;
  for (const auto &key : *this)
    if (!other.contains(key) || at(key) != other.at(key))
      return false;
  return true;
}

template <class Key, class Value, int16_t MaxSize, class Except>
bool small_map<Key, Value, MaxSize, Except>::operator!=(
    const small_map &other) const noexcept {
  return !operator==(other);
}

template <class Key, class Value, int16_t MaxSize, class Except>
typename std::array<Key, MaxSize>::const_iterator
small_map<Key, Value, MaxSize, Except>::find(const Key &key) const {
  return std::find(begin(), end(), key);
}

template <class Key, class Value, int16_t MaxSize, class Except>
bool small_map<Key, Value, MaxSize, Except>::contains(
    const Key &key) const noexcept {
  return std::find(begin(), end(), key) != end();
}

template <class Key, class Value, int16_t MaxSize, class Except>
const Value &
small_map<Key, Value, MaxSize, Except>::operator[](const Key &key) const {
  return at(key);
}

template <class Key, class Value, int16_t MaxSize, class Except>
Value &small_map<Key, Value, MaxSize, Except>::operator[](const Key &key) {
  return at(key);
}

template <class Key, class Value, int16_t MaxSize, class Except>
const Value &small_map<Key, Value, MaxSize, Except>::at(const Key &key) const {
  if (!contains(key))
    throw except::DimensionError("Key not found");
  return m_values[std::distance(begin(), find(key))];
}

template <class Key, class Value, int16_t MaxSize, class Except>
Value &small_map<Key, Value, MaxSize, Except>::at(const Key &key) {
  if (!contains(key))
    throw except::DimensionError("Key not found");
  return m_values[std::distance(begin(), find(key))];
}

template <class Key, class Value, int16_t MaxSize, class Except>
void small_map<Key, Value, MaxSize, Except>::insert_left(const Key &key,
                                                         const Value &value) {
  expectUnique(*this, key);
  if (size() == MaxSize)
    throw except::DimensionError("Exceeding builtin map size");
  for (scipp::index i = size(); i > 0; --i) {
    m_keys[i] = m_keys[i - 1];
    m_values[i] = m_values[i - 1];
  }
  m_keys[0] = key;
  m_values[0] = value;
  m_size++;
}

template <class Key, class Value, int16_t MaxSize, class Except>
void small_map<Key, Value, MaxSize, Except>::insert_right(const Key &key,
                                                          const Value &value) {
  expectUnique(*this, key);
  if (size() == MaxSize)
    throw except::DimensionError("Exceeding builtin map size");
  m_keys[m_size] = key;
  m_values[m_size] = value;
  m_size++;
}

template <class Key, class Value, int16_t MaxSize, class Except>
void small_map<Key, Value, MaxSize, Except>::erase(const Key &key) {
  if (!contains(key))
    throw except::DimensionError("Key not found");
  m_size--;
  for (scipp::index i = std::distance(begin(), find(key)); i < size(); ++i) {
    m_keys[i] = m_keys[i + 1];
    m_values[i] = m_values[i + 1];
  }
}

template <class Key, class Value, int16_t MaxSize, class Except>
void small_map<Key, Value, MaxSize, Except>::clear() {
  m_size = 0;
}

template <class Key, class Value, int16_t MaxSize, class Except>
void small_map<Key, Value, MaxSize, Except>::replace_key(const Key &key,
                                                         const Key &new_key) {
  if (!contains(key))
    throw except::DimensionError("Key not found");
  auto it = std::find(m_keys.begin(), m_keys.end(), key);
  *it = new_key;
}

template class small_map<Dim, scipp::index, NDIM_MAX, int>;

Sizes::Sizes(const std::unordered_map<Dim, scipp::index> &sizes) {
  for (const auto &[dim, size] : sizes)
    insert_right(dim, size);
}

scipp::index &Sizes::at(const Dim dim) {
  scipp::expect::contains(*this, dim);
  return m_sizes.at(dim);
}

void Sizes::set(const Dim dim, const scipp::index size) {
  if (contains(dim) && operator[](dim) != size)
    throw except::DimensionError(
        "Inconsistent size for dim '" + to_string(dim) + "', given " +
        std::to_string(at(dim)) + ", requested " + std::to_string(size));
  insert_right(dim, size);
}

void Sizes::relabel(const Dim from, const Dim to) {
  if (to != Dim::Invalid)
    expectUnique(*this, to);
  replace_key(from, to);
}

bool Sizes::contains(const Sizes &sizes) const {
  for (const auto &dim : sizes)
    if (!contains(dim) || at(dim) != sizes[dim])
      return false;
  return true;
}

Sizes Sizes::slice(const Slice &params) const {
  core::expect::validSlice(*this, params);
  Sizes sliced(*this);
  if (params.isRange())
    sliced.at(params.dim()) = params.end() - params.begin();
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

bool is_edges(const Sizes &sizes, const Dimensions &dims, const Dim dim) {
  if (dim == Dim::Invalid)
    return false;
  for (const auto &d : dims.labels())
    if (d != dim && !(sizes.contains(d) && sizes[d] == dims[d]))
      return false;
  const auto size = dims[dim];
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

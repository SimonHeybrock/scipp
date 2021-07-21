// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>

#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/variable.h"

namespace scipp::variable {

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(string, std::string)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(float64, double)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(float32, float)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(int64, int64_t)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(int32, int32_t)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(bool, bool)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(datetime64, scipp::core::time_point)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(Variable, Variable)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(pair_int64, std::pair<int64_t, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(pair_int32, std::pair<int32_t, int32_t>)

} // namespace scipp::variable

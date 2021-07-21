// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/variable.h"

namespace scipp::variable {

// Variable's elements are views into other variables. Used internally for
// implementing functionality for event data combined with dense data using
// transform.
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_float64, span<const double>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_float32, span<const float>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_float64, span<double>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_float32, span<float>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_int64, span<const int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_int32, span<const int32_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_int64, span<int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_int32, span<int32_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_bool, span<const bool>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_bool, span<bool>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_datetime64,
                                   span<scipp::core::time_point>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_datetime64,
                                   span<const core::time_point>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_string, span<const std::string>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_string, span<std::string>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_vector_3_float64,
                                   span<const Eigen::Vector3d>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_vector_3_float64, span<Eigen::Vector3d>)

} // namespace scipp::variable

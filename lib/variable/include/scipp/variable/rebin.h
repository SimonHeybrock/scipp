// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable rebin(const Variable &var,
                                                   const Dim dim,
                                                   const Variable &oldCoord,
                                                   const Variable &newCoord);

} // namespace scipp::variable

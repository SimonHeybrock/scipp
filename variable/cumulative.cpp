// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/cumulative.h"
#include "scipp/core/element/cumulative.h"
#include "scipp/variable/accumulate.h"
#include "scipp/variable/util.h"

using namespace scipp;

namespace scipp::variable {

Variable cumsum(const Variable &var, const Dim dim, const CumSumMode mode) {
  if (var.dims()[dim] == 0)
    return copy(var);
  Variable cumulative = copy(var.slice({dim, 0}));
  fill_zeros(cumulative);
  Variable out = copy(var);
  if (mode == CumSumMode::Inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan,
                        "cumsum");
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan,
                        "cumsum");
  return out;
}

Variable cumsum(const Variable &var, const CumSumMode mode) {
  Variable cumulative(var, Dimensions{});
  Variable out = copy(var);
  if (mode == CumSumMode::Inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan,
                        "cumsum");
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan,
                        "cumsum");
  return out;
}

Variable cumsum_bins(const Variable &var, const CumSumMode mode) {
  Variable out = copy(var);
  auto cumulative = Variable(variable::variableFactory().elem_dtype(var),
                             var.dims(), var.unit());
  if (mode == CumSumMode::Inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan,
                        "cumsum_bins");
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan,
                        "cumsum_bins");
  return out;
}

} // namespace scipp::variable

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from typing import Optional

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def irreducible_mask(masks: _cpp.Masks, dim: str) -> Optional[_cpp.Variable]:
    """Returns the union of all masks with irreducible dimension.

    Irreducible means that a reduction operation must apply these masks since
    they depend on the reduction dimension.

    Parameters
    ----------
    masks:
        Masks of a data array or dataset.
    dim:
        Dimension along which a reduction would be performed.

    Returns
    -------
    :
        Union of irreducible masks or ``None`` if there is no irreducible mask.
    """
    return _call_cpp_func(_cpp.irreducible_mask, masks, dim)


def merge(lhs: _cpp.Dataset, rhs: _cpp.Dataset) -> _cpp.Dataset:
    """Merge two datasets into one.

    Parameters
    ----------
    lhs:
        First dataset.
    rhs:
        Second dataset.

    Returns
    -------
    :
        A new dataset that contains the union of all data items,
        coords, masks and attributes.

    Raises
    ------
    scipp.DatasetError
        If there are conflicting items with different content.
    """
    return _call_cpp_func(_cpp.merge, lhs, rhs)

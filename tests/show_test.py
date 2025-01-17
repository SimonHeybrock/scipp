# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import pytest


def test_large_variable():
    for n in [10, 100, 1000, 10000]:
        var = sc.zeros(dims=['x', 'y'], shape=(n, n))
    assert len(sc.make_svg(var)) < 100000


def test_too_many_variable_dimensions():
    var = sc.zeros(dims=['x', 'y', 'z', 'time'], shape=(1, 1, 1, 1))
    with pytest.raises(RuntimeError):
        sc.make_svg(var)


def test_too_many_dataset_dimensions():
    d = sc.Dataset(
        data={
            'xy': sc.zeros(dims=['x', 'y'], shape=(1, 1)),
            'zt': sc.zeros(dims=['z', 'time'], shape=(1, 1))
        })
    with pytest.raises(RuntimeError):
        sc.make_svg(d)

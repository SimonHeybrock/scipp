# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file

import numpy as np
import pytest
import scipp as sc
from .common import assert_export


def test_broadcast_variable():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    assert_export(sc.broadcast, x=x, sizes={'x': 6, 'y': 3})
    assert_export(sc.broadcast, x=x, dims=['x', 'y'], shape=[6, 3])
    assert sc.identical(sc.broadcast(x, sizes={
        'x': 6,
        'y': 3
    }), sc.broadcast(x, dims=['x', 'y'], shape=[6, 3]))


def test_broadcast_data_array():
    N = 6
    d = sc.linspace('x', 2., 10., N)
    x = sc.arange('x', float(N))
    a = sc.arange('x', float(N)) + 3.0
    m = x < 3.
    da = sc.DataArray(d, coords={'x': x}, attrs={'a': a}, masks={'m': m})
    expected = sc.DataArray(sc.broadcast(d, sizes={
        'x': 6,
        'y': 3
    }),
                            coords={'x': x},
                            attrs={'a': a},
                            masks={'m': m})
    assert sc.identical(sc.broadcast(da, sizes={'x': 6, 'y': 3}), expected)
    assert sc.identical(sc.broadcast(da, dims=['x', 'y'], shape=[6, 3]), expected)


def test_broadcast_fails_with_bad_inputs():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    with pytest.raises(ValueError):
        _ = sc.broadcast(x, sizes={'x': 6, 'y': 3}, dims=['x', 'y'], shape=[6, 3])
    with pytest.raises(ValueError):
        _ = sc.broadcast(x, sizes={'x': 6, 'y': 3}, dims=['x', 'y'])
    with pytest.raises(ValueError):
        _ = sc.broadcast(x, sizes={'x': 6, 'y': 3}, shape=[6, 3])


def test_concat():
    var = sc.scalar(1.0)
    assert sc.identical(sc.concat([var, var + var, 3 * var], 'x'),
                        sc.array(dims=['x'], values=[1.0, 2.0, 3.0]))


def test_fold():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    assert_export(sc.fold, x=x, dim='x', sizes={'x': 2, 'y': 3})
    assert_export(sc.fold, x=x, dim='x', dims=['x', 'y'], shape=[2, 3])
    assert_export(sc.fold, x=da, dim='x', sizes={'x': 2, 'y': 3})
    assert_export(sc.fold, x=da, dim='x', dims=['x', 'y'], shape=[2, 3])


def test_fold_size_minus_1_variable():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    assert sc.identical(sc.fold(x, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(x, dim='x', sizes={
        'x': 2,
        'y': -1
    }))
    assert sc.identical(sc.fold(x, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(x, dim='x', sizes={
        'x': -1,
        'y': 3
    }))


def test_fold_size_minus_1_data_array():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    assert sc.identical(sc.fold(da, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(da, dim='x', sizes={
        'x': 2,
        'y': -1
    }))
    assert sc.identical(sc.fold(da, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(da, dim='x', sizes={
        'x': -1,
        'y': 3
    }))


def test_fold_raises_two_minus_1():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    with pytest.raises(sc.DimensionError):
        sc.fold(x, dim='x', sizes={'x': -1, 'y': -1})
    with pytest.raises(sc.DimensionError):
        sc.fold(da, dim='x', sizes={'x': -1, 'y': -1})


def test_fold_raises_non_divisible():
    x = sc.array(dims=['x'], values=np.arange(10.0))
    da = sc.DataArray(x)
    with pytest.raises(ValueError):
        sc.fold(x, dim='x', sizes={'x': 3, 'y': -1})
    with pytest.raises(ValueError):
        sc.fold(da, dim='x', sizes={'x': -1, 'y': 3})


def test_flatten():
    x = sc.array(dims=['x', 'y'], values=np.arange(6.0).reshape(2, 3))
    da = sc.DataArray(x)
    assert_export(sc.flatten, x=x, dims=['x', 'y'], to='z')
    assert_export(sc.flatten, x=x, to='z')
    assert_export(sc.flatten, x=da, dims=['x', 'y'], to='z')
    assert_export(sc.flatten, x=da, to='z')


def test_squeeze():
    xy = sc.arange('a', 2).fold('a', {'x': 1, 'y': 2})
    assert sc.identical(sc.squeeze(xy, dim='x'), sc.arange('y', 2))
    assert sc.identical(sc.squeeze(xy, dim=['x']), sc.arange('y', 2))
    assert sc.identical(sc.squeeze(xy), sc.arange('y', 2))

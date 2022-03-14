# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .model import Model


class Model1d:
    """
    Model class for 1 dimensional plots.
    """
    def __init__(self, dict_of_data_arrays):
        self._models = {key: Model(array) for key, array in dict_of_data_arrays.items()}
        # self._data_array = data_array

    def keys(self):
        return self._models.keys()

    def values(self):
        return self._models.values()

    def items(self):
        return self._models.items()

    def update(self, key, **kwargs):
        return self._models[key].update(**kwargs)
        # return {
        #     key: model.update(*args, **kwargs)
        #     for key, model in self._models.items()
        # }
        # # out = self._data_array
        # # for p in data_processors:
        # #     out = p.func(out, p.values)
        # # return out

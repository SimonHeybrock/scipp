# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Dimitar Tasev
from .._scipp import core as sc
from .formatting_html import dataset_repr, variable_repr


def make_html(container):
    if isinstance(container, sc.Variable):
        return variable_repr(container)
    else:
        return dataset_repr(container)


def to_html(container):
    from IPython.display import display, HTML
    display(HTML(make_html(container)))

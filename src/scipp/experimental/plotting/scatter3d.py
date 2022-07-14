# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import config, DataArray
from .tools import fig_to_pngbytes
from .toolbar import Toolbar
from .mesh import Mesh
from .line import Line
from ...utils import name_with_unit
from .view import View
from .points import Points

import ipywidgets as ipw
import matplotlib.pyplot as plt
import numpy as np
from typing import Any, Tuple
import pythreejs as p3


class Scatter3d(View):

    def __init__(self, *nodes):

        super().__init__(*nodes)

        self._new_artist = False

        self._children = {}
        self.outline = None
        self.axticks = None

        width = 600
        height = 400

        self.camera = p3.PerspectiveCamera(position=[10, 0, 0], aspect=width / height)

        # Add red/green/blue axes helper
        self.axes_3d = p3.AxesHelper()

        # p = Points(positions=np.random.random([1000, 3]), pixel_size=0.05).points

        # Create the pythreejs scene
        self.scene = p3.Scene(children=[self.camera, self.axes_3d],
                              background="#f0f0f0")

        # Add camera controller
        # TODO: additional parameters whose default values are Inf need to be specified
        # here to avoid a warning being raised: minAzimuthAngle, maxAzimuthAngle,
        # maxDistance, maxZoom. Note that we change the maxDistance once we know the
        # extents of the box.
        # See https://github.com/jupyter-widgets/pythreejs/issues/366.
        self.controls = p3.OrbitControls(controlling=self.camera,
                                         minAzimuthAngle=-1.0e9,
                                         maxAzimuthAngle=1.0e9,
                                         maxDistance=100.0,
                                         maxZoom=0.01)

        # Render the scene into a widget
        self.renderer = p3.Renderer(camera=self.camera,
                                    scene=self.scene,
                                    controls=[self.controls],
                                    width=width,
                                    height=height)


# camera = p3.PerspectiveCamera(position=[2.0, 0, 0], aspect=view_width/view_height)
# scene = p3.Scene(children=[points, camera], background="#DDDDDD")
# controller = p3.OrbitControls(controlling=camera)
# renderer = p3.Renderer(camera=camera, scene=scene, controls=[controller],
#                     width=view_width, height=view_height)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return the renderer and the colorbar into a widget box.
        """
        self.render()
        return self.renderer
        # ipw.HBox(
        #     [self.toolbar._to_widget(),
        #      ipw.HBox([self.renderer]), self.cbar_image])

    # def _create_point_cloud(self, positions):
    #     """
    #     Make a point cloud using pythreejs
    #     """
    #     self.points_geometry = p3.BufferGeometry(
    #         attributes={
    #             'position':
    #             p3.BufferAttribute(array=positions),
    #             'color':
    #             p3.BufferAttribute(
    #                 array=np.ones([positions.shape[0], 3], dtype='float32'))
    #         })

    #     pixel_ratio = 1.0  # config['plot']['pixel_ratio']
    #     # Note that an additional factor of 2.5 (obtained from trial and error) seems to
    #     # be required to get the sizes right in the scene.
    #     self.points_material = p3.PointsMaterial(vertexColors='VertexColors',
    #                                              size=2.5 * self._pixel_size *
    #                                              pixel_ratio,
    #                                              transparent=True)
    #     return p3.Points(geometry=self.points_geometry, material=self.points_material)

    def notify_view(self, message):
        node_id = message["node_id"]
        new_values = self._graph_nodes[node_id].request_data()
        self._update(new_values=new_values, key=node_id)

    def _update(self, new_values: DataArray, key: str):
        """
        Update image array with new values.
        """
        # if new_values.ndim > 1:
        #     raise ValueError("S can only be used to plot 1-D and 2-D data.")
        if key not in self._children:
            self._new_artist = True
            # points = Points(positions=new_values.coords['position'].values)
            self._children[key] = Points(positions=new_values.coords['position'].values)
            # print("ADDING POINT CLOUD")
            self.scene.add(self._children[key].points)
        else:
            self._children[key].update(new_values=new_values)

        # self.draw()

    def render(self):
        # print("RENDERING")
        for node in self._graph_nodes.values():
            new_values = node.request_data()
            self._update(new_values=new_values, key=node.id)

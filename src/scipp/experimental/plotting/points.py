import pythreejs as p3
import numpy as np
from matplotlib import cm


class Points:

    def __init__(self, data, pixel_size=1):
        """
        Make a point cloud using pythreejs
        """
        positions = data.coords['position'].values
        self.geometry = p3.BufferGeometry(
            attributes={
                'position':
                p3.BufferAttribute(array=positions.astype('float32')),
                'color':
                p3.BufferAttribute(
                    array=np.ones([positions.shape[0], 3], dtype='float32'))
            })

        pixel_ratio = 1.0  # config['plot']['pixel_ratio']
        # Note that an additional factor of 2.5 (obtained from trial and error) seems to
        # be required to get the sizes right in the scene.
        self.material = p3.PointsMaterial(vertexColors='VertexColors',
                                          size=2.5 * pixel_size * pixel_ratio,
                                          transparent=True)
        self.points = p3.Points(geometry=self.geometry, material=self.material)

        self.scalar_map = cm.ScalarMappable(cmap='viridis')
        self.update(new_values=data)

    def update(self, new_values):
        colors = self.scalar_map.to_rgba(new_values.values)[..., :3]
        # self._unit = array.unit

        # if 'mask' in new_values:
        #     # We change the colors of the points in-place where masks are True
        #     masks_inds = np.where(new_values['mask'].values)
        #     masks_colors = self.masks_scalar_map.to_rgba(
        #         array.values[masks_inds])[..., :3]
        #     colors[masks_inds] = masks_colors

        colors = colors.astype('float32')
        self.geometry.attributes["color"].array = colors
        # if "cut" in self.point_clouds:
        #     self.point_clouds["cut"].geometry.attributes["color"].array = colors[
        #         self.cut_surface_indices]
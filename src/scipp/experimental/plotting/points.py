import pythreejs as p3
import numpy as np


class Points:

    def __init__(self, positions, pixel_size=1):
        """
        Make a point cloud using pythreejs
        """
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
        # self.material = p3.PointsMaterial(color='black',
        #                                   size=2.5 * pixel_size * pixel_ratio)
        self.points = p3.Points(geometry=self.geometry, material=self.material)

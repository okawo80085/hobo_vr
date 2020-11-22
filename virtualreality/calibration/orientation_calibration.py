from PyQt5 import QtWidgets
from gltensors.model_instancer import ModelViewer, model_dir
import gltensors.biome_generators as biogen
import os
import numpy as np

if __name__ == '__main__':
    app = QtWidgets.QApplication([])
    window = ModelViewer()

    box = biogen.cloud_layer_gen(200, 200, 100, turbulence=.1).astype(np.bool_)

    visible_plains = biogen.restrict_visible(box, box, show_bounds=True)
    visible_locations = biogen.get_locations(visible_plains)

    window.load_model_file_into_positions(model_dir + os.sep + 'cube.obj',
                                          visible_locations)

    window.set_pose([-36.64364585, -176.81920739,  -79.70142023,
                    -0.29609551923914257, 0.6352982400150031, 0.6521352103614712, 0.28886546544566705])

    window.move(QtWidgets.QDesktopWidget().rect().center() - window.rect().center())
    window.show()

    app.exec_()
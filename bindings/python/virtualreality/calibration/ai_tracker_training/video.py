import pathlib

import cv2
import numpy as np
from PIL import Image

ourpath = pathlib.Path(__file__).parent

menu_img = Image.open(ourpath.joinpath("ai_menu.png").absolute())
menu_np = np.copy(np.asarray(menu_img))
menu_np.setflags(write=True)
red = np.copy(menu_np[:, :, 0])
blue = menu_np[:, :, 2]
menu_np[:, :, 0] = blue
menu_np[:, :, 2] = red

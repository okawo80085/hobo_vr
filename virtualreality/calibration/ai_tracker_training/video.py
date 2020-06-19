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


def overlay_transparent(background, overlay, x=None, y=None):
    # https://stackoverflow.com/a/54058766/782170
    assert overlay.shape[2] == 4, "Overlay must be BGRA"

    background_width = background.shape[1]
    background_height = background.shape[0]

    if (x is not None and x >= background_width) or (y is not None and y >= background_height):
        return background

    h, w = overlay.shape[0], overlay.shape[1]

    if x is None:
        x = int(background_width / 2 - w / 2)
    if y is None:
        y = int(background_height / 2 - h / 2)

    if x < 0:
        w += x
        overlay = overlay[:, -x:]

    if y < 0:
        w += y
        overlay = overlay[:, -y:]

    if x + w > background_width:
        w = background_width - x
        overlay = overlay[:, :w]

    if y + h > background_height:
        h = background_height - y
        overlay = overlay[:h]

    overlay_image = overlay[..., :3]
    mask = overlay[..., 3:] / 255.0

    background[y:y + h, x:x + w] = (1.0 - mask) * background[y:y + h, x:x + w] + mask * overlay_image

    return background


def transform_about_center(arr, scale_multiplier=(1, 1), rotation_degrees=0, translation=(0, 0), skew=(0, 0)):
    center_scale_xform = np.eye(3)
    center_scale_xform[0, 0] = scale_multiplier[1]
    center_scale_xform[1, 1] = scale_multiplier[0]
    center_scale_xform[0:2, -1] = [arr.shape[1] // 2, arr.shape[0] // 2]

    rotation_xform = np.eye(3)

    theta = np.radians(rotation_degrees)
    c, s = np.cos(theta), np.sin(theta)
    R = np.array(((c, -s), (s, c)))
    rotation_xform[0:2, 0:2] = R
    skew = np.radians(skew)
    skew = np.tan(skew)
    rotation_xform[-1, 0:2] = [skew[1] / arr.shape[1], skew[0] / arr.shape[0]]

    translation_skew_xform = np.eye(3)
    translation_skew_xform[0:2, -1] = [(-arr.shape[1] - translation[1]) // 2, (-arr.shape[0] - translation[0]) // 2]

    full_xform = center_scale_xform @ rotation_xform @ translation_skew_xform
    xformd_arr = cv2.warpPerspective(arr, full_xform, tuple(reversed(arr.shape[:2])), flags=0)
    return xformd_arr

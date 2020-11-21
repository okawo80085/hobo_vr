"""
pyvr calibrate.

Usage:
    pyvr calibrate [options]

Options:
   -h, --help
   -c, --camera <camera>        Source of the camera to use for calibration [default: 0]
   -r, --resolution <res>       Input resolution in width and height [default: -1x-1]
   -n, --n_masks <n_masks>      Number of masks to calibrate [default: 1]
   -l, --load_from_file <file>  Load previous calibration settings [default: ranges.pickle]
   -s, --save <file>            Save calibration settings to a file [default: ranges.pickle]
"""

import logging
import pickle
import sys
from copy import copy
from pathlib import Path
from typing import Optional, List

import cv2
from docopt import docopt

from virtualreality import __version__


class ColorRange(object):
    def __init__(
            self,
            color_num,
            hue_center=0,
            hue_range=180,
            sat_center=0,
            sat_range=180,
            val_center=0,
            val_range=180,
    ):
        self.color_num = color_num
        self.hue_center = hue_center
        self.hue_range = hue_range
        self.sat_center = sat_center
        self.sat_range = sat_range
        self.val_center = val_center
        self.val_range = val_range


    def __repr__(self):
        return f'ColorRange({self.color_num}, {self.hue_center}, {self.hue_range}, {self.sat_center}, {self.sat_range}, {self.val_center}, {self.val_range})'



class CalibrationData(object):
    def __init__(
            self,
            width=1,
            height=1,
            auto_exposure=0.25,
            exposure=0,
            saturation=50,
            num_colors=4,
    ):
        self.width = width
        self.height = height
        self.exposure = exposure
        self.saturation = saturation
        self.num_colors = num_colors
        self.color_ranges: List[ColorRange] = []

        color_dist = 180 // num_colors
        for color in range(num_colors):
            self.color_ranges.append(
                ColorRange(color, *[color * color_dist, color_dist] * 3)
            )

    def __repr__(self):
        return f'<{self.__class__.__module__}.{self.__class__.__name__} color_ranges={self.color_ranges} at {hex(id(self))}>'


    @classmethod
    def load_from_file(
            cls, load_file: str = str(Path(__file__).parent) + "ranges.pickle"
    ) -> Optional["CalibrationData"]:
        """Load the calibration data from a file."""
        try:
            with open(load_file, "rb") as file:
                ranges = pickle.load(file)
            return ranges
        except FileNotFoundError as fe:
            logging.warning(f"Could not load calibration file '{load_file}'.")

    def save_to_file(
            self, save_file: str = str(Path(__file__).parent) + "ranges.pickle"
    ) -> None:
        with open(save_file, "wb") as file:
            pickle.dump(self, file)


def list_supported_capture_properties(cap: cv2.VideoCapture):
    """List the properties supported by the capture device."""
    # thanks: https://stackoverflow.com/q/47935846/782170
    supported = list()
    for attr in dir(cv2):
        if attr.startswith("CAP_PROP") and cap.get(getattr(cv2, attr)) != -1:
            supported.append(attr)
    return supported


def get_color_mask(hsv, color_range: ColorRange):
    color_low = [
        color_range.hue_center - color_range.hue_range,
        color_range.sat_center - color_range.sat_range,
        color_range.val_center - color_range.val_range,
    ]

    color_high = [
        color_range.hue_center + color_range.hue_range,
        color_range.sat_center + color_range.sat_range,
        color_range.val_center + color_range.val_range,
    ]

    color_low_neg = copy(color_low)
    color_high_neg = copy(color_high)
    for c in range(3):
        if c == 0:
            c_max = 180
        else:
            c_max = 255
        if color_low_neg[c] < 0:
            color_low_neg[c] = c_max + color_low_neg[c]
            color_high_neg[c] = c_max
            color_low[c] = 0
        elif color_high_neg[c] > c_max:
            color_low_neg[c] = 0
            color_high_neg[c] = color_high_neg[c] - c_max
            color_high[c] = c_max

    mask1 = cv2.inRange(hsv, tuple(color_low), tuple(color_high))
    mask2 = cv2.inRange(hsv, tuple(color_low_neg), tuple(color_high_neg))
    mask = cv2.bitwise_or(mask1, mask2)
    return mask


def _set_default_camera_properties(vs, cam, vs_supported, frame_width, frame_height):
    if "CAP_PROP_FOURCC" not in vs_supported:
        logging.warning(f"Camera {cam} does not support setting video codec.")
    else:
        vs.set(cv2.CAP_PROP_FOURCC, cv2.CAP_OPENCV_MJPEG)

    if "CAP_PROP_AUTO_EXPOSURE" not in vs_supported:
        logging.warning(f"Camera {cam} does not support turning on/off auto exposure.")
    else:
        vs.set(cv2.CAP_PROP_AUTO_EXPOSURE, 0.25)

    if "CAP_PROP_EXPOSURE" not in vs_supported:
        logging.warning(f"Camera {cam} does not support directly setting exposure.")
    else:
        vs.set(cv2.CAP_PROP_EXPOSURE, -7)

    if "CAP_PROP_EXPOSURE" not in vs_supported:
        logging.warning(f"Camera {cam} does not support directly setting exposure.")
    else:
        vs.set(cv2.CAP_PROP_EXPOSURE, -7)

    if "CAP_PROP_FRAME_HEIGHT" not in vs_supported:
        logging.warning(f"Camera {cam} does not support requesting frame height.")
    else:
        vs.set(cv2.CAP_PROP_FRAME_HEIGHT, frame_height)

    if "CAP_PROP_FRAME_WIDTH" not in vs_supported:
        logging.warning(f"Camera {cam} does not support requesting frame width.")
    else:
        vs.set(cv2.CAP_PROP_FRAME_WIDTH, frame_width)


def manual_calibration(
        cam=0,
        num_colors_to_track=4,
        frame_width=-1,
        frame_height=-1,
        load_file="",
        save_file="ranges.pickle",
):
    """Manually calibrate the hsv ranges and camera settings used for blob tracking."""
    vs = cv2.VideoCapture(cam)
    vs.set(cv2.CAP_PROP_EXPOSURE, -7)
    vs_supported = list_supported_capture_properties(vs)

    _set_default_camera_properties(vs, cam, vs_supported, frame_width, frame_height)

    cam_window = f"camera {cam} input"
    cv2.namedWindow(cam_window)
    if "CAP_PROP_AUTO_EXPOSURE" in vs_supported:
        cv2.createTrackbar(
            "auto_exposure",
            cam_window,
            25,
            100,
            lambda x: vs.set(cv2.CAP_PROP_EXPOSURE, x / 100.0),
        )
    if "CAP_PROP_EXPOSURE" in vs_supported:
        cv2.createTrackbar(
            "exposure",
            cam_window,
            50,
            10000,
            lambda x: vs.set(cv2.CAP_PROP_EXPOSURE, x),
        )
    if "CAP_PROP_SATURATION" in vs_supported:
        cv2.createTrackbar(
            "saturation",
            cam_window,
            0,
            100,
            lambda x: vs.set(cv2.CAP_PROP_SATURATION, x),
        )
    else:
        logging.warning(f"Camera {cam} does not support setting saturation.")

    ranges = None
    if load_file:
        ranges = CalibrationData.load_from_file(load_file)
    if ranges is None:
        ranges = CalibrationData(
            width=frame_width, height=frame_height, num_colors=num_colors_to_track
        )

    tracker_window_names = []
    for color in range(num_colors_to_track):
        tracker_window_names.append(f"color {color}")
        cv2.namedWindow(tracker_window_names[color])

        cv2.createTrackbar(
            "hue center",
            tracker_window_names[color],
            ranges.color_ranges[color].hue_center,
            180,
            lambda _: None,
        )
        cv2.createTrackbar(
            "hue range",
            tracker_window_names[color],
            ranges.color_ranges[color].hue_range,
            180,
            lambda _: None,
        )
        cv2.createTrackbar(
            "sat center",
            tracker_window_names[color],
            ranges.color_ranges[color].sat_center,
            255,
            lambda _: None,
        )
        cv2.createTrackbar(
            "sat range",
            tracker_window_names[color],
            ranges.color_ranges[color].sat_range,
            255,
            lambda _: None,
        )
        cv2.createTrackbar(
            "val center",
            tracker_window_names[color],
            ranges.color_ranges[color].val_center,
            255,
            lambda _: None,
        )
        cv2.createTrackbar(
            "val range",
            tracker_window_names[color],
            ranges.color_ranges[color].val_range,
            255,
            lambda _: None,
        )

    while 1:
        ret, frame = vs.read()

        if frame is None:
            break

        blurred = cv2.GaussianBlur(frame, (9, 9), 0)
        hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

        exposure = cv2.getTrackbarPos("exposure", cam_window)
        saturation = cv2.getTrackbarPos("saturation", cam_window)

        ranges.exposure = exposure - 8
        ranges.saturation = saturation

        for color in range(num_colors_to_track):
            hue_center = cv2.getTrackbarPos("hue center", tracker_window_names[color])
            hue_range = cv2.getTrackbarPos("hue range", tracker_window_names[color])
            sat_center = cv2.getTrackbarPos("sat center", tracker_window_names[color])
            sat_range = cv2.getTrackbarPos("sat range", tracker_window_names[color])
            val_center = cv2.getTrackbarPos("val center", tracker_window_names[color])
            val_range = cv2.getTrackbarPos("val range", tracker_window_names[color])

            ranges.color_ranges[color].hue_center = hue_center
            ranges.color_ranges[color].hue_range = hue_range
            ranges.color_ranges[color].sat_center = sat_center
            ranges.color_ranges[color].sat_range = sat_range
            ranges.color_ranges[color].val_center = val_center
            ranges.color_ranges[color].val_range = val_range

            mask = get_color_mask(hsv, ranges.color_ranges[color])

            res = cv2.bitwise_and(hsv, hsv, mask=mask)

            cv2.imshow(tracker_window_names[color]+'_mask', mask)
            cv2.imshow(tracker_window_names[color], res)

        cv2.imshow(cam_window, blurred)

        k = cv2.waitKey(1) & 0xFF

        if k in [ord("q"), 27]:
            break

    for color in range(num_colors_to_track):
        hue_center = cv2.getTrackbarPos("hue center", tracker_window_names[color])
        hue_range = cv2.getTrackbarPos("hue range", tracker_window_names[color])
        sat_center = cv2.getTrackbarPos("sat center", tracker_window_names[color])
        sat_range = cv2.getTrackbarPos("sat range", tracker_window_names[color])
        val_center = cv2.getTrackbarPos("val center", tracker_window_names[color])
        val_range = cv2.getTrackbarPos("val range", tracker_window_names[color])

        print(f"hue_center[{color}]: {hue_center}")
        print(f"hue_range[{color}]: {hue_range}")
        print(f"sat_center[{color}]: {sat_center}")
        print(f"sat_range[{color}]: {sat_range}")
        print(f"val_center[{color}]: {val_center}")
        print(f"val_range[{color}]: {val_range}")

    if save_file:
        ranges.save_to_file(save_file)
        print(f'ranges saved to list in "{save_file}".')
        print(
            "You can use this in the pyvr tracker using the --calibration-file argument."
        )

    vs.release()
    cv2.destroyAllWindows()


def main():
    """Calibrate entry point."""
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if len(argv) < 2 or sys.argv[1] != "calibrate":
        argv = ["calibrate"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    width, height = args["--resolution"].split("x")

    if args["--camera"].isdigit():
        cam = int(args["--camera"])
    else:
        cam = args["--camera"]

    manual_calibration(
        cam=cam,
        num_colors_to_track=int(args["--n_masks"]),
        frame_width=int(width),
        frame_height=int(height),
        load_file=args["--load_from_file"],
        save_file=args["--save"],
    )

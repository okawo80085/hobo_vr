"""
pyvr calibrate.

Usage:
    pyvr calibrate [options]

Options:
   -h, --help
   -c, --camera <camera>    Source of the camera to use for calibration [default: 0]
   -r, --resolution <res>   Input resolution in width and height [default: -1x-1]
   -n, --n_masks <n_masks>  Number of masks to calibrate [default: 1]
   -l, --load <file>        Load previous calibration settings [default: ranges.pickle]
   -s, --save <file>        Save calibration settings to a file [default: ranges.pickle]
"""

import logging
import pickle
import sys
from typing import Optional, List

import cv2
from docopt import docopt

from virtualreality import __version__


def list_supported_capture_properties(cap: cv2.VideoCapture):
    """List the properties supported by the capture device."""
    # thanks: https://stackoverflow.com/q/47935846/782170
    supported = list()
    for attr in dir(cv2):
        if attr.startswith("CAP_PROP") and cap.get(getattr(cv2, attr)) != -1:
            supported.append(attr)
    return supported


def load_calibration(load_file: Optional[str]) -> Optional[List[List[int]]]:
    """Load the calibration data from a file."""
    if load_file:
        try:
            with open(load_file, "rb") as file:
                ranges = pickle.load(file)
            return ranges
        except FileNotFoundError as fe:
            logging.warning(f"Could not load calibration file '{load_file}'.")


def manual_calibration(
    cam=0, num_colors_to_track=4, frame_width=-1, frame_height=-1, load_file="", save_file="ranges.pickle"
):
    """Manually calibrate the hsv ranges and camera settings used for blob tracking."""
    vs = cv2.VideoCapture(cam)
    vs.set(cv2.CAP_PROP_EXPOSURE, -7)
    vs_supported = list_supported_capture_properties(vs)

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

    cam_window = f"camera {cam} input"
    cv2.namedWindow(cam_window)
    if "CAP_PROP_EXPOSURE" in vs_supported:
        cv2.createTrackbar(
            "exposure", cam_window, 0, 16, lambda x: vs.set(cv2.CAP_PROP_EXPOSURE, x - 8),
        )
    if "CAP_PROP_SATURATION" in vs_supported:
        cv2.createTrackbar(
            "saturation", cam_window, 0, 100, lambda x: vs.set(cv2.CAP_PROP_SATURATION, x),
        )
    else:
        logging.warning(f"Camera {cam} does not support setting saturation.")

    ranges = load_calibration(load_file)
    if ranges is None:
        color_dist = 180 // num_colors_to_track
        ranges = [[color * color_dist, color_dist] * 3 for color in range(num_colors_to_track)]

    tracker_window_names = []
    for color in range(num_colors_to_track):
        tracker_window_names.append(f"color {color}")
        cv2.namedWindow(tracker_window_names[color])

        cv2.createTrackbar(
            "hue center", tracker_window_names[color], ranges[color][0], 180, lambda _: None,
        )
        cv2.createTrackbar(
            "hue range", tracker_window_names[color], ranges[color][1], 180, lambda _: None,
        )
        cv2.createTrackbar(
            "sat center", tracker_window_names[color], ranges[color][2], 255, lambda _: None,
        )
        cv2.createTrackbar(
            "sat range", tracker_window_names[color], ranges[color][3], 255, lambda _: None,
        )
        cv2.createTrackbar(
            "val center", tracker_window_names[color], ranges[color][4], 255, lambda _: None,
        )
        cv2.createTrackbar(
            "val range", tracker_window_names[color], ranges[color][5], 255, lambda _: None,
        )

    while 1:
        ret, frame = vs.read()

        if frame is None:
            break

        blurred = cv2.GaussianBlur(frame, (3, 3), 0)
        hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

        for color in range(num_colors_to_track):
            hue_center = cv2.getTrackbarPos("hue center", tracker_window_names[color])
            hue_range = cv2.getTrackbarPos("hue range", tracker_window_names[color])
            sat_center = cv2.getTrackbarPos("sat center", tracker_window_names[color])
            sat_range = cv2.getTrackbarPos("sat range", tracker_window_names[color])
            val_center = cv2.getTrackbarPos("val center", tracker_window_names[color])
            val_range = cv2.getTrackbarPos("val range", tracker_window_names[color])

            ranges[color] = [
                hue_center,
                hue_range,
                sat_center,
                sat_range,
                val_center,
                val_range,
            ]

            color_low = (
                hue_center - hue_range,
                sat_center - sat_range,
                val_center - val_range,
            )
            color_high = (
                hue_center + hue_range,
                sat_center + sat_range,
                val_center + val_range,
            )

            mask = cv2.inRange(hsv, tuple(color_low), tuple(color_high))

            res = cv2.bitwise_and(hsv, hsv, mask=mask)

            cv2.imshow(tracker_window_names[color], res)

        cv2.imshow(cam_window, frame)

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
        with open(save_file, "wb") as file:
            pickle.dump(ranges, file)
            print('ranges saved to list in "ranges.pickle".')

    vs.release()
    cv2.destroyAllWindows()


def main():
    """Calibrate entry point."""
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if sys.argv[1] != "calibrate":
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
        load_file=args["--load"],
        save_file=args["--save"],
    )


if __name__ == "__main__":
    main()

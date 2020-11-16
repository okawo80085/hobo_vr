"""
calculate camera distortion matrix
saves matrix to camera_matrix.pickle

press s to save image from camera, and q to close camera and start calibration

usage:
    pyvr calibrate-cam [options]

Options:
   -c, --camera <camera>                    Source of the camera to use for calibration [default: 0]
   -fov, --field-of-view <degrees>          Field of view. (In degrees.) [default: 78]
   -s, --spacing <mm>                       Spacing between checkerboard corners. (In mm.) [default: 31.35]
   -l, --load <file>                        Load previous settings from a file.
"""
import logging
import os
from docopt import docopt
import sys
import pickle

import cv2
import numpy as np

from virtualreality import __version__

images_location = os.path.join(os.path.dirname(__file__), "calibration_images")
print(images_location)

CHECKERBOARD = (7, 7)


def save_images_to_process(cam=0, fov=78):
    """save images from specified camera to calibration_images directory for processing

    Keyword Arguments:
        cam {int} -- reference to usb camera input (default: {0})
    """
    img_counter = 1
    cap = cv2.VideoCapture(cam)
    while cap.isOpened():
        ret, frame = cap.read()
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        if not ret:
            logging.error("failed to receive image from camera")
            break

        if fov > 180:
            valid_img, corners = cv2.findChessboardCorners(
                gray,
                CHECKERBOARD,
                cv2.CALIB_CB_ADAPTIVE_THRESH
                + cv2.CALIB_CB_NORMALIZE_IMAGE,
            )
        else:
            valid_img, corners = cv2.findChessboardCorners(
                gray,
                CHECKERBOARD,
                cv2.CALIB_CB_ADAPTIVE_THRESH
                + cv2.CALIB_CB_FAST_CHECK
                + cv2.CALIB_CB_NORMALIZE_IMAGE,
            )

        if valid_img:
            corners2 = cv2.cornerSubPix(
                gray,
                corners,
                (11, 11),
                (-1, -1),
                (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001),
            )
            frame = cv2.drawChessboardCorners(frame, CHECKERBOARD, corners2, True)

        cv2.imshow("drawn_corners", frame)

        key = cv2.waitKey(1)
        if key & 0xFF == ord("q") or img_counter >= 100:
            # press q to stop
            cap.release()
            cv2.destroyAllWindows()
            # if img_counter == 1:
            break
        # automatically save the image (because getting valid images is tricky, especially with fisheye)
        if valid_img:
            print(f"saving image {img_counter}")
            cv2.imwrite(
                os.path.join(images_location, f"image{img_counter}.jpg"), gray
            )
            img_counter += 1


def calibrate_camera(cam=0, fov=78.0, spacing=31.35, file=""):
    """calculate camera matrix from precollected images

    Keyword Arguments:
        cam {int} -- reference to usb camera input (default: {0})
    """
    # Checkerboard dimensions that the method looks for
    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

    object_points = []
    image_points = []

    obj_p = np.zeros((1, CHECKERBOARD[0] * CHECKERBOARD[1], 3), np.float32)
    obj_p[0, :, :2] = np.mgrid[0: CHECKERBOARD[0], 0: CHECKERBOARD[1]].T.reshape(
        -1, 2
    )
    obj_p *= spacing

    if not os.path.exists(images_location):
        os.makedirs(images_location)
        save_images_to_process(cam)
    elif len(os.listdir(images_location)) == 0:
        save_images_to_process(cam)

    for image_path in os.listdir(images_location):

        input_path = os.path.join(images_location, image_path)
        frame = cv2.imread(input_path)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        if fov > 180:
            valid_img, corners = cv2.findChessboardCorners(
                gray,
                CHECKERBOARD,
                cv2.CALIB_CB_ADAPTIVE_THRESH
                + cv2.CALIB_CB_NORMALIZE_IMAGE,
            )
        else:
            valid_img, corners = cv2.findChessboardCorners(
                gray,
                CHECKERBOARD,
                cv2.CALIB_CB_ADAPTIVE_THRESH
                + cv2.CALIB_CB_FAST_CHECK
                + cv2.CALIB_CB_NORMALIZE_IMAGE,
            )

        if valid_img:
            object_points.append(obj_p)
            corners2 = cv2.cornerSubPix(
                gray,
                corners,
                (11, 11),
                (-1, -1),
                (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001),
            )
            image_points.append(corners2)
        else:
            logging.warning(f"could not find pattern in {image_path}. Ignoring")

    # calibrate camera with aggregated object_points and image_points from samples
    ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(
        object_points, image_points, (640, 480), None, None
    )

    logging.info("camera matrix: ")
    logging.info(mtx)

    with open("calib_data.p", "wb") as camera_calibration_file:
        pickle.dump((ret, mtx, dist, rvecs, tvecs), camera_calibration_file)
    print(f"Your focal length is {mtx[1, 1]}mm.")
    image_width_in_pixels = 640
    focal_len_px = (image_width_in_pixels * 0.5) / np.tan(fov * 0.5 * np.pi / 180)
    print(f"Your focal length is {focal_len_px} pixels.")


def main():
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if sys.argv[1] != "calibrate-cam":
        argv = ["calibrate-cam"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    cam = int(args["--camera"]) if args["--camera"].isdigit() else args["--camera"]
    spacing = None
    f = args["--file"]
    try:
        spacing = float(args["--spacing"])
    except ValueError:
        print("Checkerboard pattern spacing must be specified as a decimal.")

    fov = float(args["--field-of-view"])

    calibrate_camera(cam, spacing=spacing, fov=fov, file=f)

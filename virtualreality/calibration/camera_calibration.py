"""
calculate camera distortion matrix
saves matrix to camera_matrix.pickle

press s to save image from camera, and q to close camera and start calibration

usage:
    pyvr calibrate-cam [options]

Options:
   -c, --camera <camera>                    Source of the camera to use for calibration [default: 0]

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

CHECKERBOARD = (6, 9)


def save_images_to_process(cam=0):
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
            cv2.drawChessboardCorners(frame, CHECKERBOARD, corners2, True)

        cv2.imshow("drawn_corners", frame)

        key = cv2.waitKey(1)
        if key & 0xFF == ord("q") or img_counter >= 20:
            # press q to stop
            cap.release()
            cv2.destroyAllWindows()
            if img_counter == 1:
                exit
            break
        elif key & 0xFF == ord("s"):
            # press s to save image

            if valid_img:
                print(f"saving image {img_counter}")
                cv2.imwrite(
                    os.path.join(images_location, f"image{img_counter}.jpg"), gray
                )
                img_counter += 1
            else:
                print("no checkerboard found. Not Saving.")


def calibrate_camera(cam=0):
    """calculate camera matrix from precollected images

    Keyword Arguments:
        cam {int} -- reference to usb camera input (default: {0})
    """
    # Checkerboard dimensions that the method looks for
    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

    object_points = []
    image_points = []

    obj_p = np.zeros((1, CHECKERBOARD[0] * CHECKERBOARD[1], 3), np.float32)
    obj_p[0, :, :2] = np.mgrid[0 : CHECKERBOARD[0], 0 : CHECKERBOARD[1]].T.reshape(
        -1, 2
    )

    if not os.path.exists(images_location):
        os.makedirs(images_location)
        save_images_to_process(0)
    elif len(os.listdir(images_location)) == 0:
        save_images_to_process(0)

    for image_path in os.listdir(images_location):

        input_path = os.path.join(images_location, image_path)
        frame = cv2.imread(input_path)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

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
                (11, 13),
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

    camera_calibration_file = open(
        os.path.join(os.path.dirname(__file__), "camera_matrix.pickle"), "wb"
    )
    pickle.dump(mtx, camera_calibration_file)
    camera_calibration_file.close()


def main():
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if sys.argv[1] != "calibrate-cam":
        argv = ["calibrate-cam"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    cam = int(args["--camera"]) if args["--camera"].isdigit() else args["--camera"]

    calibrate_camera(0)


if __name__ == "__main__":
    main()

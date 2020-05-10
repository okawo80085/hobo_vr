"""Manual calibration"""

import logging

import cv2

import pickle


def list_supported_capture_properties(cap: cv2.VideoCapture):
    """ List the properties supported by the capture device."""
    # thanks: https://stackoverflow.com/q/47935846/782170
    supported = list()
    for attr in dir(cv2):
        if attr.startswith("CAP_PROP") and cap.get(getattr(cv2, attr)) != -1:
            print(attr)
            supported.append(attr)
    return supported


def manual_calibration(cam=0, num_colors_to_track=4, frame_width=-1, frame_height=-1, load_file=""):
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

    if load_file:
        ranges = pickle.load(open(load_file, "rb"))
    else:
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

    pickle.dump(ranges, open("ranges.pickle", "wb"))
    print('ranges saved to list in "ranges.pickle".')

    vs.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    manual_calibration()

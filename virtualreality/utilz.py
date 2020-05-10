"""Various VR utilities."""

import queue
import threading
import time
from asyncio.streams import StreamReader
from typing import Sequence, Dict

import cv2
import numpy as np
import serial.threaded
from pykalman import KalmanFilter


def format_str_for_write(input_str: str) -> bytes:
    """Format a string for writing to SteamVR's stream."""
    if len(input_str) < 1:
        return "".encode("utf-8")

    if input_str[-1] != "\n":
        return (input_str + "\n").encode("utf-8")

    return input_str.encode("utf-8")


async def read(reader: StreamReader, read_len: int = 20) -> str:
    """Read one line from reader asynchronously."""
    data = []
    temp = " "
    while temp[-1] != "\n" and temp != "":
        temp = await reader.read(read_len)
        temp = temp.decode("utf-8")
        data.append(temp)
        time.sleep(0)  # allows thread switching

    return "".join(data)


def rotate_z(points: Dict[str, Dict[str, float]], angle: float):
    """Rotate a set of points around the z axis."""
    sin = np.sin(angle)
    cos = np.cos(angle)

    for key, p in points.items():
        points[key]["x"] = p["x"] * cos - p["y"] * sin
        points[key]["y"] = p["y"] * cos + p["x"] * sin


def rotate_x(points: Dict[str, Dict[str, float]], angle: float):
    """Rotate a set of points around the x axis."""
    sin = np.sin(angle)
    cos = np.cos(angle)

    for key, p in points.items():
        points[key]["y"] = p["y"] * cos - p["z"] * sin
        points[key]["z"] = p["z"] * cos + p["y"] * sin


def rotate_y(points: Dict[str, Dict[str, float]], angle: float):
    """Rotate a set of points around the y axis."""
    sin = np.sin(angle)
    cos = np.cos(angle)

    for key, p in points.items():
        points[key]["x"] = p["x"] * cos - p["z"] * sin
        points[key]["z"] = p["z"] * cos + p["x"] * sin


def rotate(points: Dict[str, Dict[str, float]], angles: Sequence[float]) -> None:
    """
    Rotate a set of points around the x, y, then z axes.

    :param points: a point dictionary, such as: `{"marker 1" : {"x": 0, "y": 0, "z": 0}}`
    :param angles: the degrees to rotate on the x, y, and z axes
    """
    if angles[0] != 0:
        rotate_x(points, angles[0])

    if angles[1] != 0:
        rotate_y(points, angles[1])

    if angles[2] != 0:
        rotate_z(points, angles[2])


def translate(points: Dict[str, Dict[str, float]], offsets: Sequence[float]) -> None:
    """
    Translate a set of points along the x, y, then z axes.

    :param points: a point dictionary, such as: `{"marker 1" : {"x": 0, "y": 0, "z": 0}}`
    :param offsets: the distance to translate on the x, y, and z axes
    """
    for key, p in points.items():
        points[key]["x"] += offsets[0]
        points[key]["y"] += offsets[1]
        points[key]["z"] += offsets[2]


def strings_share_characters(str1: str, str2: str) -> bool:
    """Determine if two strings share any characters."""
    for i in str1:
        if i in str2:
            return True

    return False


def get_numbers_from_text(text):
    """Get a list of number from a string of numbers seperated by tabs."""
    try:
        if strings_share_characters(text.lower(), "qwertyuiopsasdfghjklzxcvbnm><*[]{}()") or len(text) == 0:
            return []

        return [float(i) for i in text.split("\t")]

    except Exception as e:
        print(f"get_numbers_from_text: {e} {repr(text)}")

        return []


def has_nan_in_pose(pose):
    """Determine if any numbers in pose are invalid."""
    for key in ["x", "y", "z"]:
        if np.isnan(pose[key]) or np.isinf(pose[key]):
            return True

    return False


class SerialReaderFactory(serial.threaded.LineReader):
    """
    A protocol factory for serial.threaded.ReaderThread.

    self.lastRead should be read only, if you need to modify it make a copy

    usage:
        with serial.threaded.ReaderThread(serial_instance, SerialReaderFactory) as protocol:
            protocol.lastRead # contains the last incoming message from serial
            protocol.write_line(single_line_text) # used to write a single line to serial
    """

    def __init__(self):
        """Create the SerialReaderFactory."""
        super().__init__()
        self._last_read = ""

    @property
    def last_read(self):
        """Get the readonly last read."""
        return self._last_read

    def handle_line(self, data):
        """Store the latest line in last_read."""
        self._last_read = data
        # print (f'cSerialReader: data received: {repr(data)}')

    def connection_lost(self, exc):
        """Notify the user that the connection was lost."""
        print(f"SerialReaderFactory: port closed {repr(exc)}")


class BlobTracker(threading.Thread):
    """
    Tracks color blobs in 3D space.

    all tracking is done in a separate thread, so use this class in context manager

    self.start() - starts tracking thread
    self.close() - stops tracking thread

    self.get_poses() - gets latest tracker poses
    """

    def __init__(
            self, cam_index=0, *, focal_length_px=490, ball_radius_cm=2, offsets=[0, 0, 0], color_masks={},
    ):
        """
        Create a blob tracker.

        example usage:
        >>> tracker = BlobTracker(color_masks={
        ...         'tracker1':{
        ...             'h':(98, 10),
        ...             's':(200, 55),
        ...             'v':(250, 32)
        ...         },
        ... })

        >>> with tracker:
        ...     for _ in range(3):
        ...         poses = tracker.get_poses() # gets latest poses for the trackers
        ...         print (poses)
        ...         time.sleep(1/60) # some delay, doesn't affect the tracking
        {'tracker1': {'x': 0, 'y': 0, 'z': 0}}
        {'tracker1': {'x': 0, 'y': 0, 'z': 0}}
        {'tracker1': {'x': 0, 'y': 0, 'z': 0}}

        :param cam_index: index of the camera that will be used to track the blobs
        :param focal_length_px: focal length in pixels
        :param ball_radius_cm: the radius of the ball to be tracked in cm
        :param offsets: rotational offsets, in radians, applied to the 3D coordinates of the blobs
        :param color_masks: color mask parameters, in opencv hsv color space, for color detection
        """
        super().__init__()
        self._vs = cv2.VideoCapture(cam_index)
        self._vs.set(cv2.CAP_PROP_FOURCC, cv2.CAP_OPENCV_MJPEG)  # high speed capture

        time.sleep(2)

        self.camera_focal_length = focal_length_px
        self.BALL_RADIUS_CM = ball_radius_cm
        self.offsets = offsets

        self.can_track, frame = self._vs.read()

        if not self.can_track:
            self._vs.release()
            self._vs = None

        self.frame_height, self.frame_width, _ = frame.shape if self.can_track else (0, 0, 0)

        self.markerMasks = color_masks

        self.poses = {}
        self.blobs = {}

        self.kalmanTrainSize = 15

        self.kalmanFilterz: Dict[str, KalmanFilter] = {}
        self.kalmanTrainBatch = {}
        self.kalmanWasInited = {}
        self.kalmanStateUpdate = {}

        self.kalmanFilterz2: Dict[str, KalmanFilter] = {}
        self.kalmanTrainBatch2 = {}
        self.kalmanWasInited2 = {}
        self.kalmanStateUpdate2 = {}
        self.xywForKalman2 = {}

        for i in self.markerMasks:
            self.poses[i] = {"x": 0, "y": 0, "z": 0}
            self.blobs[i] = None

            self.kalmanFilterz[i] = None
            self.kalmanStateUpdate[i] = None
            self.kalmanTrainBatch[i] = []
            self.kalmanWasInited[i] = False

            self.kalmanFilterz2[i] = None
            self.kalmanStateUpdate2[i] = None
            self.kalmanTrainBatch2[i] = []
            self.kalmanWasInited2[i] = False

        # -------------------threading related------------------------

        self.alive = True
        self._lock = threading.Lock()
        self.daemon = False
        self.pose_que = queue.Queue()

    def stop(self):
        """Stop the blob tracking thread."""
        self.alive = False
        self.can_track = False
        self.join(4)

    def run(self):
        """Run the main blob tracking thread."""
        try:
            self.can_track, _ = self._vs.read()

            if not self.can_track:
                raise RuntimeError("video source already expired")

        except Exception as e:
            print(f"failed to start thread, video source expired?: {repr(e)}")
            self.alive = False
            return

        while self.alive and self.can_track:
            try:
                self.find_blobs_in_frame()
                self.solve_blob_poses()
                rotate(self.poses, self.offsets)

                if not self.pose_que.empty():
                    try:
                        self.pose_que.get_nowait()

                    except queue.Empty:
                        pass

                self.pose_que.put(self.poses.copy())

                if not self.can_track:
                    break

            except Exception as e:
                print(f"tracking failed: {repr(e)}")
                break

        self.alive = False
        self.can_track = False

    def get_poses(self) -> Dict[str, Dict[str, float]]:
        """Get the least recent set of poses."""
        if self.can_track and self.alive:
            return self.pose_que.get()

        else:
            raise RuntimeError("tracking loop already finished")

    def _init_kalman(self, key: str) -> None:
        self.kalmanTrainBatch[key] = np.array(self.kalmanTrainBatch[key])

        init_state = [*self.kalmanTrainBatch[key][0]]

        transition_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        observation_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        self.kalmanFilterz[key] = KalmanFilter(
            transition_matrices=transition_matrix,
            observation_matrices=observation_matrix,
            initial_state_mean=init_state,
        )

        print(f'initializing Kalman filter for mask "{key}"...')

        t_start = time.time()
        self.kalmanFilterz[key] = self.kalmanFilterz[key].em(self.kalmanTrainBatch[key], n_iter=5)
        print(f"took: {time.time() - t_start}s")

        f_means, f_covars = self.kalmanFilterz[key].filter(self.kalmanTrainBatch[key])
        self.kalmanStateUpdate[key] = {"x_now": f_means[-1, :], "p_now": f_covars[-1, :]}

        self.kalmanWasInited[key] = True

    def _init_kalman_2(self, key):
        self.kalmanTrainBatch2[key] = np.array(self.kalmanTrainBatch2[key])

        init_state = [*self.kalmanTrainBatch2[key][0]]

        transition_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        observation_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        self.kalmanFilterz2[key] = KalmanFilter(
            transition_matrices=transition_matrix,
            observation_matrices=observation_matrix,
            initial_state_mean=init_state,
        )

        print(f'initializing second Kalman filter for mask "{key}"...')

        t_start = time.time()
        self.kalmanFilterz2[key] = self.kalmanFilterz2[key].em(self.kalmanTrainBatch2[key], n_iter=5)
        print(f"took: {time.time() - t_start}s")

        f_means, f_covars = self.kalmanFilterz2[key].filter(self.kalmanTrainBatch2[key])
        self.kalmanStateUpdate2[key] = {"x_now": f_means[-1, :], "p_now": f_covars[-1, :]}

        self.kalmanWasInited2[key] = True

    def _applyKalman(self, key):
        obz = np.array([self.poses[key]["x"], self.poses[key]["y"], self.poses[key]["z"]])
        if self.kalmanFilterz[key] is not None:
            (self.kalmanStateUpdate[key]["x_now"], self.kalmanStateUpdate[key]["p_now"],) = self.kalmanFilterz[
                key
            ].filter_update(
                filtered_state_mean=self.kalmanStateUpdate[key]["x_now"],
                filtered_state_covariance=self.kalmanStateUpdate[key]["p_now"],
                observation=obz,
            )

            (self.poses[key]["x"], self.poses[key]["y"], self.poses[key]["z"],) = self.kalmanStateUpdate[key]["x_now"]

    def _applyKalman2(self, key):
        obz = np.array(self.xywForKalman2[key])
        if self.kalmanFilterz2[key] is not None:
            (self.kalmanStateUpdate2[key]["x_now"], self.kalmanStateUpdate2[key]["p_now"],) = self.kalmanFilterz2[
                key
            ].filter_update(
                filtered_state_mean=self.kalmanStateUpdate2[key]["x_now"],
                filtered_state_covariance=self.kalmanStateUpdate2[key]["p_now"],
                observation=obz,
            )

            self.xywForKalman2[key] = self.kalmanStateUpdate2[key]["x_now"]

    def find_blobs_in_frame(self):
        """Get a frame from the camera and find all the blobs in it."""
        if self.alive:
            self.can_track, frame = self._vs.read()

            for key, mask_range in self.markerMasks.items():
                hc, hr = mask_range["h"]

                sc, sr = mask_range["s"]

                vc, vr = mask_range["v"]

                color_high = [hc + hr, sc + sr, vc + vr]
                color_low = [hc - hr, sc - sr, vc - vr]

                if self.can_track:
                    blurred = cv2.GaussianBlur(frame, (3, 3), 0)

                    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

                    mask = cv2.inRange(hsv, tuple(color_low), tuple(color_high))

                    _, cnts, hr = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

                    if len(cnts) > 0:
                        c = max(cnts, key=cv2.contourArea)
                        self.blobs[key] = c if len(c) >= 5 and cv2.contourArea(c) > 10 else None

    def solve_blob_poses(self):
        """Solve for and set the poses of all blobs visible by the camera."""
        for key, blob in self.blobs.items():
            if blob is not None:
                elip = cv2.fitEllipse(blob)

                x, y = elip[0]
                w, h = elip[1]

                self.xywForKalman2[key] = [x, y, w]

                if len(self.kalmanTrainBatch2[key]) < self.kalmanTrainSize:
                    self.kalmanTrainBatch2[key].append([x, y, w])

                elif len(self.kalmanTrainBatch2[key]) >= self.kalmanTrainSize and not self.kalmanWasInited2[key]:
                    self._init_kalman_2(key)

                else:
                    self._applyKalman2(key)
                    x, y, w = self.xywForKalman2[key]

                f_px = self.camera_focal_length
                x_px = x
                y_px = y
                a_px = w / 2

                x_px -= self.frame_width / 2
                y_px = self.frame_height / 2 - y_px

                l_px = np.sqrt(x_px ** 2 + y_px ** 2)

                k = l_px / f_px

                j = (l_px + a_px) / f_px

                l = (j - k) / (1 + j * k)

                d_cm = self.BALL_RADIUS_CM * np.sqrt(1 + l ** 2) / l

                fl = f_px / l_px

                z_cm = d_cm * fl / np.sqrt(1 + fl ** 2)

                l_cm = z_cm * k

                x_cm = l_cm * x_px / l_px
                y_cm = l_cm * y_px / l_px

                self.poses[key]["x"] = x_cm / 100
                self.poses[key]["y"] = y_cm / 100
                self.poses[key]["z"] = z_cm / 100

                # self._translate()

                if len(self.kalmanTrainBatch[key]) < self.kalmanTrainSize:
                    self.kalmanTrainBatch[key].append(
                        [self.poses[key]["x"], self.poses[key]["y"], self.poses[key]["z"], ]
                    )

                elif len(self.kalmanTrainBatch[key]) >= self.kalmanTrainSize and not self.kalmanWasInited[key]:
                    self._init_kalman(key)

                else:
                    if not has_nan_in_pose(self.poses[key]):
                        self._applyKalman(key)

    def close(self):
        """Clocse the blob tracker thread."""
        with self._lock:
            self.stop()

            if self._vs is not None:
                self._vs.release()
                self._vs = None

    def __enter__(self):
        self.start()
        if not self.alive:
            raise RuntimeError("video source already expired")

        return self

    def __exit__(self, *exc):
        self.close()

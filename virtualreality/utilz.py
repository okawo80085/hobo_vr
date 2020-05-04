import asyncio
import time
import cv2
import numpy as np
import math as m

from pykalman import KalmanFilter
import serial
import serial.threaded

import threading
import queue


def convv(sss):
    if len(sss) < 1:
        return "".encode("utf-8")

    if sss[-1] != "\n":
        return (sss + "\n").encode("utf-8")

    return sss.encode("utf-8")


async def newRead(reader, readLen=20):
    data = []
    temp = " "
    while temp[-1] != "\n" and temp != "":
        temp = await reader.read(readLen)
        temp = temp.decode("utf-8")
        data.append(temp)

    return "".join(data)


def angle2rad(angle):
    return angle * m.pi / 180


def rotateZ(points, angle):
    sin = np.sin(angle)
    cos = np.cos(angle)

    for key, p in points.items():
        points[key]["x"] = p["x"] * cos - p["y"] * sin
        points[key]["y"] = p["y"] * cos + p["x"] * sin


def rotateX(points, angle):
    sin = np.sin(angle)
    cos = np.cos(angle)

    for key, p in points.items():
        points[key]["y"] = p["y"] * cos - p["z"] * sin
        points[key]["z"] = p["z"] * cos + p["y"] * sin


def rotateY(points, angle):
    sin = np.sin(angle)
    cos = np.cos(angle)

    for key, p in points.items():
        points[key]["x"] = p["x"] * cos - p["z"] * sin
        points[key]["z"] = p["z"] * cos + p["x"] * sin


def rotate(points, angles):
    if angles[0] != 0:
        rotateX(points, angles[0])

    if angles[1] != 0:
        rotateY(points, angles[1])

    if angles[2] != 0:
        rotateZ(points, angles[2])


def translate(points, offsets):
    for key, p in points.items():
        points[key]["x"] += offsets[0]
        points[key]["y"] += offsets[1]
        points[key]["z"] += offsets[2]


def hasCharsInString(str1, str2):
    for i in str1:
        if i in str2:
            return True

    return False


def decodeSerial(text):
    try:
        if (
            hasCharsInString(text.lower(), "qwertyuiopsasdfghjklzxcvbnm><*[]{}()")
            or len(text) == 0
        ):
            return []

        return [float(i) for i in text.split("\t")]

    except Exception as e:
        print(f"decodeSerial: {e} {repr(text)}")

        return []


def hasNanInPose(pose):
    for key in ["x", "y", "z"]:
        if np.isnan(pose[key]) or np.isinf(pose[key]):
            return True

    return False


class SerialReaderFactory(serial.threaded.LineReader):
    """
    this is a protocol factory for serial.threaded.ReaderThread

    self.lastRead should be read only, if you need to modify it make a copy

    usage:
        with serial.threaded.ReaderThread(serial_instance, SerialReaderFactory) as protocol:
            protocol.lastRead # contains the last incoming message from serial
            protocol.write_line(single_line_text) # used to write a single line to serial
    """

    def __init__(self):
        self.buffer = bytearray()
        self.transport = None
        self.lastRead = ""

    def connection_made(self, transport):
        super(SerialReaderFactory, self).connection_made(transport)

    def handle_line(self, data):
        self.lastRead = data
        # print (f'cSerialReader: data received: {repr(data)}')

    def connection_lost(self, exc):
        print(f"SerialReaderFactory: port closed {repr(exc)}")


class BlobTracker(threading.Thread):
    """
    tracks color blobs in 3D space

    color_masks - color mask parameters, in opencv hsv color space, for color detection
    offsets - rotational offsets, in radians, applied to the 3D coordinates of the blobs

    all tracking is done in a separate thread, so use this class in context manager

    self.start() - starts tracking thread
    self.close() - stops tracking thread

    self.getPoses() - gets latest tracker poses

    example usage:
        tracker = BlobTracker(color_masks={
                'tracker1':{
                    'h':(98, 10),
                    's':(200, 55),
                    'v':(250, 32)
                },
        })

        with tracker:
            for _ in range(300):
                poses = tracker.getPoses() # gets latest poses for the trackers
                print (poses)

                time.sleep(1/60) # some  delay, doesn't affect the tracking

    """

    def __init__(
        self,
        cam_index=0,
        *,
        focal_length_px=490,
        ball_radius_cm=2,
        offsets=[0, 0, 0],
        color_masks={},
    ):
        super().__init__()
        self._vs = cv2.VideoCapture(cam_index)

        time.sleep(2)

        self.camera_focal_length = focal_length_px
        self.BALL_RADIUS_CM = ball_radius_cm
        self.offsets = offsets

        self.can_track, frame = self._vs.read()

        if not self.can_track:
            self._vs.release()
            self._vs = None

        self.frame_height, self.frame_width, _ = (
            frame.shape if self.can_track else (0, 0, 0)
        )

        self.markerMasks = color_masks

        self.poses = {}
        self.blobs = {}

        self.kalmanTrainSize = 15

        self.kalmanFilterz = {}
        self.kalmanTrainBatch = {}
        self.kalmanWasInited = {}
        self.kalmanStateUpdate = {}

        self.kalmanFilterz2 = {}
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
        self.poseQue = queue.Queue()

    def stop(self):
        self.alive = False
        self.can_track = False
        self.join(4)

    def run(self):
        try:
            self.can_track, _ = self._vs.read()

            if not self.can_track:
                raise RuntimeError("video source already expired")

        except Exception as e:
            print(f"failed to start thread, video source expired?: {repr(r)}")
            self.alive = False
            return

        while self.alive and self.can_track:
            try:
                self.getFrame()
                self.solvePose()
                rotate(self.poses, self.offsets)

                if not self.poseQue.empty():
                    try:
                        self.poseQue.get_nowait()

                    except queue.Empty:
                        pass

                self.poseQue.put(self.poses.copy())

                if not self.can_track:
                    break

            except Exception as e:
                print(f"tracking failed: {repr(e)}")
                break

        self.alive = False
        self.can_track = False

    def getPoses(self):
        if self.can_track and self.alive:
            return self.poseQue.get()

        else:
            raise RuntimeError('tracking loop already finished')

    def _initKalman(self, key):
        self.kalmanTrainBatch[key] = np.array(self.kalmanTrainBatch[key])

        initState = [*self.kalmanTrainBatch[key][0]]

        transition_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        observation_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        self.kalmanFilterz[key] = KalmanFilter(
            transition_matrices=transition_matrix,
            observation_matrices=observation_matrix,
            initial_state_mean=initState,
        )

        print(f'initializing Kalman filter for mask "{key}"...')

        tStart = time.time()
        self.kalmanFilterz[key] = self.kalmanFilterz[key].em(
            self.kalmanTrainBatch[key], n_iter=5
        )
        print(f"took: {time.time() - tStart}s")

        fMeans, fCovars = self.kalmanFilterz[key].filter(self.kalmanTrainBatch[key])
        self.kalmanStateUpdate[key] = {"x_now": fMeans[-1, :], "p_now": fCovars[-1, :]}

        self.kalmanWasInited[key] = True

    def _initKalman2(self, key):
        self.kalmanTrainBatch2[key] = np.array(self.kalmanTrainBatch2[key])

        initState = [*self.kalmanTrainBatch2[key][0]]

        transition_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        observation_matrix = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]

        self.kalmanFilterz2[key] = KalmanFilter(
            transition_matrices=transition_matrix,
            observation_matrices=observation_matrix,
            initial_state_mean=initState,
        )

        print(f'initializing second Kalman filter for mask "{key}"...')

        tStart = time.time()
        self.kalmanFilterz2[key] = self.kalmanFilterz2[key].em(
            self.kalmanTrainBatch2[key], n_iter=5
        )
        print(f"took: {time.time() - tStart}s")

        fMeans, fCovars = self.kalmanFilterz2[key].filter(self.kalmanTrainBatch2[key])
        self.kalmanStateUpdate2[key] = {"x_now": fMeans[-1, :], "p_now": fCovars[-1, :]}

        self.kalmanWasInited2[key] = True

    def _applyKalman(self, key):
        obz = np.array(
            [self.poses[key]["x"], self.poses[key]["y"], self.poses[key]["z"]]
        )
        if self.kalmanFilterz[key] is not None:
            (
                self.kalmanStateUpdate[key]["x_now"],
                self.kalmanStateUpdate[key]["p_now"],
            ) = self.kalmanFilterz[key].filter_update(
                filtered_state_mean=self.kalmanStateUpdate[key]["x_now"],
                filtered_state_covariance=self.kalmanStateUpdate[key]["p_now"],
                observation=obz,
            )

            (
                self.poses[key]["x"],
                self.poses[key]["y"],
                self.poses[key]["z"],
            ) = self.kalmanStateUpdate[key]["x_now"]

    def _applyKalman2(self, key):
        obz = np.array(self.xywForKalman2[key])
        if self.kalmanFilterz2[key] is not None:
            (
                self.kalmanStateUpdate2[key]["x_now"],
                self.kalmanStateUpdate2[key]["p_now"],
            ) = self.kalmanFilterz2[key].filter_update(
                filtered_state_mean=self.kalmanStateUpdate2[key]["x_now"],
                filtered_state_covariance=self.kalmanStateUpdate2[key]["p_now"],
                observation=obz,
            )

            self.xywForKalman2[key] = self.kalmanStateUpdate2[key]["x_now"]

    def getFrame(self):
        if self.alive:
            self.can_track, frame = self._vs.read()

        for key, maskRange in self.markerMasks.items():
            hc, hr = maskRange["h"]

            sc, sr = maskRange["s"]

            vc, vr = maskRange["v"]

            colorHigh = [hc + hr, sc + sr, vc + vr]
            colorLow = [hc - hr, sc - sr, vc - vr]

            if self.can_track:
                blurred = cv2.GaussianBlur(frame, (3, 3), 0)

                hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

                mask = cv2.inRange(hsv, tuple(colorLow), tuple(colorHigh))

                _, cnts, hr = cv2.findContours(
                    mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
                )

                if len(cnts) > 0:
                    c = max(cnts, key=cv2.contourArea)
                    self.blobs[key] = (
                        c if len(c) >= 5 and cv2.contourArea(c) > 10 else None
                    )

    def solvePose(self):
        for key, blob in self.blobs.items():
            if blob is not None:
                elip = cv2.fitEllipse(blob)

                x, y = elip[0]
                w, h = elip[1]

                self.xywForKalman2[key] = [x, y, w]

                if len(self.kalmanTrainBatch2[key]) < self.kalmanTrainSize:
                    self.kalmanTrainBatch2[key].append([x, y, w])

                elif (
                    len(self.kalmanTrainBatch2[key]) >= self.kalmanTrainSize
                    and not self.kalmanWasInited2[key]
                ):
                    self._initKalman2(key)

                else:
                    self._applyKalman2(key)
                    x, y, w = self.xywForKalman2[key]

                f_px = self.camera_focal_length
                X_px = x
                Y_px = y
                A_px = w / 2

                X_px -= self.frame_width / 2
                Y_px = self.frame_height / 2 - Y_px

                L_px = np.sqrt(X_px ** 2 + Y_px ** 2)

                k = L_px / f_px

                j = (L_px + A_px) / f_px

                l = (j - k) / (1 + j * k)

                D_cm = self.BALL_RADIUS_CM * np.sqrt(1 + l ** 2) / l

                fl = f_px / L_px

                Z_cm = D_cm * fl / np.sqrt(1 + fl ** 2)

                L_cm = Z_cm * k

                X_cm = L_cm * X_px / L_px
                Y_cm = L_cm * Y_px / L_px

                self.poses[key]["x"] = X_cm / 100
                self.poses[key]["y"] = Y_cm / 100
                self.poses[key]["z"] = Z_cm / 100

                # self._translate()

                if len(self.kalmanTrainBatch[key]) < self.kalmanTrainSize:
                    self.kalmanTrainBatch[key].append(
                        [
                            self.poses[key]["x"],
                            self.poses[key]["y"],
                            self.poses[key]["z"],
                        ]
                    )

                elif (
                    len(self.kalmanTrainBatch[key]) >= self.kalmanTrainSize
                    and not self.kalmanWasInited[key]
                ):
                    self._initKalman(key)

                else:
                    if not hasNanInPose(self.poses[key]):
                        self._applyKalman(key)

    def close(self):
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

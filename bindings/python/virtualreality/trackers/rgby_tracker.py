"""
pyvr track.

Usage:
    pyvr track [options]

Options:
   -h, --help
   -l, --load_calibration <file>    (in progress) Load color mask calibration settings [default: ranges.pickle]
   -i, --ip_address <ip_address>    IP Address of the server to connect to [default: 127.0.0.1]
   --calibration-data <calib_file>  Source of the camera to use for calibration
   -s, --standalone                 Run the server alongside the tracker.
"""

import asyncio
import math
import sys

import serial
import serial.threaded
import pyrr
from docopt import docopt
from ..util import utilz as u
from ..util.IMU import get_i2c_imu
from ..util.kalman import EulerKalman
import numpy as np
from .. import __version__
from .. import templates
from ..server import server
from ..templates import PoseEuler
from ..calibration.manual_color_mask_calibration import CalibrationData, ColorRange


def horizontal_asymptote(val, max_val, speed=1):
    b = max_val + 1
    out_val = (speed * b * val + b) / (speed * val + b) - 1
    return out_val


class HeadsetPoserPiece(object):
    def __init__(
        self,
        tracker_camera=0,
        imu_factory=get_i2c_imu,
        blob_colors={},
        translation_offsets={},
    ):
        self.tracker_camera = tracker_camera
        self.conn, self.imu = imu_factory()
        self.pose = PoseEuler()
        self.blob_colors = blob_colors
        self.translation_offsets = translation_offsets
        self._last_blob_update = 0
        self._last_imu_update = 0
        self.kalman = EulerKalman()

        assert len(blob_colors.keys()) == len(translation_offsets.keys())

        try:
            if self.blob_colors:
                self.blob_tracker = u.BlobTracker(
                    self.tracker_camera,
                    offsets=[0, 0, 0],
                    color_masks=self.blob_colors,
                )
            else:
                self.blob_tracker = None
        except Exception as e:
            print(f"failed to init headset blob tracker: {repr(e)}")
            self.blob_tracker = None

    def update_blob_xyz(self):
        try:
            if (
                self.blob_colors
                and self.blob_tracker.time_of_last_frame != self._last_blob_update
            ):
                self._last_blob_update = self.blob_tracker.time_of_last_frame
                pose_list = []
                poses = self.blob_tracker.get_poses()
                for color in self.blob_colors.keys():
                    pose_x = poses[color]["x"]
                    pose_y = poses[color]["y"]
                    pose_z = poses[color]["z"]
                    pose_x += self.translation_offsets[color]["x"]
                    pose_y += self.translation_offsets[color]["y"]
                    pose_z += self.translation_offsets[color]["z"]
                    pose_list.append((pose_x, pose_y, pose_z))
                pose_x = sum((p[0] for p in pose_list)) / len(pose_list)
                pose_y = sum((p[1] for p in pose_list)) / len(pose_list)
                pose_z = sum((p[2] for p in pose_list)) / len(pose_list)
                self.kalman.apply_blob((pose_x, pose_y, pose_z))
        except Exception as e:
            print("Could not get blob location:", e)

    def update_imu_info(self):
        if (
            self.imu.protocol is not None
            and self.imu.protocol.time_of_last_data != self._last_imu_update
        ):
            self._last_imu_update = self.imu.protocol.time_of_last_data
            self.kalman.apply_imu(self.imu.protocol)


class Poser(templates.PoserTemplate):
    """A pose estimator."""

    def __init__(
        self, *args, camera=4, width=-1, height=-1, calibration_file=None, **kwargs
    ):
        """Create a pose estimator."""
        super().__init__(*args, **kwargs)

        self.headset = HeadsetPoserPiece(
            0,
            blob_colors={
                "blue": {"h": (98, 10), "s": (200, 55), "v": (250, 32)},
                "green": {"h": (68, 15), "s": (135, 53), "v": (255, 50)},
            },
            translation_offsets={"blue": [0, 0, 0], "green": [0, 0, 0]},
        )

        self.mode = 0
        self.useVelocity = False

        self.camera = camera
        self.width = width
        self.height = height

    @templates.PoserTemplate.register_member_thread(1 / 60)
    async def get_location(self):
        """Get locations from blob trackers."""
        try:
            t1 = u.BlobTracker(
                self.camera,
                offsets=[0.6981317007977318, 0, 0],
                color_masks={
                    "blue": {"h": (98, 10), "s": (200, 55), "v": (250, 32)},
                    "green": {"h": (68, 15), "s": (135, 53), "v": (255, 50)},
                },
            )

        except Exception as e:
            print(f"failed to init location tracker: {repr(e)}")
            return

        with t1:
            while self.coro_keep_alive["get_location"].is_alive:
                try:
                    poses = t1.get_poses()

                    self.temp_pose.x = round(-poses["green"]["x"], 6)
                    self.temp_pose.y = round(poses["green"]["y"] + 1, 6)
                    self.temp_pose.z = round(poses["green"]["z"], 6)

                    self.pose.x = round(-poses["blue"]["x"] - 0.01, 6)
                    self.pose.y = round(poses["blue"]["y"] + 1 - 0.07, 6)
                    self.pose.z = round(poses["blue"]["z"] - 0.03, 6)

                    await asyncio.sleep(
                        self.coro_keep_alive["get_location"].sleep_delay
                    )

                except Exception as e:
                    print("stopping get_location:", e)
                    break

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def serial_listener_2(self):
        """Get controller data from serial."""
        past_velocity = [0, 0, 0]
        velocity_until_reset = 0
        temp_for_offz = {"x": 0, "y": 0, "z": 0}
        while self.coro_keep_alive["serial_listener_2"].is_alive:
            try:
                yaw_offset = 0
                with serial.Serial(
                    self.serialPaths["green"], 115200, timeout=1 / 4
                ) as ser:
                    with serial.threaded.ReaderThread(
                        ser, u.SerialReaderFactory
                    ) as protocol:
                        for _ in range(10):
                            protocol.write_line("nut")
                            await asyncio.sleep(1)

                        while self.coro_keep_alive["serial_listener_2"].is_alive:
                            try:
                                gg = u.get_numbers_from_text(protocol.last_read)

                                if len(gg) > 0:
                                    (
                                        y,
                                        p,
                                        r,
                                        az,
                                        ax,
                                        ay,
                                        trgr,
                                        grp,
                                        util,
                                        sys,
                                        menu,
                                        padClk,
                                        padY,
                                        padX,
                                    ) = gg

                                    if velocity_until_reset < 20:

                                        u.rotate_y(
                                            {"": temp_for_offz},
                                            math.radians(-yaw_offset),
                                        )
                                        past_velocity = [
                                            temp_for_offz["x"],
                                            temp_for_offz["y"],
                                            temp_for_offz["z"],
                                        ]
                                        temp_for_offz["x"] = round(
                                            past_velocity[0]
                                            - ax
                                            * self.coro_keep_alive[
                                                "serial_listener_2"
                                            ].sleep_delay
                                            * 0.005,
                                            4,
                                        )
                                        temp_for_offz["y"] = round(
                                            past_velocity[1]
                                            - ay
                                            * self.coro_keep_alive[
                                                "serial_listener_2"
                                            ].sleep_delay
                                            * 0.005,
                                            4,
                                        )
                                        temp_for_offz["z"] = round(
                                            past_velocity[2]
                                            - az
                                            * self.coro_keep_alive[
                                                "serial_listener_2"
                                            ].sleep_delay
                                            * 0.005,
                                            4,
                                        )
                                        u.rotate_y(
                                            {"": temp_for_offz},
                                            math.radians(yaw_offset),
                                        )

                                        if self.useVelocity:
                                            self.temp_pose.vel_x = temp_for_offz["x"]
                                            self.temp_pose.vel_y = temp_for_offz["y"]
                                            self.temp_pose.vel_z = temp_for_offz["z"]

                                        velocity_until_reset += 1

                                    else:
                                        velocity_until_reset = 0
                                        temp_for_offz = {"x": 0, "y": 0, "z": 0}
                                        self.temp_pose.vel_x = 0
                                        self.temp_pose.vel_y = 0
                                        self.temp_pose.vel_z = 0

                                    yaw = y
                                    eulers = [y + yaw_offset, p, r]
                                    eulers = np.radians(eulers)
                                    x, y, z, w = pyrr.Quaternion.from_eulers(
                                        eulers
                                    ).xyzw

                                    self.temp_pose.r_w = round(w, 4)
                                    self.temp_pose.r_x = round(y, 4)
                                    self.temp_pose.r_y = round(-x, 4)
                                    self.temp_pose.r_z = round(-z, 4)

                                    self.temp_pose.trigger_value = abs(trgr - 1)
                                    self.temp_pose.grip = abs(grp - 1)
                                    self.temp_pose.menu = abs(menu - 1)
                                    self.temp_pose.system = abs(sys - 1)
                                    self.temp_pose.trackpad_click = abs(padClk - 1)

                                    self.temp_pose.trackpad_x = round(
                                        (padX - 524) / 530, 3
                                    ) * (-1)
                                    self.temp_pose.trackpad_y = round(
                                        (padY - 506) / 512, 3
                                    ) * (-1)

                                    temp_mode = abs(util - 1)

                                    self._serialResetYaw = False
                                    if self.temp_pose.trackpad_x > 0.6 and temp_mode:
                                        self.mode = 1

                                    elif self.temp_pose.trackpad_x < -0.6 and temp_mode:
                                        self.mode = 0

                                    elif self.temp_pose.trackpad_y < -0.6 and temp_mode:
                                        self.useVelocity = False

                                    elif self.temp_pose.trackpad_y > 0.6 and temp_mode:
                                        self.useVelocity = True

                                    elif temp_mode:
                                        yaw_offset = 0 - yaw
                                        self._serialResetYaw = True

                                if (
                                    abs(self.temp_pose.trackpad_x) > 0.07
                                    or abs(self.temp_pose.trackpad_y) > 0.07
                                ):
                                    self.temp_pose.trackpad_touch = 1

                                else:
                                    self.temp_pose.trackpad_touch = 0

                                if self.temp_pose.trigger_value > 0.9:
                                    self.temp_pose.trackpad_click = 1

                                else:
                                    self.temp_pose.trigger_click = 0

                                await asyncio.sleep(
                                    self.coro_keep_alive[
                                        "serial_listener_2"
                                    ].sleep_delay
                                )

                            except Exception as e:
                                print(f"{self.serial_listener_2.__name__}: {e}")
                                break

            except Exception as e:
                print(f"serial_listener_2: {e}")

            await asyncio.sleep(1)

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def serial_listener(self):
        """Get orientation data from serial."""
        while self.coro_keep_alive["serial_listener"].is_alive:
            try:
                with serial.Serial(
                    self.serialPaths["blue"], 115200, timeout=1 / 5
                ) as ser2:
                    with serial.threaded.ReaderThread(
                        ser2, u.SerialReaderFactory
                    ) as protocol:
                        yaw_offset = 0
                        for _ in range(10):
                            protocol.write_line("nut")
                            await asyncio.sleep(1)

                        while self.coro_keep_alive["serial_listener"].is_alive:
                            try:
                                gg = u.get_numbers_from_text(protocol.last_read)

                                if len(gg) > 0:
                                    ypr = gg

                                    eulers = [ypr[0] + yaw_offset, ypr[1], ypr[2]]
                                    eulers = np.radians(eulers)
                                    x, y, z, w = pyrr.Quaternion.from_eulers(
                                        eulers
                                    ).xyzw

                                    # self.pose['rW'] = rrr2[0]
                                    # self.pose['rX'] = -rrr2[2]
                                    # self.pose['rY'] = rrr2[3]
                                    # self.pose['rZ'] = -rrr2[1]

                                    self.pose.r_w = round(w, 4)
                                    self.pose.r_x = round(y, 4)
                                    self.pose.r_y = round(-x, 4)
                                    self.pose.r_z = round(-z, 4)

                                    if self._serialResetYaw:
                                        yaw_offset = 0 - ypr[0]

                                await asyncio.sleep(
                                    self.coro_keep_alive["serial_listener"].sleep_delay
                                )

                            except Exception as e:
                                print(f"{self.serial_listener.__name__}: {e}")
                                break

            except Exception as e:
                print(f"serial_listener: {e}")

            await asyncio.sleep(1)


def run_poser_only(addr, calib):
    """Run the poser only. The server must be started in another program."""
    t = Poser(addr=addr, calib=calib)
    asyncio.run(t.main())


def run_poser_and_server(addr, calib):
    """Run the poser and server in one program."""
    t = Poser(addr=addr, calib=calib)
    server.run_til_dead(t)


def main():
    """Run color tracker entry point."""
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if len(argv) < 2 or sys.argv[1] != "track":
        argv = ["track"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    if args["--calibration-data"]:
        calib = CalibrationData.load_from_file(args["--calibration-data"])
    else:
        calib = CalibrationData.load_from_file("rgby_calibration.pickle")

    if args["--standalone"]:
        run_poser_and_server(args["--ip_address"], calib)
    else:
        run_poser_only(args["--ip_address"], calib)

    print(args)

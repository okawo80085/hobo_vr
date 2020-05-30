"""
pyvr track.

Usage:
    pyvr track [options]

Options:
   -h, --help
   -c, --camera <camera>           Source of the camera to use for calibration [default: 0]
   -r, --resolution <res>          (in progress) Input resolution in width and height [default: -1x-1]
   -l, --load_calibration <file>   (in progress) Load color mask calibration settings [default: ranges.pickle]
   -i, --ip_address <ip_address>   IP Address of the server to connect to [default: 127.0.0.1]
   -s, --standalone                Run the server alongside the tracker.
"""

# undocumented reference
# kinda specific to okawo's hardware

import asyncio
import math
import sys
from copy import copy
import numpy as np
import serial
import serial.threaded
import pyrr
from docopt import docopt

from ..util import utilz as u
from .. import __version__
from .. import templates
from ..calibration.manual_color_mask_calibration import CalibrationData
from ..server import server
from ..templates import ControllerState


class Poser(templates.PoserTemplate):
    """A pose estimator."""

    def __init__(self, *args, camera=4, width=-1, height=-1, calibration_file=None, **kwargs):
        """Create a pose estimator."""
        super().__init__(*args, **kwargs)

        self.temp_pose = ControllerState()

        self.serialPaths = {"green": "/dev/ttyUSB1", "blue": "/dev/ttyUSB0"}

        self.mode = 0
        self._serialResetYaw = False
        self.useVelocity = False

        self.camera = camera
        self.width = width
        self.height = height
        self.calibration = CalibrationData.load_from_file(calibration_file)

    @templates.thread_register(1 / 90)
    async def mode_switcher(self):
        """Check to switch between left and right controllers."""
        while self.coro_keep_alive["mode_switcher"][0]:
            try:
                if self.mode == 1:
                    self.pose_controller_l.trackpad_touch = 0
                    self.pose_controller_l.trackpad_x = 0
                    self.pose_controller_l.trackpad_y = 0
                    self.pose_controller_r = copy(self.temp_pose)

                else:
                    self.pose_controller_r.trackpad_touch = 0
                    self.pose_controller_r.trackpad_x = 0
                    self.pose_controller_r.trackpad_y = 0
                    self.pose_controller_l = copy(self.temp_pose)

                await asyncio.sleep(self.coro_keep_alive["mode_switcher"][1])

            except Exception as e:
                print(f"failed mode_switcher: {e}")
                self.coro_keep_alive["mode_switcher"][0] = False
                break

    @templates.thread_register(1 / 60)
    async def get_location(self):
        """Get locations from blob trackers."""
        try:
            offsets = [0.6981317007977318, 0, 0]
            t1 = u.BlobTracker(
                self.camera,
                color_masks={
                    "blue": {"h": (98, 10), "s": (200, 55), "v": (250, 32)},
                    "green": {"h": (68, 15), "s": (135, 53), "v": (255, 50)},
                },
            )

        except Exception as e:
            print(f"failed to init location tracker: {repr(e)}")
            return

        with t1:
            while self.coro_keep_alive["get_location"][0]:
                try:
                    poses = t1.get_poses()

                    u.rotate(poses, offsets)

                    self.temp_pose.x = round(-poses["green"]["x"], 6)
                    self.temp_pose.y = round(poses["green"]["y"] + 1, 6)
                    self.temp_pose.z = round(poses["green"]["z"], 6)

                    self.pose.x = round(-poses["blue"]["x"] - 0.01, 6)
                    self.pose.y = round(poses["blue"]["y"] + 1 - 0.07, 6)
                    self.pose.z = round(poses["blue"]["z"], 6)

                    await asyncio.sleep(self.coro_keep_alive["get_location"][1])

                except Exception as e:
                    print("stopping get_location:", e)
                    break

    @templates.thread_register(1 / 100)
    async def serial_listener_2(self):
        """Get controller data from serial."""
        past_velocity = [0, 0, 0]
        velocity_until_reset = 0
        temp_for_offz = {"x": 0, "y": 0, "z": 0}
        while self.coro_keep_alive["serial_listener_2"][0]:
            try:
                yaw_offset = 0
                with serial.Serial(self.serialPaths["green"], 115200, timeout=1 / 4) as ser:
                    with serial.threaded.ReaderThread(ser, u.SerialReaderFactory) as protocol:
                        for _ in range(10):
                            protocol.write_line("nut")
                            await asyncio.sleep(1)

                        while self.coro_keep_alive["serial_listener_2"][0]:
                            try:
                                gg = u.get_numbers_from_text(protocol.last_read)

                                if len(gg) > 0:
                                    (y, p, r, az, ax, ay, trgr, grp, util, sys, menu, padClk, padY, padX,) = gg

                                    if velocity_until_reset < 20:

                                        u.rotate_y(
                                            {"": temp_for_offz}, math.radians(-yaw_offset),
                                        )
                                        past_velocity = [
                                            temp_for_offz["x"],
                                            temp_for_offz["y"],
                                            temp_for_offz["z"],
                                        ]
                                        temp_for_offz["x"] = round(
                                            past_velocity[0]
                                            - ax * self.coro_keep_alive["serial_listener_2"][1] * 0.005,
                                            4,
                                        )
                                        temp_for_offz["y"] = round(
                                            past_velocity[1]
                                            - ay * self.coro_keep_alive["serial_listener_2"][1] * 0.005,
                                            4,
                                        )
                                        temp_for_offz["z"] = round(
                                            past_velocity[2]
                                            - az * self.coro_keep_alive["serial_listener_2"][1] * 0.005,
                                            4,
                                        )
                                        u.rotate_y(
                                            {"": temp_for_offz}, math.radians(yaw_offset),
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
                                    x, y, z, w = pyrr.Quaternion.from_eulers(eulers).xyzw

                                    self.temp_pose.r_w = round(w, 4)
                                    self.temp_pose.r_x = round(y, 4)
                                    self.temp_pose.r_y = round(-x, 4)
                                    self.temp_pose.r_z = round(-z, 4)

                                    self.temp_pose.trigger_value = abs(trgr - 1)
                                    self.temp_pose.grip = abs(grp - 1)
                                    self.temp_pose.menu = abs(menu - 1)
                                    self.temp_pose.system = abs(sys - 1)
                                    self.temp_pose.trackpad_click = abs(padClk - 1)

                                    self.temp_pose.trackpad_x = round((padX - 524) / 530, 3) * (-1)
                                    self.temp_pose.trackpad_y = round((padY - 506) / 512, 3) * (-1)

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

                                if abs(self.temp_pose.trackpad_x) > 0.07 or abs(self.temp_pose.trackpad_y) > 0.07:
                                    self.temp_pose.trackpad_touch = 1

                                else:
                                    self.temp_pose.trackpad_touch = 0

                                if self.temp_pose.trigger_value > 0.9:
                                    self.temp_pose.trigger_click = 1

                                else:
                                    self.temp_pose.trigger_click = 0

                                await asyncio.sleep(self.coro_keep_alive["serial_listener_2"][1])

                            except Exception as e:
                                print(f"{self.serial_listener_2.__name__}: {e}")
                                break

            except Exception as e:
                print(f"serial_listener_2: {e}")

            await asyncio.sleep(1)

    @templates.thread_register(1 / 100)
    async def serial_listener(self):
        """Get orientation data from serial."""
        while self.coro_keep_alive["serial_listener"][0]:
            try:
                with serial.Serial(self.serialPaths["blue"], 115200, timeout=1 / 5) as ser2:
                    with serial.threaded.ReaderThread(ser2, u.SerialReaderFactory) as protocol:
                        yaw_offset = 0
                        for _ in range(10):
                            protocol.write_line("nut")
                            await asyncio.sleep(1)

                        while self.coro_keep_alive["serial_listener"][0]:
                            try:
                                gg = u.get_numbers_from_text(protocol.last_read)

                                if len(gg) > 0:
                                    ypr = gg

                                    eulers = [ypr[0] + yaw_offset, ypr[1], ypr[2]]
                                    eulers = np.radians(eulers)
                                    x, y, z, w = pyrr.Quaternion.from_eulers(eulers).xyzw

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

                                await asyncio.sleep(self.coro_keep_alive["serial_listener"][1])

                            except Exception as e:
                                print(f"{self.serial_listener.__name__}: {e}")
                                break

            except Exception as e:
                print(f"serial_listener: {e}")

            await asyncio.sleep(1)


def run_poser_only(addr="127.0.0.1", cam=4):
    """Run the poser only. The server must be started in another program."""
    t = Poser(addr=addr, camera=cam)
    asyncio.run(t.main())


def run_poser_and_server(addr="127.0.0.1", cam=4):
    """Run the poser and server in one program."""
    t = Poser(addr=addr, camera=cam)
    server.run_til_dead(t)


def main():
    """Run color tracker entry point."""
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if sys.argv[1] != "track":
        argv = ["track"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    width, height = args["--resolution"].split("x")

    if args["--camera"].isdigit():
        cam = int(args["--camera"])
    else:
        cam = args["--camera"]

    if args["--standalone"]:
        run_poser_and_server(args["--ip_address"], cam)
    else:
        run_poser_only(args["--ip_address"], cam)

    print(args)

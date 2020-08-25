"""
pyvr track.

Usage:
    pyvr track [options]

Options:
   -h, --help
   -c, --camera <camera>                    Source of the camera to use for calibration [default: 0]
   -r, --resolution <res>                   (in progress) Input resolution in width and height [default: -1x-1]
   -l, --load_calibration <file>            (optional) Load color mask calibration settings
   -m, --load_calibration_map <map_file>    (optional) Load color mask calibration map settings
   -i, --ip_address <ip_address>            IP Address of the server to connect to [default: 127.0.0.1]
   -s, --standalone                         Run the server alongside the tracker.
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
from pyrr import Quaternion
from docopt import docopt
import glob

from ..util import utilz as u
from .. import __version__
from .. import templates
from ..calibration.manual_color_mask_calibration import CalibrationData
from ..calibration.manual_color_mask_calibration import colordata_to_blob
from ..calibration.manual_color_mask_calibration import load_mapdata_from_file
from ..server import server
from ..templates import ControllerState

def check_serial_dict(ser_dict, key):
    if key in ser_dict:
        if ser_dict[key] in glob.glob("/dev/tty[A-Za-z]*"):
            return True

    return False

class Poser(templates.PoserTemplate):
    """A pose estimator."""

    def __init__(self, *args, camera=4, width=-1, height=-1, calibration_file=None, calibration_map_file=None, **kwargs):
        """Create a pose estimator."""
        super().__init__(*args, **kwargs)

        self.temp_pose = ControllerState()

        self.serialPaths = {"cont_r": "/dev/ttyUSB2","cont_l": "/dev/ttyUSB1", "hmd": "/dev/ttyUSB0"}

        self.mode = 0
        self._serialResetYaw = False
        self.usePos = True

        self.camera = camera
        self.width = width
        self.height = height

        if calibration_file is not None:
            try:
                if calibration_map_file is not None:
                    self.calibration = colordata_to_blob(CalibrationData.load_from_file(calibration_file), load_mapdata_from_file(calibration_map_file))

                else:
                    raise Exception('no map file provided')

            except Exception as e:
                print (f'calibration file provided, but {repr(e)}, ignoring calibration file..')
                self.calibration = {
                                "blue": {"h": (98, 10), "s": (200, 55), "v": (250, 32)},
                                "green": {"h": (68, 15), "s": (135, 53), "v": (255, 50)},
                                    }

        else:
            self.calibration = {
                            "blue": {"h": (98, 10), "s": (200, 55), "v": (250, 32)},
                            "green": {"h": (68, 15), "s": (135, 53), "v": (255, 50)},
                                }

        print (f'current color masks {self.calibration}')


    @templates.PoserTemplate.register_member_thread(1 / 90)
    async def mode_switcher(self):
        """Check to switch between left and right controllers."""
        while self.coro_keep_alive["mode_switcher"].is_alive:
            try:
                if self.mode == 1:
                    self.pose_controller_l.trackpad_touch = 0
                    self.pose_controller_l.trackpad_x = 0
                    self.pose_controller_l.trackpad_y = 0
                    self.pose_controller_r.trackpad_touch = self.temp_pose.trackpad_touch
                    self.pose_controller_r.trackpad_click = self.temp_pose.trackpad_click
                    self.pose_controller_r.trackpad_x = self.temp_pose.trackpad_x
                    self.pose_controller_r.trackpad_y = self.temp_pose.trackpad_y
                    self.pose_controller_r.trigger_value = self.temp_pose.trigger_value
                    self.pose_controller_r.trigger_click = self.temp_pose.trigger_click
                    self.pose_controller_r.system = self.temp_pose.system
                    self.pose_controller_r.grip = self.temp_pose.grip
                    self.pose_controller_r.menu = self.temp_pose.menu

                else:
                    self.pose_controller_r.trackpad_touch = 0
                    self.pose_controller_r.trackpad_x = 0
                    self.pose_controller_r.trackpad_y = 0
                    self.pose_controller_l.trackpad_touch = self.temp_pose.trackpad_touch
                    self.pose_controller_l.trackpad_click = self.temp_pose.trackpad_click
                    self.pose_controller_l.trackpad_x = self.temp_pose.trackpad_x
                    self.pose_controller_l.trackpad_y = self.temp_pose.trackpad_y
                    self.pose_controller_l.trigger_value = self.temp_pose.trigger_value
                    self.pose_controller_l.trigger_click = self.temp_pose.trigger_click
                    self.pose_controller_l.system = self.temp_pose.system
                    self.pose_controller_l.grip = self.temp_pose.grip
                    self.pose_controller_l.menu = self.temp_pose.menu

                await asyncio.sleep(self.coro_keep_alive["mode_switcher"].sleep_delay)

            except Exception as e:
                print(f"failed mode_switcher: {e}")
                break
        self.coro_keep_alive["mode_switcher"].is_alive = False

    @templates.PoserTemplate.register_member_thread(1 / 65)
    async def get_location(self):
        """Get locations from blob trackers."""
        try:
            offsets = [0.6981317007977318, 0, 0]
            t1 = u.BlobTracker(
                self.camera,
                color_masks=self.calibration,
                focal_length_px=554.2563
            )

        except Exception as e:
            print(f"failed to init get_location: {e}")
            self.coro_keep_alive["get_location"].is_alive = False
            return

        with t1:
            while self.coro_keep_alive["get_location"].is_alive:
                try:
                    poses = t1.get_poses()

                    u.rotate(poses, offsets)

                    if self.usePos:
                        self.pose_controller_l.x = round(-poses[1][0] , 6)
                        self.pose_controller_l.y = round(poses[1][1] , 6)
                        self.pose_controller_l.z = round(poses[1][2] , 6)

                        self.pose_controller_r.x = round(-poses[2][0] , 6)
                        self.pose_controller_r.y = round(poses[2][1] , 6)
                        self.pose_controller_r.z = round(poses[2][2] , 6)

                        self.pose.x = round(-poses[0][0]  - 0.01, 6)
                        self.pose.y = round(poses[0][1]  - 0.07, 6)
                        self.pose.z = round(poses[0][2], 6)

                    await asyncio.sleep(self.coro_keep_alive["get_location"].sleep_delay)

                except Exception as e:
                    print("stopping get_location:", e)
                    break

        self.coro_keep_alive["get_location"].is_alive = False

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def serial_listener2(self):
        """Get controller data from serial."""
        irl_rot_off = Quaternion.from_z_rotation(np.pi/2) # imu on this controller is rotated 90 degrees irl for me

        my_off = Quaternion()

        if not check_serial_dict(self.serialPaths, 'cont_l'):
            print ('failed to init serial_listener2: bad serial dict for key \'cont_l\'')
            self.coro_keep_alive["serial_listener2"].is_alive = False
            return

        with serial.Serial(self.serialPaths["cont_l"], 115200, timeout=1 / 4) as ser:
            with serial.threaded.ReaderThread(ser, u.SerialReaderFactory) as protocol:
                protocol.write_line("nut")
                await asyncio.sleep(5)

                while self.coro_keep_alive["serial_listener2"].is_alive:
                    try:
                        gg = u.get_numbers_from_text(protocol.last_read, ',')

                        if len(gg) > 0:
                            (w, x, y, z, trgr, grp, util, sys, menu, padClk, padY, padX,) = gg

                            my_q = Quaternion([-y, z, -x, w])

                            my_q = Quaternion(my_off * my_q * irl_rot_off).normalised
                            self.pose_controller_l.r_w = round(my_q[3], 5)
                            self.pose_controller_l.r_x = round(my_q[0], 5)
                            self.pose_controller_l.r_y = round(my_q[1], 5)
                            self.pose_controller_l.r_z = round(my_q[2], 5)

                            self.temp_pose.trigger_value = trgr
                            self.temp_pose.grip = grp
                            self.temp_pose.menu = menu
                            self.temp_pose.system = sys
                            self.temp_pose.trackpad_click = padClk

                            self.temp_pose.trackpad_x = round((padX - 428) / 460, 3) * (-1)
                            self.temp_pose.trackpad_y = round((padY - 530) / 540, 3) * (-1)

                            self._serialResetYaw = False
                            if self.temp_pose.trackpad_x > 0.6 and util:
                                self.mode = 1

                            elif self.temp_pose.trackpad_x < -0.6 and util:
                                self.mode = 0

                            elif self.temp_pose.trackpad_y > 0.6 and util:
                                self.usePos = True

                            elif self.temp_pose.trackpad_y < -0.6 and util:
                                self.usePos = False

                            elif util:
                                my_off = Quaternion([0, z, 0, w]).inverse.normalised
                                self._serialResetYaw = True

                        if abs(self.temp_pose.trackpad_x) > 0.09 or abs(self.temp_pose.trackpad_y) > 0.09:
                            self.temp_pose.trackpad_touch = 1

                        else:
                            self.temp_pose.trackpad_touch = 0

                        if self.temp_pose.trigger_value > 0.9:
                            self.temp_pose.trigger_click = 1

                        else:
                            self.temp_pose.trigger_click = 0

                        await asyncio.sleep(self.coro_keep_alive["serial_listener2"].sleep_delay)

                    except Exception as e:
                        print(f"{self.serial_listener2.__name__}: {e}")
                        break
        self.coro_keep_alive["serial_listener2"].is_alive = False

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def serial_listener3(self):
        """Get orientation data from serial."""
        irl_rot_off = Quaternion.from_x_rotation(np.pi/2) # imu on this controller is rotated 90 degrees irl for me

        my_off = Quaternion()

        if not check_serial_dict(self.serialPaths, 'cont_r'):
            print ('failed to init serial_listener3: bad serial dict for key \'cont_r\'')
            self.coro_keep_alive["serial_listener3"].is_alive = False
            return

        with serial.Serial(self.serialPaths["cont_r"], 115200, timeout=1 / 5) as ser2:
            with serial.threaded.ReaderThread(ser2, u.SerialReaderFactory) as protocol:
                protocol.write_line("nut")
                await asyncio.sleep(5)

                while self.coro_keep_alive["serial_listener3"].is_alive:
                    try:
                        gg = u.get_numbers_from_text(protocol.last_read, ',')

                        if len(gg) > 0:
                            w, x, y, z = gg
                            my_q = Quaternion([-y, z, -x, w])

                            if self._serialResetYaw:
                                my_off = Quaternion([0, z, 0, w]).inverse.normalised

                            my_q = Quaternion(my_off * my_q * irl_rot_off).normalised
                            self.pose_controller_r.r_w = round(my_q[3], 5)
                            self.pose_controller_r.r_x = round(my_q[0], 5)
                            self.pose_controller_r.r_y = round(my_q[1], 5)
                            self.pose_controller_r.r_z = round(my_q[2], 5)

                        await asyncio.sleep(self.coro_keep_alive["serial_listener3"].sleep_delay)

                    except Exception as e:
                        print(f"{self.serial_listener.__name__}: {e}")
                        break
        self.coro_keep_alive["serial_listener3"].is_alive = False

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def serial_listener(self):
        """Get orientation data from serial."""
        my_off = Quaternion()

        if not check_serial_dict(self.serialPaths, 'hmd'):
            print ('failed to init serial_listener: bad serial dict for key \'hmd\'')
            self.coro_keep_alive["serial_listener"].is_alive = False
            return


        with serial.Serial(self.serialPaths["hmd"], 115200, timeout=1 / 5) as ser2:
            with serial.threaded.ReaderThread(ser2, u.SerialReaderFactory) as protocol:
                protocol.write_line("nut")
                await asyncio.sleep(5)

                while self.coro_keep_alive["serial_listener"].is_alive:
                    try:
                        gg = u.get_numbers_from_text(protocol.last_read, ',')

                        if len(gg) > 0:
                            w, x, y, z = gg
                            my_q = Quaternion([-y, z, -x, w])

                            if self._serialResetYaw:
                                my_off = Quaternion([0, z, 0, w]).inverse.normalised

                            my_q = Quaternion(my_off * my_q).normalised
                            self.pose.r_w = round(my_q[3], 5)
                            self.pose.r_x = round(my_q[0], 5)
                            self.pose.r_y = round(my_q[1], 5)
                            self.pose.r_z = round(my_q[2], 5)

                        await asyncio.sleep(self.coro_keep_alive["serial_listener"].sleep_delay)

                    except Exception as e:
                        print(f"{self.serial_listener.__name__}: {e}")
                        break
        self.coro_keep_alive["serial_listener"].is_alive = False

def run_poser_only(addr="127.0.0.1", cam=4, colordata=None, mapdata=None):
    """Run the poser only. The server must be started in another program."""
    t = Poser(addr=addr, camera=cam, calibration_file=colordata, calibration_map_file=mapdata)
    asyncio.run(t.main())


def run_poser_and_server(addr="127.0.0.1", cam=4, colordata=None, mapdata=None):
    """Run the poser and server in one program."""
    t = Poser(addr=addr, camera=cam, calibration_file=colordata, calibration_map_file=mapdata)
    server.run_til_dead(t)


def main():
    """Run color tracker entry point."""
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if sys.argv[1] != "track":
        argv = ["track"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    width, height = args["--resolution"].split("x")

    print (args)

    if args["--camera"].isdigit():
        cam = int(args["--camera"])
    else:
        cam = args["--camera"]

    if args["--standalone"]:
        run_poser_and_server(args["--ip_address"], cam, args['--load_calibration'], args['--load_calibration_map'])
    else:
        run_poser_only(args["--ip_address"], cam, args['--load_calibration'], args['--load_calibration_map'])

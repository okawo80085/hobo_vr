# (c) 2021 Simleek
# (c) 2021 Okawo
# This code is licensed under MIT license (see LICENSE for details)

"""
pyvr track.

Usage:
    pyvr track-nrf [options] [-S | --serial2ignore=<val>]...

Options:
   -h, --help
   -c, --camera <camera>                    Source of the camera to use for calibration [default: 0]
   -r, --resolution <res>                   (in progress) Input resolution in width and height [default: -1x-1]
   -l, --load_calibration <file>            (optional) Load color mask calibration settings
   -i, --ip_address <ip_address>            IP Address of the server to connect to [default: 127.0.0.1]
   -s, --standalone                         Run the server alongside the tracker.
   -b, --bash_pre_start <path>              (optional) bash script to be ran before the poser starts
   -S, --serial2ignore <val>                serial paths to ignore in device search
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

import time
import subprocess as sub

from ..util import utilz as u
from .. import __version__
from virtualreality import templates
from ..calibration.manual_color_mask_calibration import CalibrationData, ColorRange

from virtualreality.server import server
from virtualreality.templates import ControllerState
import serial.tools.list_ports
from typing import Optional

import concurrent.futures


def is_port_gud(port):
    try:
        ser = serial.Serial(port)
        return True
    except:
        return False


class Poser(templates.UduPoserTemplate):
    """A pose estimator."""
    # poses[0] - hmd
    # poses[1] - right controller
    # poses[2] - left controller

    _CLI_SETTS = '''hobo_vr poser

Usage: poser [-h | --help] [-q | --quit] [options] [-m | --ctrl_inpt_mode]...

Options:
    -h, --help                  shows this message
    -q, --quit                  exits the poser
    -m, --ctrl_inpt_mode        decides how
    -b, --kill_blob             send a kill loop signal to get_location
    -B, --blob_reboot           toggle get_location reboot on kill
    -s, --kill_serial           send a kill loop to serial threads
    -S, --serial_reboot         toggle reboot on kill for serial threads
    -f, --camera_focal <val>    focal point of the camera in pixels[default: 554.2563]
'''

    def __init__(
            self,
            *args,
            camera=4,
            width=-1,
            height=-1,
            calibration_file=None,
            bad_serials=[],
            **kwargs,
    ):
        """Create a pose estimator."""
        super().__init__(*args, **kwargs)

        self.temp_pose = ControllerState()

        self._serials2ignore = set(bad_serials)

        self.serialPaths = {}
        self.serialsInUse = []

        self._serialResetYaw = False
        self.usePos = True

        self.camera = camera
        self.width = width
        self.height = height

        if calibration_file is not None:
            self.calibration = CalibrationData.load_from_file(calibration_file).color_ranges

        else:
            # full brightness
            '''self.calibration = [
                ColorRange(0, 180, 39, 221, 96, 197, 60),
                ColorRange(1, 126, 32, 192, 67, 209, 79),
                ColorRange(2, 75, 24, 220, 39, 167, 114),
            ]'''
            # indoor lighting
            self.calibration = [
                ColorRange(0, 151, 34, 176, 68, 185, 60),# daylight:75
                ColorRange(2, 126, 32, 192, 67, 209, 114),
                ColorRange(1, 75, 20, 220, 39, 197, 131),
            ]
            print('using default calibration')

        # cli interface related

        # self.toggle
        # self.signal

        self.toggleRetryBlobTracker = False
        self.signalKillBlobTrackerLoop = False

        self.toggleRetrySerials = True
        self.signalKillSerialLoops = False

        self.multiToggleMode = 1

        # old:
        # self.camera_focal_px = 554.2563
        # standard laptop:
        # self.camera_focal_px = 395.1670900912165
        # fisheye. Requires special equation depending on type of fisheye
        self.camera_focal_px = 180.0  # approximate
        print(f"current color masks {self.calibration}")

    # def _find_serial_devices(self):
    #     self.serialPaths['headset'] = self._get_serial_for_device('headset', 3.3)

    #     self.serialPaths['right_controller'] = self._get_serial_for_device('right_controller', 3.3)

    #     self.serialPaths['left_controller'] = self._get_serial_for_device('left_controller', 3.3)

    async def _cli_arg_map(self, pair):
        if pair[0] == '--ctrl_inpt_mode' and pair[1]:
            self.multiToggleMode = int(pair[1])
            print(f'mode set to {self.multiToggleMode}')
            return

        elif pair[0] == '--kill_blob' and pair[1]:
            self.signalKillBlobTrackerLoop = True
            await asyncio.sleep(1 / 50)
            self.signalKillBlobTrackerLoop = False
            print('blob kill loop signal sent')
            return

        elif pair[0] == '--blob_reboot' and pair[1]:
            self.toggleRetryBlobTracker = True if not self.toggleRetryBlobTracker else False
            print(f'blob loop reboot set to {"on" if self.toggleRetryBlobTracker else "off"}')
            return

        elif pair[0] == '--camera_focal' and pair[1]:
            if float(pair[1]) != self.camera_focal_px:
                self.camera_focal_px = float(pair[1])
                print(f'new blob tracker camera focal point: {pair[1]}px')
            return

        elif pair[0] == '--kill_serial' and pair[1]:
            # self.serialPaths = {i:None for i in self.serialPaths.keys()}
            self.serialsInUse = []
            self.signalKillSerialLoops = True
            await asyncio.sleep(1 / 50)
            self.signalKillSerialLoops = False
            print('serial kill loops signal sent')
            return

        elif pair[0] == '--serial_reboot' and pair[1]:
            self.toggleRetrySerials = True if not self.toggleRetrySerials else False
            print(f'serial reboot loops set to {"on" if self.toggleRetrySerials else "off"}')
            return

    # im lazy, so its here now
    def _get_serial_for_device(self, device_name: str, timeout: int = 10) -> Optional[str]:
        available_ports = set([comport.device for comport in serial.tools.list_ports.comports()])
        used_ports = set(iter(self.serialPaths.values())) | self._serials2ignore
        ports_to_check = list(available_ports - used_ports)
        attempts = 10

        t0 = time.time()
        while (time.time() - t0) < timeout:
            for p in ports_to_check:
                if p not in self.serialsInUse:
                    self.serialsInUse.append(p)
                else:
                    break

                try:
                    with serial.Serial(p, 1000000, timeout=1) as ser:
                        l = ser.readline()
                        l = str(l)
                        print(l)
                        if device_name in l:
                            print(f'Connected! {p}')
                            # self.serialsInUse.remove(p)
                            return p
                        else:
                            attempts -= 1
                            buffer_attempts = 5
                            while buffer_attempts>0:
                                ser.write('q'.encode('ASCII'))
                                buffer_attempts-=1

                                l = ser.readline()
                                l = str(l)
                                #print(l)
                                if device_name in l:
                                    # self.serialsInUse.remove(p)
                                    print(f'Connected! {p}')
                                    return p
                        if attempts == 0:
                            continue
                except Exception as e:
                    print(f'_get_serial_for_device: {p} failed, skipping, reason: {e}')

                self.serialsInUse.remove(p)
            time.sleep(1 / 100)

        return None

    @templates.PoserTemplate.register_member_thread(1 / 65)
    async def get_location(self):
        """Get locations from blob trackers."""

        axisScale = np.array([1, 1, 1]) * [-1, 1, 1]

        l_oof = np.array([0, 0, 0.037])
        r_oof = np.array([0, 0, 0.032])

        hmd_oof = np.array([-0.035, -0.03, 0.05])

        while self.coro_keep_alive["get_location"].is_alive:
            try:
                offsets = [0.6981317007977318, 0, 0]
                BlobT = u.BlobTracker(
                    self.camera, color_masks=self.calibration, focal_length_px=self.camera_focal_px
                )

            except Exception as e:
                print(f"failed to init get_location: {e}")
                self.coro_keep_alive["get_location"].is_alive = False
                return

            with BlobT:
                while self.coro_keep_alive["get_location"].is_alive:
                    try:
                        if self.signalKillBlobTrackerLoop:
                            break
                        poses = BlobT.get_poses()

                        u.rotate(poses, offsets)

                        poses *= axisScale

                        # print (poses)

                        if self.usePos:
                            # origin point correction math
                            m33_r = pyrr.matrix33.create_from_quaternion(
                                [self.poses[2].r_x, self.poses[2].r_y, self.poses[2].r_z,
                                 self.poses[2].r_w])
                            m33_l = pyrr.matrix33.create_from_quaternion(
                                [self.poses[1].r_x, self.poses[1].r_y, self.poses[1].r_z,
                                 self.poses[1].r_w])
                            m33_hmd = pyrr.matrix33.create_from_quaternion(
                                [self.poses[0].r_x, self.poses[0].r_y, self.poses[0].r_z, self.poses[0].r_w])

                            r_oof2 = m33_r.dot(r_oof)
                            l_oof2 = m33_l.dot(l_oof)
                            hmd_oof2 = m33_hmd.dot(hmd_oof)

                            poses[1] += l_oof2
                            poses[2] += r_oof2
                            poses[0] += hmd_oof2

                            self.poses[0].x = poses[0][0]
                            self.poses[0].y = poses[0][1]
                            self.poses[0].z = poses[0][2]

                            self.poses[2].x = poses[1][0]
                            self.poses[2].y = poses[1][1]
                            self.poses[2].z = poses[1][2]

                            self.poses[1].x = poses[2][0]
                            self.poses[1].y = poses[2][1]
                            self.poses[1].z = poses[2][2]

                        await asyncio.sleep(
                            self.coro_keep_alive["get_location"].sleep_delay
                        )

                    except Exception as e:
                        print("stopping get_location:", e)
                        break

            if not self.toggleRetryBlobTracker:
                break

            del BlobT

            await asyncio.sleep(1)

        self.coro_keep_alive["get_location"].is_alive = False

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def serial_listener(self):
        """Get orientation data from serial."""
        my_off = Quaternion.from_x_rotation(np.radians(10))

        while self.coro_keep_alive["serial_listener"].is_alive:
            # port = self.serialPaths['headset']
            loop = asyncio.get_running_loop()
            with concurrent.futures.ThreadPoolExecutor() as pool:
                port = await loop.run_in_executor(pool, self._get_serial_for_device, 'all')
                print(port)
            self.serialPaths['all'] = port
            print("serialPaths")

            if port is None:
                print("failed to init serial_listener: No serial ports return 'all' when queried.")
                if self.toggleRetrySerials:
                    await asyncio.sleep(1)

                    continue
                else:
                    self.coro_keep_alive["serial_listener"].is_alive = False
                    return
            print("port not none")

            if not is_port_gud(port) or self.signalKillSerialLoops:
                if self.toggleRetrySerials:
                    await asyncio.sleep(1)
                    print('retrying')
                    continue
                else:
                    return
            print("port gud")

            with serial.Serial(port, 1000000, timeout=10) as ser2:
                print("port opened")
                with serial.threaded.ReaderThread(ser2, u.SerialReaderMultiBinary) as protocol:
                    print("protocol opened")
                    protocol.write_line("nut")
                    await asyncio.sleep(1)

                    while self.coro_keep_alive["serial_listener"].is_alive:
                        try:
                            if self.signalKillSerialLoops:
                                break
                            gg = protocol.last_read
                            if gg is None:
                                continue
                            header = gg[0]
                            data = gg[1:]
                            #print(f'poses: {self.poses}')

                            if header == b'hmd:':
                                #print('hmd')
                                if data is not None and len(data[0]) >= 4:
                                    w, x, y, z, *_ = data[0]
                                    my_q = Quaternion([-y, z, -x, w])

                                    if self._serialResetYaw:
                                        my_off = Quaternion([0, z, 0, w]).inverse.normalised

                                    my_q = Quaternion(my_off * my_q).normalised
                                    self.poses[0].r_w = round(my_q[3], 5)
                                    self.poses[0].r_x = round(my_q[0], 5)
                                    self.poses[0].r_y = round(my_q[1], 5)
                                    self.poses[0].r_z = round(my_q[2], 5)
                                    #print(self.poses[0])
                            elif header == b'lc:':
                                #print('lc')
                                irl_rot_off = Quaternion.from_z_rotation(
                                    np.pi / 2
                                )
                                w, x, y, z, trgr, padX, padY, padClk, sys, menu, grp, util, *_ = data[0]

                                my_q = Quaternion([-y, z, -x, w])

                                my_q = Quaternion(my_off * my_q * irl_rot_off).normalised
                                self.poses[1].r_w = round(my_q[3], 5)
                                self.poses[1].r_x = round(my_q[0], 5)
                                self.poses[1].r_y = round(my_q[1], 5)
                                self.poses[1].r_z = round(my_q[2], 5)

                                self.poses[1].trigger_value = trgr
                                self.poses[1].grip = grp
                                self.poses[1].menu = menu
                                self.poses[1].system = sys
                                self.poses[1].trackpad_click = padClk

                                self.poses[1].trackpad_x = round((padX - 428) / 460, 3) * (
                                    -1
                                )
                                self.poses[1].trackpad_y = round((padY - 530) / 540, 3) * (
                                    -1
                                )

                                self._serialResetYaw = False
                                if self.poses[1].trackpad_y > 0.6 and util:
                                    self.usePos = True

                                elif self.poses[1].trackpad_y < -0.6 and util:
                                    self.usePos = False

                                elif util:
                                    my_off = Quaternion([0, z, 0, w]).inverse.normalised
                                    self._serialResetYaw = True

                                if (
                                        abs(self.poses[1].trackpad_x) > 0.1
                                        or abs(self.poses[1].trackpad_y) > 0.1
                                ):
                                    self.poses[1].trackpad_touch = 1

                                else:
                                    self.poses[1].trackpad_touch = 0

                                if self.poses[1].trigger_value > 0.9:
                                    self.poses[1].trigger_click = 1

                                else:
                                    self.poses[1].trigger_click = 0
                                #print(self.poses[1])
                            elif header == b'rc:':
                                irl_rot_off = Quaternion.from_y_rotation(
                                    np.pi
                                )  # imu on this controller is rotated 90 degrees irl for me
                                irl_rot_off2 = Quaternion.from_y_rotation(
                                    np.pi
                                )
                                w, x, y, z, trgr, padX, padY, padClk, sys, menu, grp, util, *_ = data[0]

                                my_q = Quaternion([-y, z, -x, w])

                                my_q = Quaternion(my_off * irl_rot_off2 * my_q * irl_rot_off).normalised
                                self.poses[2].r_w = round(my_q[3], 5)
                                self.poses[2].r_x = round(my_q[0], 5)
                                self.poses[2].r_y = round(my_q[1], 5)
                                self.poses[2].r_z = round(my_q[2], 5)

                                self.poses[2].trigger_value = trgr
                                self.poses[2].grip = grp
                                self.poses[2].menu = menu
                                self.poses[2].system = sys
                                self.poses[2].trackpad_click = padClk

                                self.poses[2].trackpad_x = round((padX - 428) / 460, 3) * (
                                    -1
                                )
                                self.poses[2].trackpad_y = round((padY - 530) / 540, 3) * (
                                    -1
                                )

                                self._serialResetYaw = False
                                if self.poses[2].trackpad_y > 0.6 and util:
                                    self.usePos = True

                                elif self.poses[2].trackpad_y < -0.6 and util:
                                    self.usePos = False

                                elif util:
                                    my_off = Quaternion([0, z, 0, w]).inverse.normalised
                                    self._serialResetYaw = True

                                if (
                                        abs(self.poses[2].trackpad_x) > 0.1
                                        or abs(self.poses[2].trackpad_y) > 0.1
                                ):
                                    self.poses[2].trackpad_touch = 1

                                else:
                                    self.poses[2].trackpad_touch = 0

                                if self.poses[2].trigger_value > 0.9:
                                    self.poses[2].trigger_click = 1

                                else:
                                    self.poses[2].trigger_click = 0


                            await asyncio.sleep(
                                self.coro_keep_alive["serial_listener"].sleep_delay
                            )

                        except Exception as e:
                            print(f"{self.serial_listener.__name__}: {e}")
                            break

            if not self.toggleRetrySerials:
                break

            await asyncio.sleep(1)

        self.coro_keep_alive["serial_listener"].is_alive = False


def run_poser_only(addr="127.0.0.1", cam=4, colordata=None, serz=[]):
    """Run the poser only. The server must be started in another program."""
    t = Poser('h c c',
              addr=addr, camera=cam, calibration_file=colordata, bad_serials=serz
              )
    asyncio.run(t.main())


def run_poser_and_server(addr="127.0.0.1", cam=4, colordata=None, serz=[]):
    """Run the poser and server in one program."""
    t = Poser('h c c',
              addr=addr, camera=cam, calibration_file=colordata, bad_serials=serz
              )
    server.run_til_dead(t)


def main():
    """Run color tracker entry point."""
    # allow calling from both python -m and from pyvr:
    argv = sys.argv[1:]
    if sys.argv[1] != "track-nrf":
        argv = ["track-nrf"] + argv

    args = docopt(__doc__, version=f"pyvr version {__version__}", argv=argv)

    width, height = args["--resolution"].split("x")

    print(args)

    if args['--bash_pre_start']:
        sub.run(args['--bash_pre_start'], shell=True)

    if args["--camera"].isdigit():
        cam = int(args["--camera"])
    else:
        cam = args["--camera"]

    if args["--standalone"]:
        run_poser_and_server(
            args["--ip_address"],
            cam,
            args["--load_calibration"],
            args["--serial2ignore"]
        )
    else:
        run_poser_only(
            args["--ip_address"],
            cam,
            args["--load_calibration"],
            args["--serial2ignore"]
        )


if __name__ == "__main__":
    main()

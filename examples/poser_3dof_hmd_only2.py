"""
this is a hobo_vr poser, enough said ┐(￣ヘ￣)┌

only works with new hobovr arduino code, ask okawo for details

this poser expects hobovr driver to have this udu setting:
h13

"""

import asyncio
import time
import numpy as np
import pyrr
from pyrr import Quaternion
import serial
import serial.threaded

from virtualreality import templates
from virtualreality.util import utilz as u

SERIAL_PORT = "COM3" # serial port, this can be replaced with a device search function now

SERIAL_BAUD = 115200 # baud rate of the serial port

poser = templates.UduPoserClient("h")

@poser.thread_register(1 / 100)
async def serial_listener():
    try:
        irl_rot_off = Quaternion.from_y_rotation(np.pi)  # supply your own imu offsets here, has to be a Quaternion object
        irl_rot_off2 = Quaternion.from_y_rotation(np.pi)  # supply your own imu offsets here, has to be a Quaternion object

        aa_last = np.zeros((3,))
        aa_last2 = np.zeros((3,))
        vel = np.zeros((3,))
        grav_v = np.array((0, 1, 0))*9.8
        mm = pyrr.matrix33.create_from_quaternion(irl_rot_off2 * irl_rot_off)
        with serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1 / 4) as ser:
            with serial.threaded.ReaderThread(ser, u.SerialReaderBinary(struct_len=19)) as protocol:
                protocol.write_line("nut")
                await asyncio.sleep(1)

                print ('serial_listener: starting tracking thread')

                while poser.coro_keep_alive["serial_listener"].is_alive:
                    gg = protocol.last_read

                    if gg is not None:
                        w, x, y, z, ax, ay, az, gx, gy, gz, *rest = gg
                        my_q = Quaternion([-y, z, -x, w])

                        # print ([gx, gy, gz])
                        aa = np.array([ay, az, ax])
                        ge = np.array([gy, gz, gx])

                        my_q = Quaternion(irl_rot_off2 * my_q * irl_rot_off).normalised

                        mm2 = pyrr.matrix33.create_from_quaternion(my_q)
                        aa = mm.dot(aa)
                        ge = mm.dot(ge) / 250 * np.pi
                        grav_v = mm2.dot(grav_v)

                        if np.linalg.norm(aa-aa_last2) < 0.08:
                            vel = np.zeros((3,))
                        else:
                            vel = ((aa+aa_last+aa_last2))/3

                        aa_last2 = aa_last
                        aa_last = aa

                        poser.poses[0].r_w = my_q[3]
                        poser.poses[0].r_x = my_q[0]
                        poser.poses[0].r_y = my_q[1]
                        poser.poses[0].r_z = my_q[2]
                        poser.poses[0].vel_x = vel[0]
                        poser.poses[0].vel_y = vel[1]
                        poser.poses[0].vel_z = vel[2]
                        poser.poses[0].ang_vel_x = ge[0]
                        poser.poses[0].ang_vel_y = ge[1]
                        poser.poses[0].ang_vel_z = ge[2]

                    await asyncio.sleep(poser.coro_keep_alive["serial_listener"].sleep_delay)

    except Exception as e:
        print (f'serial_listener: failed {e}')

    poser.coro_keep_alive["serial_listener"].is_alive = False

asyncio.run(poser.main())

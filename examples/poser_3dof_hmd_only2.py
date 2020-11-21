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
        irl_rot_off = Quaternion.from_x_rotation(0)  # supply your own imu offsets here, has to be a Quaternion object

        with serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1 / 4) as ser:
            with serial.threaded.ReaderThread(ser, u.SerialReaderBinary(struct_len=19)) as protocol:
                protocol.write_line("nut")
                await asyncio.sleep(1)

                print ('serial_listener: starting tracking thread')

                while poser.coro_keep_alive["serial_listener"].is_alive:
                    gg = protocol.last_read

                    if gg is not None:
                        w, x, y, z, *rest = gg
                        my_q = Quaternion([-y, z, -x, w])

                        print (rest, len(rest))

                        my_q = Quaternion(my_q * irl_rot_off).normalised
                        poser.poses[0].r_w = round(my_q[3], 5)
                        poser.poses[0].r_x = round(my_q[0], 5)
                        poser.poses[0].r_y = round(my_q[1], 5)
                        poser.poses[0].r_z = round(my_q[2], 5)

                    await asyncio.sleep(poser.coro_keep_alive["serial_listener"].sleep_delay)

    except Exception as e:
        print (f'serial_listener: failed {e}')

    poser.coro_keep_alive["serial_listener"].is_alive = False

asyncio.run(poser.main())

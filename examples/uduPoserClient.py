"""
udu poser client example

more info: help(templates.UduPoserTemplate)
"""

import asyncio
import time
import numpy as np
import pyrr

from virtualreality import templates
from virtualreality.server import server

poser = templates.UduPoserClient("h c c")  # devices setup identical to normal posers


@poser.thread_register(1 / 60)
async def example_thread():
    # h = [0 for _ in range(len(poser.poses))]
    h = 0
    while poser.coro_keep_alive["example_thread"].is_alive:
        x, y, z, w = pyrr.Quaternion.from_y_rotation(h)
        poser.poses[0].x = np.sin(h)
        poser.poses[0].z = np.cos(h)

        poser.poses[0].r_x = x
        poser.poses[0].r_y = y
        poser.poses[0].r_z = z
        poser.poses[0].r_w = w
        for i in range(1, len(poser.poses)):
            poser.poses[i].x = np.sin(h / (i + 1))
            poser.poses[i].z = np.cos(h / (i + 1))

            poser.poses[i].r_x = -x
            poser.poses[i].r_y = -y
            poser.poses[i].r_z = -z
            poser.poses[i].r_w = w

        h += 0.01

        await asyncio.sleep(poser.coro_keep_alive["example_thread"].sleep_delay)


@poser.thread_register(1 / 60)
async def example_receive_haptics_thread():
    while poser.coro_keep_alive["example_receive_haptics_thread"].is_alive:
        if poser.last_read:
            print (poser.last_read)
            poser.last_read = b""

        await asyncio.sleep(poser.coro_keep_alive["example_receive_haptics_thread"].sleep_delay)



asyncio.run(poser.main())

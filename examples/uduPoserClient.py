
import asyncio
import time
import numpy as np
import pyrr

from virtualreality import templates
from virtualreality.server import server

poser = templates.UduPoserClient('h c c t t t')

@poser.thread_register(1/60)
async def example_thread():
    # h = [0 for _ in range(len(poser.poses))]
    h = 0
    while poser.coro_keep_alive["example_thread"][0]:
        x, y, z, w = pyrr.Quaternion.from_y_rotation(h)
        for i in range(len(poser.poses)):
            poser.poses[i].x = np.sin(h/(i+1))
            poser.poses[i].z = np.cos(h/(i+1))

            poser.poses[i].r_x = x
            poser.poses[i].r_y = y
            poser.poses[i].r_z = z
            poser.poses[i].r_w = w

        h += 0.01


        await asyncio.sleep(poser.coro_keep_alive["example_thread"][1])


# @poser.thread_register(1, runInDefaultExecutor=True)
# def example_thread2():
#     while poser.coro_keep_alive["example_thread2"][0]:
#         poser.pose["x"] += 0.2

#         time.sleep(poser.coro_keep_alive["example_thread2"][1])


asyncio.run(poser.main())

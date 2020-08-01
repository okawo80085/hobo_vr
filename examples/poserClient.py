"""
this is an example of using virtualreality.templates

to get basic overview of virtualreality.templates.PoserClient run this in your python interpreter:
    help(virtualreality.templates.PoserClient)

to get more info on virtualreality.templates.PoserTemplate run this in your python interpreter:
    help(virtualreality.templates.PoserTemplate)

or visit https://github.com/okawo80085/hobo_vr/blob/master/examples/poserTemplate.py

more examples/references:
    https://github.com/okawo80085/hobo_vr/blob/master/virtualreality/trackers/color_tracker.py

"""

import asyncio
import time
import numpy as np
import pyrr

from virtualreality import templates
from virtualreality.server import server

poser = templates.PoserClient()

@poser.thread_register(1/60)
async def example_thread():
    h = 0
    while poser.coro_keep_alive["example_thread"][0]:
        # poser.pose.y = round(np.sin(h/2)-2, 4)
        poser.pose.y = -2.1
        # poser.pose.x = round(np.cos(h/2), 4)
        poser.pose.z = round(np.cos(h/2), 4)
        poser.pose_controller_l.x = round(0.5-np.cos(h*2)/5, 4)
        poser.pose_controller_l.y = -2
        poser.pose_controller_r.z = round(-1 - np.cos(h*2)/5, 4)
        poser.pose_controller_r.y = -2.2

        x, y, z, w = pyrr.Quaternion.from_z_rotation(h/2)
        poser.pose.r_x = round(x, 4)
        poser.pose.r_y = round(y, 4)
        poser.pose.r_z = round(z, 4)
        poser.pose.r_w = round(w, 4)

        x, y, z, w = pyrr.Quaternion.from_z_rotation(h*3)
        poser.pose_controller_r.r_x = round(x, 4)
        poser.pose_controller_r.r_y = round(y, 4)
        poser.pose_controller_r.r_z = round(z, 4)
        poser.pose_controller_r.r_w = round(w, 4)

        x, y, z, w = pyrr.Quaternion.from_z_rotation(-h*3)
        poser.pose_controller_l.r_x = round(x, 4)
        poser.pose_controller_l.r_y = round(y, 4)
        poser.pose_controller_l.r_z = round(z, 4)
        poser.pose_controller_l.r_w = round(w, 4)

        h += 0.01

        await asyncio.sleep(poser.coro_keep_alive["example_thread"][1])


# @poser.thread_register(1, runInDefaultExecutor=True)
# def example_thread2():
#     while poser.coro_keep_alive["example_thread2"][0]:
#         poser.pose["x"] += 0.2

#         time.sleep(poser.coro_keep_alive["example_thread2"][1])


asyncio.run(poser.main())

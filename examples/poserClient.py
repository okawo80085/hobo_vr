"""
this is an example of using virtualreality.templates

to get basic overview of virtualreality.templates.PoserClient run this in your python interpreter:
    help(virtualreality.templates.PoserClient)

to get more info on virtualreality.templates.PoserTemplate run this in your python interpreter:
    help(virtualreality.templates.PoserTemplate)

or visit https://github.com/okawo80085/hobo_vr/blob/master/examples/poserTemplate.py

more examples/references:
    https://github.com/okawo80085/hobo_vr/blob/master/poseTracker.py

"""

import asyncio
import time

from virtualreality import templates
from virtualreality.server import server

poser = templates.PoserClient()


@poser.thread_register(1.1)
async def example_thread():
    while poser.coro_keep_alive["example_thread"][0]:
        poser.pose["y"] += 0.2

        await asyncio.sleep(poser.coro_keep_alive["example_thread"][1])


@poser.thread_register(1, runInDefaultExecutor=True)
def example_thread2():
    while poser.coro_keep_alive["example_thread2"][0]:
        poser.pose["x"] += 0.2

        time.sleep(poser.coro_keep_alive["example_thread2"][1])


asyncio.run(poser)

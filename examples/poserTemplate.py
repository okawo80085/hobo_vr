"""
this is an example of using virtualreality.templates

to get detailed overview of virtualreality.templates.PoserTemplate run this in your python interpreter:
	help(virtualreality.templates.PoserTemplate)

more examples/references:
	https://github.com/okawo80085/hobo_vr/blob/master/virtualreality/trackers/color_tracker.py

"""

import asyncio
import time
import numpy as np

from virtualreality import templates


class MyPoser(templates.PoserTemplate):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    @templates.PoserTemplate.register_member_thread(1 / 100)
    async def example_thread1(self):
        '''moves the headset in a circle'''
        h = 0
        while self.coro_keep_alive["example_thread1"].is_alive:
            try:
                self.pose.x = np.sin(h)
                self.pose.y = np.cos(h)
                h += 0.01

                await asyncio.sleep(self.coro_keep_alive["example_thread1"].sleep_delay)

            except Exception as e:
                print(f"example_thread1 failed: {e}")
                break
        self.coro_keep_alive["example_thread1"].is_alive = False

    @templates.PoserTemplate.register_member_thread(1 / 100, runInDefaultExecutor=True)
    def example_thread2(self):
        '''moves the controller up and down'''
        h = 0
        while self.coro_keep_alive["example_thread2"].is_alive:
            try:
                self.pose_controller_l.y = 1+np.cos(h)/5
                h += 0.01

                time.sleep(self.coro_keep_alive["example_thread2"].sleep_delay)

            except Exception as e:
                print(f"example_thread2 failed: {e}")
                break
        self.coro_keep_alive["example_thread2"].is_alive = False


poser = MyPoser()

asyncio.run(poser.main())

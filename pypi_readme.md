# virtualreality
python side of [hobo_vr](https://github.com/okawo80085/hobo_vr)

mainly used for writing posers

has many helpful utilities for positional tracking, serial/socket/bluetooth communication, etc.

has an async socket server

# installation
to install the module, run the following command:
```
python -m pip install virtualreality
```
if you want to have camera based tracking, install it with the `camera` extra
```
python -m pip install virtualreality[camera]
```

to install the development version, do the following:
```
git clone https://github.com/okawo80085/hobo_vr
cd ./hobo_vr
python -m pip install -e .
```

# quick example
```python
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
        '''moves the left controller up and down'''
        h = 0
        while self.coro_keep_alive["example_thread2"].is_alive:
            try:
                self.pose_controller_l.y = 1+np.cos(h*3)/5
                h += 0.01

                time.sleep(self.coro_keep_alive["example_thread2"].sleep_delay)

            except Exception as e:
                print(f"example_thread2 failed: {e}")
                break
        self.coro_keep_alive["example_thread2"].is_alive = False


poser = MyPoser()

asyncio.run(poser.main())
```

# poser client example
```python
import asyncio
import time
import numpy as np

from virtualreality import templates

poser = templates.PoserClient()

@poser.thread_register(1/60)
async def example_thread():
    '''moves the headset in a circle'''
    h = 0
    while poser.coro_keep_alive["example_thread"].is_alive:
        poser.pose.y = round(np.sin(h), 4)
        poser.pose.x = round(np.cos(h), 4)

        h += 0.01

        await asyncio.sleep(poser.coro_keep_alive["example_thread"].sleep_delay)


@poser.thread_register(1/60, runInDefaultExecutor=True)
def example_thread2():
    '''moves the controller up and down'''
    while poser.coro_keep_alive["example_thread2"].is_alive:
        poser.pose_controller_l.x = 1+np.cos(h*3)/5

        time.sleep(poser.coro_keep_alive["example_thread2"].sleep_delay)


asyncio.run(poser.main())
```

# udu poser client example
```
u - unlimited
d - devices
u - upgrade
```
buts its more like a mode

here is an example
```python
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

poser = templates.UduPoserClient("h c t")
#                                 ^^^^^
#                                 this dictates devices used
#                                 for this example its:
#                                 hmd controller tracker
# poser = templates.UduPoserClient("h c c") btw normal posers use this device configuration


@poser.thread_register(1 / 60)
async def example_thread():
    # spins all devices in an orbit
    h = 0
    while poser.coro_keep_alive["example_thread"].is_alive:
        x, y, z, w = pyrr.Quaternion.from_y_rotation(h)
        for i in range(len(poser.poses)):
            poser.poses[i].x = np.sin(h / (i + 1))
            poser.poses[i].z = np.cos(h / (i + 1))

            poser.poses[i].r_x = x
            poser.poses[i].r_y = y
            poser.poses[i].r_z = z
            poser.poses[i].r_w = w

        h += 0.01

        await asyncio.sleep(poser.coro_keep_alive["example_thread"].sleep_delay)

# any poser can do this actually
@poser.thread_register(1 / 60)
async def example_receive_haptics_thread():
    while poser.coro_keep_alive["example_receive_haptics_thread"].is_alive:
        # check for new reads
        if poser.last_read:
            print (poser.last_read) # print received
            poser.last_read = b"" # reset

        await asyncio.sleep(poser.coro_keep_alive["example_receive_haptics_thread"].sleep_delay)

asyncio.run(poser.main())
```

unlike normal posers, udu posers generate pose structs on start

you then need use those pose structs as the value for `DeviceManifestList` in the driver config

[more info on udu](https://github.com/okawo80085/hobo_vr/wiki/udu)

# more examples
[hobo_vr's examples](https://github.com/okawo80085/hobo_vr/tree/master/examples)

there is also [`poser_3dof_hmd_only.py`](https://github.com/okawo80085/hobo_vr/blob/master/examples/poser_3dof_hmd_only.py), its an udu poser with actual 3dof tracking(more info in the example itself)
# python bindings for hobo_vr posers
A python implementation of the [hobovr driver protocol](https://github.com/okawo80085/hobo_vr/wiki/Driver:-communication-protocol) and a runtime for the said protocol, more info can be found [here](https://github.com/okawo80085/hobo_vr/wiki/virtualreality-API).

# installation
to install the module, run the following command:
```
python -m pip install virtualreality
```

to install the development version, do the following:
```
git clone https://github.com/okawo80085/hobo_vr
pip install -e ./hobo_vr/bindings/python
```

# udu poser client example
```
u - black
d - magic
u - mmmmmmmmmmmmmmmmmm yes
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
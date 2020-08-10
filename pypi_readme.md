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

to install the development version, do the following:
```
git clone https://github.com/okawo80085/hobo_vr
cd ./hobo_vr
python setup.py install --user
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

"""Templates for pose estimators, or posers."""
import asyncio
import numbers
import warnings

from ..util import utilz as u
from .template_base import *
from .poses import *
import struct


class PoserTemplate(PoserTemplateBase):
    """
    poser template.

    supplies 3 tracked device pose dicts:
        self.pose - hmd pose, position, orientation, velocity and angular velocity
        self.pose_controller_r - same as hmd pose +controller inputs for controller 1
        self.pose_controller_l - same as hmd pose +controller inputs for controller 2

    for more info: help(PoserTemplateBase)

    Example:
        class MyPoser(PoserTemplate):
            def __init__(self, *args, **kwargs):
                super().__init__(**kwargs)

            @PoserTemplate.register_member_thread(0.5)
            async def example_thread(self):
                while self.coro_keep_alive['example_thread'].is_alive:
                    self.pose.x += 0.04

                    await asyncio.sleep(self.coro_keep_alive['example_thread'].sleep_delay)

        poser = MyPoser()

        asyncio.run(poser.main())

    more examples:
        https://github.com/okawo80085/hobo_vr/blob/master/virtualreality/trackers/color_tracker.py
        https://github.com/okawo80085/hobo_vr/blob/master/examples/poserTemplate.py
        https://github.com/okawo80085/hobo_vr/blob/master/examples/poserClient.py

    """

    def __init__(self, *args, **kwargs):
        """
        init

        :addr: is the address of the server to connect to, stored in self.addr
        :port: is the port of the server to connect to, stored in self.port
        :send_delay: sleep delay for the self.send thread(in seconds)
        :recv_delay: sleep delay for the self.recv thread(in seconds)
        """
        super().__init__(**kwargs)

        self.pose = Pose()
        self.pose_controller_r = ControllerState(pose=(0.5, 1, -1, 1, 0, 0, 0))
        self.pose_controller_l = ControllerState(pose=(0.5, 1.1, -1, 1, 0, 0, 0))

    async def send(self):
        """Send all poses thread."""
        while self.coro_keep_alive["send"].is_alive:
            try:
                msg = b"".join(
                        [struct.pack('f'*len(self.pose), *self.pose.get_vals()),
                        struct.pack('f'*len(self.pose_controller_r), *self.pose_controller_r.get_vals()),
                        struct.pack('f'*len(self.pose_controller_l), *self.pose_controller_l.get_vals())]
                    ) + self._terminator

                self.writer.write(msg)
                await self.writer.drain()

                await asyncio.sleep(self.coro_keep_alive["send"].sleep_delay-0.0001)
            except Exception as e:
                print(f"send failed: {e}")
                break
        self.coro_keep_alive["send"].is_alive = False


class PoserClient(PoserTemplate, PoserClientBase):
    """
    PoserClient.

    example usage:
        poser = PoserClient()

        @poser.thread_register(1)
        async def lol():
            while poser.coro_keep_alive['lol'].is_alive:
                poser.pose.x += 0.2

                await asyncio.sleep(poser.coro_keep_alive['lol'].sleep_delay)

        asyncio.run(poser.main())

    more info: help(PoserTemplate)
    """

    def __init__(self, *args, **kwargs):
        """init"""
        print(
            "starting the client"
        )  # this is here bc otherwise it looks sad, useless and empty
        super().__init__(*args, **kwargs)

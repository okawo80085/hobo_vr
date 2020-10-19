"""Templates for pose estimators, or posers. unlimited devices upgrade version."""
import asyncio
import numbers
import warnings

from ..util import utilz as u
from .template_base import *
from .poses import *
import re
import struct



class UduPoserTemplate(PoserTemplateBase):
    """
    udu poser template.

    supplies a list of poses:
        self.poses - pose object will correspond to value of device_list_manifest, keep in mind that, as of now, only 3 types of devices are supported(h-hmd, c-controller, t-tracker)

    for more info: help(PoserTemplateBase)

    Example:
        class MyPoser(UduPoserTemplate):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            @UduPoserTemplate.register_member_thread(0.5)
            async def example_thread(self):
                while self.coro_keep_alive['example_thread'].is_alive:
                    self.poses[0].x += 0.04

                    await asyncio.sleep(self.coro_keep_alive['example_thread'].sleep_delay)

        poser = MyPoser('h c c')

        asyncio.run(poser.main())

    more examples:
        https://github.com/okawo80085/hobo_vr/blob/master/examples/uduPoserClient.py

    """

    def __init__(self, device_list_manifest, *args, **kwargs):
        """
        init

        :device_list_manifest: should completely match this regex: ([htc][ ])*([htc]$)
        :addr: is the address of the server to connect to, stored in self.addr
        :port: is the port of the server to connect to, stored in self.port
        :send_delay: sleep delay for the self.send thread(in seconds)
        :recv_delay: sleep delay for the self.recv thread(in seconds)
        """
        super().__init__(**kwargs)

        re_s = re.search("([htc][ ])*([htc]$)", device_list_manifest)

        if not device_list_manifest or re_s is None:
            raise RuntimeError("empty pose struct")

        if re_s.group() != device_list_manifest:
            raise RuntimeError(f"invalid pose struct: {repr(device_list_manifest)}")

        self.poses = []
        self.device_types = device_list_manifest.split(" ")

        new_struct = []
        for i in self.device_types:
            if i == "h" or i == "t":
                self.poses.append(Pose())

            elif i == "c":
                self.poses.append(ControllerState())
            
            new_struct.append(f"{i}{len(self.poses[-1])}")

        new_struct = " ".join(new_struct)

        print(
            f"total of {len(self.poses)} device(s) have been added, a new pose struct has been generated: {repr(new_struct)}"
        )
        print(
            "full device list is now available through self.device_types, all device poses are in self.poses"
        )

    async def send(self):
        """Send all poses thread."""
        while self.coro_keep_alive["send"].is_alive:
            try:
                msg = b"".join([struct.pack('f'*len(i), *i.get_vals()) for i in self.poses]) + self._terminator

                self.writer.write(msg)
                await self.writer.drain()

                await asyncio.sleep(self.coro_keep_alive["send"].sleep_delay-0.0001)
            except Exception as e:
                print(f"send failed: {e}")
                break
        self.coro_keep_alive["send"].is_alive = False


class UduPoserClient(UduPoserTemplate, PoserClientBase):
    """
    UduPoserClient.

    example usage:
        poser = UduPoserClient('h c c') # normal poser device setup: hmd controller controller

        @poser.thread_register(1)
        async def lol():
            while poser.coro_keep_alive['lol'].is_alive:
                poser.poses[0].x += 0.2

                await asyncio.sleep(poser.coro_keep_alive['lol'].sleep_delay)

        asyncio.run(poser.main())

    more info: help(UduPoserTemplate)
    """

    def __init__(self, *args, **kwargs):
        """init"""
        print(
            "starting udu client"
        )  # this is here bc otherwise it looks sad, useless and empty
        super().__init__(*args, **kwargs)

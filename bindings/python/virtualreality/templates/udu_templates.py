# (c) 2021 Okawo
# This code is licensed under MIT license (see LICENSE for details)

"""
Templates for pose estimators, or posers. unlimited devices upgrade version.
"""
import asyncio
# import numbers
# import warnings

# from ..util import utilz as u
from .template_base import *
from .poses import *
import re
import struct
import numpy as np


class UduPoserTemplate(PoserTemplateBase):
    """
    udu poser template.

    supplies a list of poses:
        self.poses - pose objects

    for more info: help(PoserTemplateBase)

    Example:
        class MyPoser(UduPoserTemplate):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            @UduPoserTemplate.register_member_thread(0.5)
            async def example_thread(self):
                while self.coro_keep_alive['example_thread'].is_alive:
                    self.poses[0].x += 0.04

                    await asyncio.sleep(
                        self.coro_keep_alive['example_thread'].sleep_delay
                    )

        poser = MyPoser('h c c')

        asyncio.run(poser.main())

    """

    def __init__(self, udu_string, *args, **kwargs):
        """
        init

        :udu_string: should completely match this regex: ([htc][ ])*([htc]$)
        :addr: is the address of the server to connect to, stored in self.addr
        :port: is the port of the server to connect to, stored in self.port
        :send_delay: sleep delay for the self.send thread(in seconds)
        :recv_delay: sleep delay for the self.recv thread(in seconds)
        """
        super().__init__(**kwargs)

        re_s = re.search("([htc][ ])*([htc]$)", udu_string)

        if not udu_string or re_s is None:
            raise RuntimeError("empty pose struct")

        if re_s.group() != udu_string:
            raise RuntimeError(f"invalid pose struct: {repr(udu_string)}")

        self.poses = []
        self.device_types = udu_string.split(" ")

        new_struct = []
        for i in self.device_types:
            if i == "h" or i == "t":
                self.poses.append(Pose())

            elif i == "c":
                self.poses.append(ControllerState())

            new_struct.append(f"{i}{len(self.poses[-1])}")

        new_struct = " ".join(new_struct)

        print(
            f"total of {len(self.poses)} device(s) have been added, your new udu settings: {repr(new_struct)}"  # noqa E501
        )
        print(
            "full device list is now available through self.device_types, all device poses are in self.poses"  # noqa E501
        )

    async def _sync_udu(self, new_udu_string):
        # update own udu
        newUduString = new_udu_string
        re_s = re.search("([htc][ ])*([htc]$)", newUduString)

        if not newUduString or re_s is None:
            print('invalid udu string')
            return

        if re_s.group() != newUduString:
            print('invalid udu string')
            return

        newPoses = []
        new_struct = []
        for i in newUduString.split(' '):
            if i == "h" or i == "t":
                newPoses.append(Pose())

            elif i == "c":
                newPoses.append(ControllerState())

            new_struct.append(f"{i}{len(newPoses[-1])}")

        self.poses = newPoses
        new_struct = " ".join(new_struct)
        print(
            f"new udu settings: {repr(new_struct)}, {len(self.poses)} device(s) total" # noqa E501
        )
        # send new udu
        udu_len = len(self.poses)
        type_d = {'h': (0, 13), 'c': (1, 22), 't': (2, 13)}
        packet = np.array(
            [type_d[i] for i in newUduString.split(' ')],
            dtype=np.uint32
        )
        packet = packet.reshape(packet.shape[0] * packet.shape[1])

        data = settManager_Message_t.pack(
            20,
            udu_len,
            *packet,
            *np.zeros((128 - packet.shape[0], ), dtype=np.uint32)
        )
        resp = await self._send_manager(data)
        return resp

    async def send(self):
        """Send all poses thread."""
        while self.coro_keep_alive["send"].is_alive:
            try:
                msg = b"".join(
                    [struct.pack(f"{len(i)}f", *i.get_vals()) for i in self.poses] # noqa E501
                ) + self._terminator
                self.writer.write(msg)
                await self.writer.drain()
                # print('written and drained')

                await asyncio.sleep(
                    self.coro_keep_alive["send"].sleep_delay - 0.0001
                )
            except Exception as e:
                print(f"send failed: {e}")
                break
        self.coro_keep_alive["send"].is_alive = False


class UduPoserClient(UduPoserTemplate, PoserClientBase):
    """
    UduPoserClient.

    example usage:
        poser = UduPoserClient('h c c') # hmd, controller, controller

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

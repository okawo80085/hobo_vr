"""Templates for pose estimators, or posers."""
import asyncio
import numbers
import warnings

from ..util import utilz as u
from .template_base import *
from .poses import *

class PoserTemplate(PoserTemplateBase):
    """
    poser template.

    supplies 3 tracked device pose dicts:
        self.pose - hmd pose, position, orientation, velocity and angular velocity
        self.pose_controller_r - same as hmd pose +controller inputs for controller 1
        self.pose_controller_l - same as hmd pose +controller inputs for controller 2

    supplies a last message from server buffer:
        self.last_read - string containing the last message from the server, it is recommended to consume it, it will be populated with new data when its received

    supplies threading vars:
        self.coro_list - list of all methods recognized as threads
        self.coro_keep_alive - dict of all registered threads, containing self.coro_keepAlive['memberThreadName'] = KeepAliveTrigger(is_alive, sleep_delay), this dict is populated at self.main() call

    this class also has 3 built in threads, it is not recommended you override any of them:
        self.send - sends all pose data to the server
        self.recv - receives messages from the server, the last message is stored in self.lastRead
        self.close - closes the connection to the server and ends all threads that where registered by register_member_thread when the 'q' key is pressed

    this class will assume that every
    child method without '_' as a first character in the name is a thread,
    unless the name of that method has been added to self._coro_name_exceptions

    every child class also needs to register it's thread
    methods with the PoserTemplate.register_member_thread decorator

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
                msg = u.format_str_for_write(
                    " ".join(
                        [str(i) for i in get_slot_values(self.pose)]
                        + [str(i) for i in get_slot_values(self.pose_controller_r)]
                        + [str(i) for i in get_slot_values(self.pose_controller_l)]
                    )
                )

                self.writer.write(msg)
                await self.writer.drain()

                await asyncio.sleep(self.coro_keep_alive["send"].sleep_delay)
            except Exception as e:
                print(f"send failed: {e}")
                self.coro_keep_alive["send"][0] = False
                break


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
    """

    def __init__(self, *args, **kwargs):
        """init"""
        print ('starting the client')
        super().__init__(*args, **kwargs)

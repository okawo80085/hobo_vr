"""Templates for pose estimators, or posers. unlimited devices upgrade version."""
import asyncio
import numbers
import warnings

from ..util import utilz as u
from .template_base import *
from .poses import *
import re


class UduPoserTemplate(PoserTemplateBase):
    """
    udu poser template.

    supplies a list of poses:
        self.poses - pose object will correspond to value of device_list_manifest, keep in mind that, as of now, only 3 types of devices are supported(h-hmd, c-controller, t-tracker)

    supplies a last message from server buffer:
        self.last_read - string containing the last message from the server, it is recommended to consume it, it will be populated with new data when its received

    supplies threading vars:
        self.coro_list - list of all methods recognized as threads
        self.coro_keep_alive - dict of all registered threads, containing self.coro_keepAlive['memberThreadName'] = [KeepAliveBool, SleepDelayInSeconds], this dict is populated at self.main() call

    this class also has 3 built in threads, it is not recommended you override any of them:
        self.send - sends all pose data to the server
        self.recv - receives messages from the server, the last message is stored in self.lastRead
        self.close - closes the connection to the server and ends all threads that where registered by thread_register

    this class will assume that every
    child method without '_' as a first character in the name is a thread,
    unless the name of that method has been added to self._coro_name_exceptions

    every child class also needs to register it's thread
    methods with the PoserTemplate.register_member_thread decorator
    Example:
        class MyPoser(UduPoserTemplate):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            @UduPoserTemplate.register_member_thread(0.5)
            async def example_thread(self):
                while self.coro_keep_alive['example_thread'][0]:
                    self.poses[0].x += 0.04

                    await asyncio.sleep(self.coro_keep_alive['example_thread'][1])

        poser = MyPoser('h c c')

        asyncio.run(poser.main())

    more examples:
        https://github.com/okawo80085/hobo_vr/blob/master/examples/uduPoserClient.py

    """

    def __init__(self, device_list_manifest, *args, **kwargs):
        """
        init

        :device_list_manifest: should match this regex: ([htc][ ])+([htc]$)|[htc]$
        :addr: is the address of the server to connect to, stored in self.addr
        :port: is the port of the server to connect to, stored in self.port
        :send_delay: sleep delay for the self.send thread(in seconds)
        :recv_delay: sleep delay for the self.recv thread(in seconds)
        """
        super().__init__(**kwargs)

        re_s = re.search('([htc][ ])+([htc]$)|[htc]$', device_list_manifest)

        if not device_list_manifest or re_s is None:
            raise RuntimeError('empty pose struct')

        if re_s.group() != device_list_manifest:
            raise RuntimeError(f'invalid pose struct: {repr(device_list_manifest)}')

        self.poses = []
        self.device_types = device_list_manifest.split(' ')

        new_struct = []
        for i in self.device_types:
            if i == 'h' or i == 't':
                self.poses.append(Pose())
                new_struct.append(f'{i}{len(self.poses[-1])}')

            elif i == 'c':
                self.poses.append(ControllerState())
                new_struct.append(f'{i}{len(self.poses[-1])}')

        new_struct = ' '.join(new_struct)

        print (f'total of {len(self.poses)} device(s) have been added, a new pose struct has been generated: {repr(new_struct)}')
        print ('full device list is available through self.poses')

    async def send(self):
        """Send all poses thread."""
        poses_index = range(len(self.device_types))
        while self.coro_keep_alive["send"][0]:
            try:
                msg = u.format_str_for_write(' '.join([str(j) for i in poses_index for j in get_slot_values(self.poses[i])]))

                self.writer.write(msg)
                await self.writer.drain()

                await asyncio.sleep(self.coro_keep_alive["send"][1])
            except Exception as e:
                print(f"send failed: {e}")
                self.coro_keep_alive["send"][0] = False
                break


class UduPoserClient(UduPoserTemplate):
    """
    UduPoserClient.

    example usage:
        poser = UduPoserClient()

        @poser.thread_register(1)
        async def lol():
            while poser.coro_keepAlive['lol'][0]:
                poser.pose.x += 0.2

                await asyncio.sleep(poser.coro_keepAlive['lol'][1])

        asyncio.run(poser.main())
    """

    def __init__(self, *args, **kwargs):
        """init"""
        super().__init__(*args, **kwargs)
        self._coro_name_exceptions.append("thread_register")

    def thread_register(self, sleep_delay, runInDefaultExecutor=False):
        """
        Register a thread for UduPoserClient.

        sleepDelay - sleep delay in seconds
        runInDefaultExecutor - bool, set True if you want the function to be executed in asyncio's default pool executor
        """

        def _thread_register(coro):
            if not asyncio.iscoroutinefunction(coro) and not runInDefaultExecutor:
                raise ValueError(f"{repr(coro)} is not a coroutine function and runInDefaultExecutor is set to False")

            if coro.__name__ not in self.coro_keep_alive and coro.__name__ not in self.coro_list:
                self.coro_keep_alive[coro.__name__] = [True, sleep_delay]
                self.coro_list.append(coro.__name__)

                if runInDefaultExecutor:

                    def _wrapper(*args, **kwargs):
                        setattr(_wrapper, "__name__", f"{coro.__name__} _decorated")
                        loop = asyncio.get_running_loop()

                        return loop.run_in_executor(None, coro, *args, **kwargs)

                    setattr(_wrapper, "__name__", coro.__name__)
                    setattr(self, coro.__name__, _wrapper)
                    return _wrapper

                else:
                    setattr(self, coro.__name__, coro)

            else:
                raise NameError(
                    f"trying to register existing thread, thread with name {repr(coro.__name__)} already exists"
                )

            return coro

        return _thread_register

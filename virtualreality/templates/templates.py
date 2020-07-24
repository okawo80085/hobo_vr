"""Templates for pose estimators, or posers."""
import asyncio
import numbers
import warnings

from ..util import utilz as u
from .poses import *

class PoserTemplate:
    """
    Poser base class.

    supplies 3 tracked device pose dicts:
        self.pose - hmd pose, position, orientation, velocity and angular velocity only
        self.pose_controller_r - same as hmd pose +controller inputs for controller 1
        self.pose_controller_l - same as hmd pose +controller inputs for controller 2
    
    more info on poses/message format:
        https://github.com/okawo80085/hobo_vr/wiki/poser-message-format

    supplies a last message from server buffer:
        self.last_read - string containing the last message from the server

    supplies threading vars:
        self.coro_list - list of all methods recognized as threads
        self.coro_keep_alive - dict of all registered threads, containing self.coro_keepAlive['threadMethodName'] = [KeepAliveBool, SleepDelay], the dict is populated at self.main() call

    this base class also has 3 built in threads, it is not recommended you override any of them:
        self.send - sends all pose data to the server
        self.recv - receives messages from the server, the last message is stored in self.lastRead
        self.close - closes the connection to the server and ends all threads that where registered by thread_register

    this base class will assume that every
    child method without '_' as a first character
    in the name is a thread, unless the name of that method has been added to self._coro_name_exceptions

    every child class also needs to register it's thread
    methods with the thread_register decorator

    Example:
        class MyPoser(PoserTemplate):
            def __init__(self, *args, **kwargs):
                super().__init(**kwargs)

            @thread_register(0.5)
            async def example_thread(self):
                while self.coro_keep_alive['example_thread'][0]:
                    self.pose['x'] += 0.04

                    await asyncio.sleep(self.coro_keep_alive['example_thread'][1])

        poser = MyPoser()

        asyncio.run(poser.main())

    more examples:
        https://github.com/okawo80085/hobo_vr/blob/master/virtualreality/trackers/color_tracker.py
        https://github.com/okawo80085/hobo_vr/blob/master/examples/poserTemplate.py

    """

    def __init__(
        self, *, addr="127.0.0.1", port=6969, send_delay=1 / 100, recv_delay=1 / 1000, **kwargs,
    ):
        """
        Create the poser template.

        :param addr: is the address of the server to connect to, stored in self.addr
        :param port: is the port of the server to connect to, stored in self.port
        :param send_delay: sleep delay for the self.send thread(in seconds)
        :param recv_delay: sleep delay for the self.recv thread(in seconds)
        """
        self.addr = addr
        self.port = port

        self._coro_name_exceptions = ["main"]

        self.coro_list = [
            method_name
            for method_name in dir(self)
            if callable(getattr(self, method_name))
            and method_name[0] != "_"
            and method_name not in self._coro_name_exceptions
        ]

        self.coro_keep_alive = {
            "close": [True, 0.1],
            "send": [True, send_delay],
            "recv": [True, recv_delay],
        }
        self.last_read = ""
        self.id_message = 'holla'

        self.pose = Pose()
        self.pose_controller_r = ControllerState(pose=(0.5, 1, -1, 1, 0, 0, 0))
        self.pose_controller_l = ControllerState(pose=(0.5, 1.1, -1, 1, 0, 0, 0))

    async def _socket_init(self):
        """
        Connect to the server using self.addr and self.port and send the id message.

        Also store socket reader and writer in self.reader and self.writer.
        It is not recommended you override this method

        send id message
        more on id messages: https://github.com/okawo80085/hobo_vr/wiki/server-id-messages
        """
        print(f'connecting to the server at "{self.addr}:{self.port}"...')
        # connect to the server
        self.reader, self.writer = await asyncio.open_connection(self.addr, self.port, loop=asyncio.get_event_loop())
        # send poser id message
        self.writer.write(u.format_str_for_write(self.id_message))

    async def send(self):
        """Send all poses thread."""
        while self.coro_keep_alive["send"][0]:
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

                await asyncio.sleep(self.coro_keep_alive["send"][1])
            except Exception as e:
                print(f"send failed: {e}")
                self.coro_keep_alive["send"][0] = False
                break

    async def recv(self):
        """Receive messages thread."""
        while self.coro_keep_alive["recv"][0]:
            try:
                data = await u.read(self.reader)
                self.last_read = data

                await asyncio.sleep(self.coro_keep_alive["recv"][1])
            except Exception as e:
                print(f"recv failed: {e}")
                self.coro_keep_alive["recv"][0] = False
                break

    async def close(self):
        """Await close thread, press "q" to kill all registered threads."""
        try:
            import keyboard

            while self.coro_keep_alive["close"][0]:
                if keyboard.is_pressed("q"):
                    break

                await asyncio.sleep(self.coro_keep_alive["close"][1])

        except ImportError as e:
            print(f"close: failed to import keyboard, poser will close in 10 seconds: {e}")
            await asyncio.sleep(10)

        print("closing...")
        self.coro_keep_alive["close"][0] = False
        for key in self.coro_keep_alive:
            if self.coro_keep_alive[key][0]:
                self.coro_keep_alive[key][0] = False
                print(f"{key} stop sent...")
            else:
                print(f"{key} already stopped")

        await asyncio.sleep(0.5)

        try:
            self.writer.write(u.format_str_for_write("CLOSE"))
            self.writer.close()
            await self.writer.wait_closed()

        except Exception as e:
            print (f'failed to close connection: {e}')

        print("finished")

    async def main(self):
        """
        Run the main async method.

        Gathers all recognized threads and runs them.
        It is not recommended you override this method.
        """
        await self._socket_init()

        await asyncio.gather(
            *[getattr(self, coro_name)() for coro_name in self.coro_list if coro_name not in self._coro_name_exceptions]
        )


def thread_register(sleepDelay, runInDefaultExecutor=False):
    """
    Register thread for PoserTemplate base class.

    sleepDelay - sleep delay in seconds
    runInDefaultExecutor - bool, set True if you want the function to be executed in asyncio's default pool executor
    """

    def _thread_reg(func):
        def _thread_reg_wrapper(self, *args, **kwargs):

            if not asyncio.iscoroutinefunction(func) and not runInDefaultExecutor:
                raise ValueError(f"{repr(func)} is not a coroutine function and runInDefaultExecutor is set to False")

            if func.__name__ not in self.coro_keep_alive and func.__name__ in self.coro_list:
                self.coro_keep_alive[func.__name__] = [True, sleepDelay]

            else:
                warnings.warn("thread register ignored, thread already exists", RuntimeWarning)

            if runInDefaultExecutor:
                loop = asyncio.get_running_loop()

                return loop.run_in_executor(None, func, self, *args, **kwargs)

            return func(self, *args, **kwargs)

        setattr(_thread_reg_wrapper, "__name__", func.__name__)

        return _thread_reg_wrapper

    return _thread_reg


class PoserClient(PoserTemplate):
    """
    PoserClient.

    example usage:
        poser = PoserClient()

        @poser.thread_register(1)
        async def lol():
            while poser.coro_keepAlive['lol'][0]:
                poser.pose['x'] += 0.2

                await asyncio.sleep(poser.coro_keepAlive['lol'][1])

        asyncio.run(poser.main())
    """

    def __init__(self, *args, **kwargs):
        """Create the poser client."""
        super().__init__(*args, **kwargs)
        self._coro_name_exceptions.append("thread_register")

    def thread_register(self, sleep_delay, runInDefaultExecutor=False):
        """
        Register a thread for PoserClient.

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

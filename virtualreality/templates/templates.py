"""Templates for pose estimators, or posers."""
import asyncio
import numbers
import warnings

from ..util import utilz as u


class PoseEuler(object):
    """Struct containing all variables needed for instantaneous pose."""

    __slots__ = [
        "x",
        "y",
        "z",
        "vel_x",
        "vel_y",
        "vel_z",
        "acc_x",
        "acc_y",
        "acc_z",
        "ang_x",
        "ang_y",
        "ang_z",
        "vel_ang_x",
        "vel_ang_y",
        "vel_ang_z",
        "acc_ang_x",
        "acc_ang_y",
        "acc_ang_z",
    ]

    def __init__(
        self,
        x: float = 0,
        y: float = 0,
        z: float = 0,
        vel_x: float = 0,
        vel_y: float = 0,
        vel_z: float = 0,
        acc_x: float = 0,
        acc_y: float = 0,
        acc_z: float = 0,
        ang_x: float = 0,
        ang_y: float = 0,
        ang_z: float = 0,
        vel_ang_x: float = 0,
        vel_ang_y: float = 0,
        vel_ang_z: float = 0,
        acc_ang_x: float = 0,
        acc_ang_y: float = 0,
        acc_ang_z: float = 0,
    ):
        """
        Instantaneous pose and velocity of an object.

        :param pose: location in meters and orientation in quaternion
        :param velocity: velocity in meters/second
                         Angular velocity of the pose in axis-angle representation. The direction is the angle of
                         rotation and the magnitude is the angle around that axis in radians/second.
        """
        self.x = x
        self.y = y
        self.z = z
        self.vel_x = vel_x
        self.vel_y = vel_y
        self.vel_z = vel_z
        self.acc_x = acc_x
        self.acc_y = acc_y
        self.acc_z = acc_z
        self.ang_x = ang_x
        self.ang_y = ang_y
        self.ang_z = ang_z
        self.vel_ang_x = vel_ang_x
        self.vel_ang_y = vel_ang_y
        self.vel_ang_z = vel_ang_z
        self.acc_ang_x = acc_ang_x
        self.acc_ang_y = acc_ang_y
        self.acc_ang_z = acc_ang_z

    def __getitem__(self, key):
        if key in self.__slots__:
            return getattr(self, key)

        raise KeyError(key)

    def __setitem__(self, key, val):
        assert isinstance(val, numbers.Number), "value should be numeric"

        if key in self.__slots__:
            setattr(self, key, val)
            return

        raise KeyError(key)

    def __repr__(self):
        return "<{}.{} [{}\n] object at {}>".format(
            self.__class__.__module__,
            self.__class__.__name__,
            "\n\t".join([f"{key}={self[key]}" for key in self.__slots__]),
            hex(id(self)),
        )


class Pose(object):
    """Struct containing all variables needed for instantaneous pose."""

    __slots__ = [
        "x",
        "y",
        "z",
        "r_w",
        "r_x",
        "r_y",
        "r_z",
        "vel_x",
        "vel_y",
        "vel_z",
        "ang_vel_x",
        "ang_vel_y",
        "ang_vel_z",
    ]

    def __init__(self, pose=(0, 0, 0, 1, 0, 0, 0), velocity=(0,) * 6):
        """
        Instantaneous pose and velocity of an object.

        :param pose: location in meters and orientation in quaternion
        :param velocity: velocity in meters/second
                         Angular velocity of the pose in axis-angle representation. The direction is the angle of
                         rotation and the magnitude is the angle around that axis in radians/second.
        """
        self.x: float = pose[0]  # +(x) is right in meters
        self.y: float = pose[1]  # +(y) is up in meters
        self.z: float = pose[2]  # -(z) is forward in meters
        self.r_w: float = pose[3]  # from 0 to 1
        self.r_x: float = pose[4]  # from -1 to 1
        self.r_y: float = pose[5]  # from -1 to 1
        self.r_z: float = pose[6]  # from -1 to 1

        self.vel_x: float = velocity[0]  # +(x) is right in meters/second
        self.vel_y: float = velocity[1]  # +(y) is up in meters/second
        self.vel_z: float = velocity[2]  # -(z) is forward in meters/second
        self.ang_vel_x: float = velocity[3]
        self.ang_vel_y: float = velocity[4]
        self.ang_vel_z: float = velocity[5]

    def __getitem__(self, key):
        if key in self.__slots__:
            return getattr(self, key)

        raise KeyError(key)

    def __setitem__(self, key, val):
        assert isinstance(val, numbers.Number), "value should be numeric"

        if key in self.__slots__:
            setattr(self, key, val)
            return

        raise KeyError(key)

    def __repr__(self):
        return "<{}.{} [{}] object at {}>".format(
            self.__class__.__module__,
            self.__class__.__name__,
            ", ".join([f"{key}={self[key]}" for key in self.__slots__]),
            hex(id(self)),
        )


"""class CoroutineKeepAlive(object):
    def __init__(self, send_delay, receive_delay, close_delay=0.1):
        self.send = [True, send_delay]
        self.receive = [True, receive_delay]
        self.close = [True, close_delay]"""


class ControllerState(Pose):
    """Struct containing all variables needed for instantaneous controller state."""

    __slots__ = [
        "x",
        "y",
        "z",
        "r_w",
        "r_x",
        "r_y",
        "r_z",
        "vel_x",
        "vel_y",
        "vel_z",
        "ang_vel_x",
        "ang_vel_y",
        "ang_vel_z",
        "grip",
        "system",
        "menu",
        "trackpad_click",
        "trigger_value",
        "trackpad_x",
        "trackpad_y",
        "trackpad_touch",
        "trigger_click",
    ]

    def __init__(
        self,
        pose=(0, 0, 0, 1, 0, 0, 0),
        velocity=(0,) * 6,
        grip=0,
        system=0,
        menu=0,
        trackpad_click=0,
        trigger_value=0,
        trackpad_x=0,
        trackpad_y=0,
        trackpad_touch=0,
        trigger_click=0,
    ):
        """
        Instantaneous controller state.

        :param pose: location in meters and orientation in quaternion
        :param velocity: velocity in meters/second
                         Angular velocity of the pose in axis-angle representation. The direction is the angle of
                         rotation and the magnitude is the angle around that axis in radians/second.
        :param grip: is grip down
        :param system: is system button down
        :param menu: is menu button down
        :param trackpad_click: is trackpad clicked
        :param trigger_value: trigger pressure value
        :param trackpad_x: trackpad touch x position
        :param trackpad_y: trackpad touch y position
        :param trackpad_touch: is trackpad being touched
        :param trigger_click: is trigger clicked
        """
        super().__init__(pose, velocity)
        self.grip: int = grip  # 0 or 1
        self.system: int = system  # 0 or 1
        self.menu: int = menu  # 0 or 1
        self.trackpad_click: int = trackpad_click  # 0 or 1
        self.trigger_value: int = trigger_value  # 0 or 1
        self.trackpad_x: float = trackpad_x  # from -1 to 1
        self.trackpad_y: float = trackpad_y  # from -1 to 1
        self.trackpad_touch: int = trackpad_touch  # 0 or 1
        self.trigger_click: int = trigger_click  # 0 or 1


def get_slot_names(slotted_instance):
    """Get all slot names in a class with slots."""
    # thanks: https://stackoverflow.com/a/6720815/782170
    return slotted_instance.__slots__


def get_slot_values(slotted_instance):
    """Get all slot values in a class with slots."""
    # thanks: https://stackoverflow.com/a/6720815/782170
    return [getattr(slotted_instance, slot) for slot in get_slot_names(slotted_instance)]


class PoserTemplate:
    """
    Poser base class.

    self.main() -

    supplies 3 tracked device pose dicts:
        self.pose - hmd pose, position, orientation, velocity and angular velocity only
        self.poseControllerR - same as hmd pose +controller inputs for controller 1
        self.poseControllerL - same as hmd pose +controller inputs for controller 2
    
    more info on poses/message format:
        https://github.com/okawo80085/hobo_vr/wiki/poser-message-format

    supplies a last message from server buffer:
        self.lastRead - string containing the last message from the server

    supplies threading vars:
        self.coro_list - list of all methods recognized as threads
        self.coro_keepAlive - dict of all registered threads, containing self.coro_keepAlive['threadMethodName'] = [KeepAliveBool, SleepDelay], the dict is populated at self.main() call

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
                while self.coro_keepAlive['example_thread'][0]:
                    self.pose['x'] += 0.04

                    await asyncio.sleep(self.coro_keepAlive['example_thread'][1])

        poser = MyPoser()

        asyncio.run(poser.main())

    more examples:
        https://github.com/okawo80085/hobo_vr/blob/master/poseTracker.py
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

        self.pose = Pose()
        self.pose_controller_r = ControllerState(pose=(0.5, 1, -1, 1, 0, 0, 0))
        self.pose_controller_l = ControllerState(pose=(0.5, 1.1, -1, 1, 0, 0, 0))
        self.last_read = ""

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
        self.writer.write(u.format_str_for_write("poser here"))

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
                    self.coro_keep_alive["close"][0] = False
                    break

                await asyncio.sleep(self.coro_keep_alive["close"][1])

        except ImportError as e:
            print(f"close: failed to import keyboard, poser will close in 10 seconds: {e}")
            await asyncio.sleep(10)

        print("closing...")
        for key in self.coro_keep_alive:
            self.coro_keep_alive[key][0] = False
            print(f"{key} stop sent...")

        await asyncio.sleep(1)

        self.writer.write(u.format_str_for_write("CLOSE"))
        self.writer.close()

        print("done")

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

"""base template class"""
import asyncio
import numbers
import warnings
from aioconsole import ainput
import shlex
import docopt

try:
    from ..util import utilz as u

except Exception as e:
    print (f'failed to import utilities, some things will be broken: {e}')


class KeepAliveTrigger:
    """a keep alive trigger, used in poser templates for threading signals"""

    __slots__ = ["is_alive", "sleep_delay"]

    def __init__(self, is_alive, sleep_delay):
        """
        :is_alive: bool, stored in self.is_alive
        :sleep_delay: sleep delay in seconds, stored in self.sleep_delay
        """
        self.is_alive = is_alive
        self.sleep_delay = sleep_delay


class PoserTemplateBase(object):
    """
    Base template class, should only be used to create new poser templates


    supplies a last message from server buffer:
        self.last_read - bytearray containing the last message from the server, it is recommended to consume it, it will be populated with new data when its received

    supplies threading vars:
        self.coro_list - list of all methods recognized as threads
        self.coro_keep_alive - dict of all registered threads, containing self.coro_keepAlive['memberThreadName'] = KeepAliveTrigger(is_alive, sleep_delay), this dict is populated at self.main() call

    this class also has 3 built in threads, it is not recommended you override any of them:
        self.send - sends all pose data, you need to implement this!
        self.recv - receives messages from the server, the last message is stored in self.lastRead
        self.close - closes the connection to the server and ends all threads that where registered by register_member_thread when the 'q' key is pressed

    this class will assume that every
    child method without '_' as a first character in the name is a thread,
    unless the name of that method has been added to self._coro_name_exceptions

    every child class also needs to register it's thread
    methods with the PoserTemplate.register_member_thread decorator
    """

    _CLI_SETTS = '''hobo_vr poser

Usage: poser [-h | --help] [options]

Options:
    -h, --help      shows this message
    -q, --quit      exits the poser
'''

    def __init__(
        self,
        *,
        addr="127.0.0.1",
        port=6969,
        send_delay=1 / 100,
        recv_delay=1 / 1000,
        **kwargs,
    ):
        """
        :addr: is the address of the server to connect to, stored in self.addr
        :port: is the port of the server to connect to, stored in self.port
        :send_delay: sleep delay for the self.send thread(in seconds)
        :recv_delay: sleep delay for the self.recv thread(in seconds)
        """
        self.addr = addr
        self.port = port

        self._coro_name_exceptions = ["main", "register_member_thread"]

        self.coro_list = [
            method_name
            for method_name in dir(self)
            if callable(getattr(self, method_name))
            and method_name[0] != "_"
            and method_name not in self._coro_name_exceptions
        ]

        self.coro_keep_alive = {
            "close": KeepAliveTrigger(True, 0.1),
            "send": KeepAliveTrigger(True, send_delay),
            "recv": KeepAliveTrigger(True, recv_delay),
        }
        self.last_read = b""
        self.id_message = "holla"
        self._terminator = b"\t\r\n"

    async def _socket_init(self):
        """
        Connect to the server using self.addr and self.port and send the id message.

        Also store socket reader and writer in self.reader and self.writer.
        It is not recommended you override this method
        """
        print(f'connecting to the server at "{self.addr}:{self.port}"...')
        # connect to the server
        self.reader, self.writer = await asyncio.open_connection(
            self.addr, self.port, loop=asyncio.get_event_loop()
        )
        # send poser id message
        self.writer.write(u.format_str_for_write(self.id_message))

    async def send(self):
        """Send all poses thread, you need to implement this!"""
        raise NotImplementedError("please implement the send thread")

    async def recv(self):
        """Receive messages thread."""
        backBuffer = bytearray()
        while self.coro_keep_alive["recv"].is_alive:
            try:
                # data = await u.read3(self.reader)
                # self.last_read = data
                data = await self.reader.read(200)
                backBuffer.extend(data)

                while self._terminator in backBuffer:
                    self.last_read, backBuffer = backBuffer.split(
                        self._terminator, 1
                    )

                await asyncio.sleep(self.coro_keep_alive["recv"].sleep_delay)
            except Exception as e:
                print(f"recv failed: {e}")
                self.coro_keep_alive["recv"].is_alive = False
                break

    async def _cli_arg_map(self, pair):
        ''':pair: is a pair of (key, value) from a dict returned by docopt command parse, override this if you want, it has to be async thread safe'''
        pass

    async def close(self):
        """Await close thread, also the thing that does cli"""

        await asyncio.sleep(1/2)

        print (self._CLI_SETTS.strip())

        while self.coro_keep_alive["close"].is_alive:
            ss = shlex.split(await ainput('> '))

            try:
                ret = docopt.docopt(self._CLI_SETTS, ss)
                if ret['--quit']:
                    break

                for i in ret.items():
                    await self._cli_arg_map(i)

            except:
                print (f'error parsing {ss} arguments\n{self._CLI_SETTS.strip()}')

            await asyncio.sleep(self.coro_keep_alive["close"].sleep_delay)

        print("closing...")
        self.coro_keep_alive["close"].is_alive = False
        for key in self.coro_keep_alive:
            if self.coro_keep_alive[key].is_alive:
                self.coro_keep_alive[key].is_alive = False
                print(f"{key} stop sent...")
            else:
                print(f"{key} already stopped")

        await asyncio.sleep(0.5)

        try:
            self.writer.write(u.format_str_for_write("CLOSE"))
            self.writer.close()
            await self.writer.wait_closed()

        except Exception as e:
            print(f"failed to close connection: {e}")

        print("finished")

    async def main(self):
        """
        Run the main async method.

        Gathers all recognized threads and runs them.
        It is not recommended you override this method.
        """
        await self._socket_init()

        await asyncio.gather(
            *[
                getattr(self, coro_name)()
                for coro_name in self.coro_list
                if coro_name not in self._coro_name_exceptions
            ]
        )

    @staticmethod
    def register_member_thread(sleepDelay, runInDefaultExecutor=False):
        """
        Registers thread functions, should only be used on member functions

        sleepDelay - sleep delay in seconds
        runInDefaultExecutor - bool, set True if you want the function to be executed in asyncio's default pool executor
        """

        def _thread_reg(func):
            def _thread_reg_wrapper(self, *args, **kwargs):

                if not asyncio.iscoroutinefunction(func) and not runInDefaultExecutor:
                    raise ValueError(
                        f"{repr(func)} is not a coroutine function and runInDefaultExecutor is set to False"
                    )

                if (
                    func.__name__ not in self.coro_keep_alive
                    and func.__name__ in self.coro_list
                    and func.__name__ not in self._coro_name_exceptions
                ):
                    self.coro_keep_alive[func.__name__] = KeepAliveTrigger(
                        True, sleepDelay
                    )

                elif func.__name__ in self._coro_name_exceptions:
                    warnings.warn(
                        "thread register ignored, thread name is in balck list",
                        RuntimeWarning,
                    )
                    return func(self, *args, **kwargs)

                else:
                    raise NameError(
                        f"trying to register existing thread, thread with name {repr(func.__name__)} already exists"
                    )

                if runInDefaultExecutor:
                    loop = asyncio.get_running_loop()

                    return loop.run_in_executor(None, func, self, *args, **kwargs)

                return func(self, *args, **kwargs)

            setattr(_thread_reg_wrapper, "__name__", func.__name__)

            return _thread_reg_wrapper

        return _thread_reg


class PoserClientBase(PoserTemplateBase):
    """
    poser client base class
    should only be inherited together with one of the poser templates
    should only be used to create new poser clients
    """

    def __init__(self, *args, **kwargs):
        """init"""
        super().__init__(*args, **kwargs)
        self._coro_name_exceptions.append("thread_register")

    def thread_register(self, sleep_delay, runInDefaultExecutor=False):
        """
        Registers a thread for the client

        sleepDelay - sleep delay in seconds
        runInDefaultExecutor - bool, set True if you want the function to be executed in asyncio's default pool executor
        """

        def _thread_register(coro):
            if not asyncio.iscoroutinefunction(coro) and not runInDefaultExecutor:
                raise ValueError(
                    f"{repr(coro)} is not a coroutine function and runInDefaultExecutor is set to False"
                )

            if (
                coro.__name__ not in self.coro_keep_alive
                and coro.__name__ not in self.coro_list
            ):
                self.coro_keep_alive[coro.__name__] = KeepAliveTrigger(
                    True, sleep_delay
                )
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

'''base template class'''
import asyncio
import numbers
import warnings

from ..util import utilz as u


class PoserTemplateBase(object):
    """
    Base template class, should only be used to create new poser templates
    """

    def __init__(
        self, *, addr="127.0.0.1", port=6969, send_delay=1 / 100, recv_delay=1 / 1000, **kwargs,
    ):
        """
        :param addr: is the address of the server to connect to, stored in self.addr
        :param port: is the port of the server to connect to, stored in self.port
        :param send_delay: sleep delay for the self.send thread(in seconds)
        :param recv_delay: sleep delay for the self.recv thread(in seconds)
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
            "close": [True, 0.1],
            "send": [True, send_delay],
            "recv": [True, recv_delay],
        }
        self.last_read = ""
        self.id_message = 'holla'

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
        raise NotImplementedError('please implement the send thread to send pose packets')

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

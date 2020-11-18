"""Server loop that communicates between the driver and posers."""
import asyncio

from .__init__ import __version__

DOMAIN = (None, 6969)


class Server:
    def __init__(self):
        """
        im not sure about this on chief,
        looks like a destructor to me
        """
        self.conz = []
        self.driver_conz = []
        self.poser_conz = []
        self.debug = False

        self._driver_idz = [
            b"hello",
        ]
        self._poser_idz = [
            b"holla",
        ]
        self._terminator = b"\n"
        self._close_msg = b"CLOSE\n"
        self._read_size = 400

    def __repr__(self):
        """do i need to explain this?"""
        return f"<{self.__class__.__module__}.{self.__class__.__name__} debug={self.debug} active_conz={len(self.conz)} object at {hex(id(self))}>"

    async def send_to_all(self, msg, me):
        """send a message to all registered connections that are not self"""
        try:
            for i in self.conz:
                if i != me:
                    i[1].write(msg)
                    await i[1].drain()

        except Exception as e:
            print(f"message from {me[0]} lost: {e}")

    async def send_to_all_poser(self, msg, me):
        """send a message to all registered connections that are not self, for poser messages only"""
        try:
            for i in self.driver_conz:
                i[1].write(msg)
                await i[1].drain()

        except Exception as e:
            print(f"message from {me[0]} poser lost: {e}")

    async def send_to_all_driver(self, msg, me):
        """send a message to all registered connections that are not self, for driver messages only"""
        try:
            for i in self.poser_conz:
                i[1].write(msg)
                await i[1].drain()

        except Exception as e:
            print(f"message from {me[0]} driver lost: {e}")

    async def __call__(self, reader, writer):
        """this is will run for each incoming connection"""

        addr = writer.get_extra_info("peername")
        try:
            first_msg = await reader.read(50)

        except Exception as e:
            print (f'pipe {addr} broke on id, reason: {e}')

        if self._terminator in first_msg:
            id_msg, first_msg = first_msg.split(self._terminator, 1)

        else:
            id_msg = b""

        me = (
            addr,
            writer,
            f"lol{len(self.conz)}",
        )

        print(f"new connection from {addr}")
        whatAmI = 0
        if id_msg in self._driver_idz and me not in self.driver_conz:
            whatAmI = 1
            self.driver_conz.append(me)

        elif id_msg in self._poser_idz and me not in self.poser_conz:
            whatAmI = 2
            self.poser_conz.append(me)

        elif me not in self.conz:
            whatAmI = 3
            self.conz.append(me)

        if first_msg:
            await self.send_to_all(first_msg, me)

        # this is does nothing but looks pretty
        if id_msg in self._driver_idz:
            print("its a driver")

        elif id_msg in self._poser_idz:
            print("its a poser")

        # main receive/transmit loop
        if whatAmI == 1:
            while 1:
                try:
                    data = await reader.read(self._read_size)

                    if not data or self._close_msg in data:
                        break

                    await self.send_to_all_driver(data, me) # send to all posers

                    if self.debug:
                        print(f"{repr(data)} from {addr}")

                except Exception as e:
                    print(f"driver {addr} broke: {e}")
                    break

        elif whatAmI == 2:
            while 1:
                try:
                    data = await reader.read(self._read_size)

                    if not data or self._close_msg in data:
                        break

                    await self.send_to_all_poser(data, me) # send to all drivers

                    if self.debug:
                        print(f"{repr(data)} from {addr}")

                except Exception as e:
                    print(f"poser {addr} broke: {e}")
                    break

        elif whatAmI == 3:
            while 1:
                try:
                    data = await reader.read(self._read_size)

                    if not data or self._close_msg in data:
                        break

                    await self.send_to_all_driver(data, me) # send to posers too, prioritize posers tho
                    await self.send_to_all(data, me) # send to all not identified connections

                    if self.debug:
                        print(f"{repr(data)} from {addr}")

                except Exception as e:
                    print(f"{addr} broke: {e}")
                    break

        if me in self.conz:
            self.conz.remove(me)

        if me in self.driver_conz:
            self.driver_conz.remove(me)

        if me in self.poser_conz:
            self.poser_conz.remove(me)

        try:
            writer.close()
            await writer.wait_closed()
        except Exception as e:
            print(f"error on {addr} close: {e}")

        print(f"connection to {addr} closed")


def run_til_dead(poser = None, conn_handle=Server()):
    """Run the server until it dies."""
    loop = asyncio.get_event_loop()
    coro = asyncio.start_server(conn_handle, *DOMAIN, loop=loop)
    server = loop.run_until_complete(coro)

    if poser is not None:
        poser_result = asyncio.run_coroutine_threadsafe(poser.main(), loop)

    # Serve requests until Ctrl+C is pressed
    print (f"server version: {repr(__version__)}")
    print("Serving on {}".format(server.sockets[0].getsockname()))
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass

    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()

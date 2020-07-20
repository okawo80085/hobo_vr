"""Server loop that communicates between the driver and posers."""
import asyncio
import logging

logger = logging.getLogger(__name__)

from ..templates import PoserTemplate
from ..util import utilz as u

DOMAIN = (None, 6969)
conz = {}

PRINT_MESSAGES = False


async def broadcast(everyone, data, me, VIP):
    """
    Broadcast a message to all trackers.

    :param everyone:
    :param data:
    :param me:
    :param VIP:
    :return:
    """
    for key, acc in everyone.items():
        try:
            if me != key and (VIP == acc[2] or acc[3]):
                acc[1].write(data)
                await acc[1].drain()

        except:
            # print (key, 'reason:', e)
            pass

    return True


async def handle_echo(reader, writer):
    """Handle communication between poser and driver."""
    addr = writer.get_extra_info("peername")
    while 1:
        try:

            data = await u.read3(reader)
            if addr not in conz:
                logger.info("New connection from {}".format(addr))
                isDriver = False
                isPoser = False
                if data == b"hello\n":
                    logger.info("driver connected")
                    isDriver = True

                if data == b"poser here\n":
                    logger.info("poser connected")
                    isPoser = True

                conz[addr] = (reader, writer, isDriver or isPoser, isPoser)

            if not len(data) or data == b"CLOSE\n":
                break

            sendOK = await broadcast(conz, data, addr, conz[addr][2])

            logger.debug("Received %r from %r %r" % (data, addr, sendOK))

            await asyncio.sleep(0.00001)

        except Exception as e:
            logger.error("Losing connection to {}, reason: {}".format(addr, e))
            break

    logging.info(f"Connection to {addr} closed")
    writer.close()
    if addr in conz:
        del conz[addr]


def run_til_dead(poser: PoserTemplate = None):
    """Run the server until it dies."""
    loop = asyncio.get_event_loop()
    coro = asyncio.start_server(handle_echo, *DOMAIN, loop=loop)
    server = loop.run_until_complete(coro)

    if poser is not None:
        poser_result = asyncio.run_coroutine_threadsafe(poser.main(), loop)

    # Serve requests until Ctrl+C is pressed
    logger.info("Serving on {}".format(server.sockets[0].getsockname()))
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass

    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()

import asyncio
from .. import utilz as u

DOMAIN = (None, 6969)
conz = {}


async def send2all(everyone, data, me, VIP):
    for key, acc in everyone.items():
        try:
            if me != key and (VIP == acc[2] or acc[3]):
                acc[1].write(u.convv(data))
                await acc[1].drain()

        except:
            # print (key, 'reason:', e)
            pass

    return True


async def handle_echo(reader, writer):
    while 1:
        try:
            addr = writer.get_extra_info("peername")

            data = await u.newRead(reader)
            if addr not in conz:
                print("New connection from {}".format(addr))
                isDriver = False
                isPoser = False
                if data == "hello\n":
                    print("driver connected")
                    isDriver = True

                if data == "poser here\n":
                    print("poser connected")
                    isPoser = True

                conz[addr] = (reader, writer, isDriver or isPoser, isPoser)

            if not len(data) or data == "CLOSE\n":
                break

            print("Received %r from %r" % (data, addr), end=" ")
            # print("Send: %r" % data)
            sendOK = await send2all(conz, data, addr, conz[addr][2])
            print(sendOK)

            await asyncio.sleep(0.00001)

        except Exception as e:
            print("Losing connection to {}, reason: {}".format(addr, e))
            break

    print("Close the client socket")
    writer.close()
    del conz[addr]


def run_til_dead():
    loop = asyncio.get_event_loop()
    coro = asyncio.start_server(handle_echo, *DOMAIN, loop=loop)
    server = loop.run_until_complete(coro)

    # Serve requests until Ctrl+C is pressed
    print("Serving on {}".format(server.sockets[0].getsockname()))
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass

    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()

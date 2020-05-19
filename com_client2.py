import asyncio
import virtualreality.util.utilz as u

addrs = ("127.0.0.1", 6969)

myTasks = []


async def tcp_echo_client(message, loop):
    reader, writer = await asyncio.open_connection(*addrs, loop=loop)

    # print('Send: %r' % message)
    writer.write(u.format_str_for_write("nut"))

    data = await u.read(reader)
    print("Received: %r" % data)

    print("Close the socket")
    writer.write(u.format_str_for_write("CLOSE"))
    writer.close()


message = "Hello World!\n"
loop = asyncio.get_event_loop()
loop.run_until_complete(tcp_echo_client(message, loop))
loop.close()

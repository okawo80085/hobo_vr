import asyncio
import virtualreality.utilz as u
import random

addrs = ("127.0.0.1", 6969)


async def tcp_echo_client(message):
    reader, writer = await asyncio.open_connection(*addrs, loop=asyncio.get_event_loop())

    for _ in range(10):
        x = random.random()
        x = round(x, 4)
        x /= 10
        print("Send: %r" % message.format(x))
        await asyncio.sleep(1 / 60)
        writer.write(u.format_str_for_write(message.format(x)))

    print("Close the socket")
    writer.write(u.format_str_for_write("CLOSE"))
    writer.close()


message = "{} 1 0 1 0 0 0"  # posX posY posZ rotW rotX rotY rotZ
asyncio.run(tcp_echo_client(message))

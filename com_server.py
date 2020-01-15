import asyncio
import utilz as u

DOMAIN = (None, 6969)
conz = {}

async def send2all(everyone, data, me):
	for key, acc in everyone.items():
		try:
			if me != key:
				acc[1].write(u.convv(data))
				await acc[1].drain()

		except:
			# print (key, 'reason:', e)
			pass

	return True


async def handle_echo(reader, writer):
	while 1:
		try:
			data = await u.newRead(reader)

			addr = writer.get_extra_info('peername')
			if addr not in conz:
				print ('New connection from {}'.format(addr))
				conz[addr] = (reader, writer)

			if not len(data) or data == 'CLOSE\n':
				break

			print("Received %r from %r" % (data, addr), end=' ')
			# print("Send: %r" % data)
			sendOK = await send2all(conz, data, addr)
			print (sendOK)

		except Exception as e:
			print ('Losing connection to {}, reason: {}'.format(addr, e))
			break

	print("Close the client socket")
	writer.close()
	del conz[addr]

loop = asyncio.get_event_loop()
coro = asyncio.start_server(handle_echo, None, 6969, loop=loop)
server = loop.run_until_complete(coro)

# Serve requests until Ctrl+C is pressed
print('Serving on {}'.format(server.sockets[0].getsockname()))
try:
	loop.run_forever()
except KeyboardInterrupt:
	pass

# Close the server
server.close()
loop.run_until_complete(server.wait_closed())
loop.close()
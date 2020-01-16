import asyncio
import utilz as u
import time
from win32 import winxpgui
import math as m
import keyboard

class Poser:
	def __init__(self, addr='127.0.0.1', port=6969):
		self.pose = {
			'x':0,	# left/right
			'y':1,	# up/down
			'z':0,	# forwards/backwards
			'rW':1,
			'rX':0,
			'rY':0,
			'rZ':0,
		}

		self.ypr = {'yaw':0, 'pitch':0, 'roll':0} # yaw is real roll, pitch is real yaw, roll is real pitch
		self._send = True
		self._track = True
		self._trackDelay = 0.008 # should be less than 0.3
		self._sendDelay = 0.01 # should be less than 0.5
		self._tasks = []

		self.moveStep = 0.05

		self.addr = addr
		self.port = port

	async def _socketInit(self):
		self.reader, self.writer = await asyncio.open_connection(self.addr, self.port,
												   loop=asyncio.get_event_loop())

	async def close(self):
		while 1:
			await asyncio.sleep(1)
			try:
				if keyboard.is_pressed('q'):
					break

			except:
				break

		print ('closing...')
		self._send = False
		self._track = False
		self.writer.write(u.convv('CLOSE'))
		self.writer.close()

		# for i in self._tasks:
		# 	if not i.done() or not i.cancelled():
		# 		i.cancel()

	async def send(self):
		while self._send:
			try:
				msg = u.convv(' '.join([str(i) for _, i in self.pose.items()]))
				self.writer.write(msg)
				await asyncio.sleep(self._sendDelay)

			except:
				self._send = False
				break

	async def getPose(self):
		while self._track:
			try:
				x, y = winxpgui.GetCursorPos()

				self.ypr['pitch'] = ((960 - x)/960) * m.pi
				self.ypr['roll'] = ((960 - y)/960) * m.pi

				t0 = m.cos(self.ypr['yaw'])
				t1 = m.sin(self.ypr['yaw'])
				t2 = m.cos(self.ypr['roll'])
				t3 = m.sin(self.ypr['roll'])
				t4 = m.cos(self.ypr['pitch'])
				t5 = m.sin(self.ypr['pitch'])

				self.pose['rW'] = round(t0 * t2 * t4 + t1 * t3 * t5, 9)
				self.pose['rX'] = round(t0 * t3 * t4 - t1 * t2 * t5, 9)
				self.pose['rY'] = round(t0 * t2 * t5 + t1 * t3 * t4, 9)
				self.pose['rZ'] = round(t1 * t2 * t4 - t0 * t3 * t5, 9)

				await asyncio.sleep(self._trackDelay)

				if keyboard.is_pressed('4'):
					self.pose['x'] -= self.moveStep
				elif keyboard.is_pressed('6'):
					self.pose['x'] += self.moveStep


				if keyboard.is_pressed('8'):
					self.pose['z'] += self.moveStep
				elif keyboard.is_pressed('5'):
					self.pose['z'] -= self.moveStep


				if keyboard.is_pressed('3'):
					self.pose['y'] += self.moveStep
				elif keyboard.is_pressed('2'):
					self.pose['y'] -= self.moveStep

				if keyboard.is_pressed('9'):
					self.pose['x'] = 0
					self.pose['y'] = 1
					self.pose['z'] = 0

			except:
				self._track = False
				break

	async def main(self):
		await self._socketInit()

		await asyncio.gather(
				self.send(),
				self.getPose(),
				self.close(),
			)



t = Poser()

asyncio.run(t.main())
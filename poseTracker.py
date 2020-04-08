import asyncio
import utilz as u
import time
# from win32 import winxpgui
import math as m
import keyboard

import imutils
import cv2
import numpy as np
from imutils.video import VideoStream
import random
import serial

def swapByOffset(a, b, offset):
	offset = abs(offset)
	return a - a*offset + b*offset, b - b*offset + a*offset

def apply_noise(dicT):
	dicT['x'] = round(dicT['x'] + round(random.randint(-10, 10)*0.0001, 4), 5)
	dicT['y'] = round(dicT['y'] + round(random.randint(-10, 10)*0.0001, 4), 5)
	dicT['z'] = round(dicT['z'] + round(random.randint(-10, 10)*0.0001, 4), 5)
	dicT['rW'] = round(dicT['rW'] + round(random.randint(-10, 10)*0.0001, 4), 5)
	dicT['rX'] = round(dicT['rX'] + round(random.randint(-10, 10)*0.0001, 4), 5)
	dicT['rY'] = round(dicT['rY'] + round(random.randint(-10, 10)*0.0001, 4), 5)
	dicT['rZ'] = round(dicT['rZ'] + round(random.randint(-10, 10)*0.0001, 4), 5)

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

		self.poseControllerR = {
			'x':0.5,	# left/right
			'y':1,	# up/down
			'z':-1,	# forwards/backwards
			'rW':1,
			'rX':0,
			'rY':0,
			'rZ':0,
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,
			'triggerClick':0,
		}
		self.poseControllerL = {
			'x':0.5,	# left/right
			'y':1.1,	# up/down
			'z':-1,	# forwards/backwards
			'rW':1,
			'rX':0,
			'rY':0,
			'rZ':0,
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,
			'triggerClick':0,
		}

		self.tempPose = {
			'x':0,	# left/right
			'y':1,	# up/down
			'z':0,	# forwards/backwards
			'rW':1,
			'rX':0,
			'rY':0,
			'rZ':0,
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,
			'triggerClick':0,
		}

		self.incomingData_readonly = ''

		self.serialPaths = {'blue':'/dev/ttyUSB1', 'green': '/dev/ttyUSB0'}

		self.mode = 0

		self.ypr = {'roll':0, 'yaw':0, 'pitch':0} # yaw is real roll, pitch is real yaw, roll is real pitch

		self._send = True
		self._recv = True
		self._listen = True
		self._keyListen = True
		self._serialListen = True
		self._retrySerial = True

		self._recvDelay = 1/1000
		self._trackDelay = 1/60
		self._yprListenDelay = 0.006
		self._sendDelay = 1/60
		self._keyListenDelay = 0.01
		self._serialListenDelay = 1/300 # don't change this
  
		try:
			self.t1 = u.Tracker()#offsets={'translation':[0, 0, 0], 'rotation':[0.6981317007977318, 0, 0]})

			self.t1.markerMasks = {
				'blue':{
					'h':(98, 10),
					's':(200, 55),
					'v':(250, 32)
						},
				'green':{
					'h':(66, 15),
					's':(90, 55),
					'v':(250, 50)
					}
				}

			self.t1._reinit()

		except:
			print ('failed to init location tracker')
			self.t1 = None

		self._track = True if self.t1 is not None else False

		self.addr = addr
		self.port = port

		self.readBuffer = ''

	async def _socketInit(self):
		self.reader, self.writer = await asyncio.open_connection(self.addr, self.port,
												   loop=asyncio.get_event_loop())

		self.writer.write(u.convv('poser here'))

	async def close(self):
		while 1:
			await asyncio.sleep(2)
			try:
				if keyboard.is_pressed('q'):
					break

			except:
				break

		print ('closing...')
		self._send = False
		self._recv = False
		self._track = False
		self._listen = False
		self._keyListen = False
		self._serialListen = False
		self._retrySerial = False

		await asyncio.sleep(1)

		self.writer.write(u.convv('CLOSE'))
		self.writer.close()
		self.t1.close()

	async def send(self):
		while self._send:
			try:

				temp = self.poseControllerR if self.mode == 1 else self.poseControllerL

				temp['grip'] = self.tempPose['grip']
				temp['system'] = self.tempPose['system']
				temp['menu'] = self.tempPose['menu']
				temp['triggerValue'] = self.tempPose['triggerValue']
				temp['trackpadClick'] = self.tempPose['trackpadClick']
				temp['trackpadClick'] = self.tempPose['trackpadClick']
				temp['trackpadX'] = self.tempPose['trackpadX']
				temp['trackpadY'] = self.tempPose['trackpadY']
				temp['trackpadClick'] = self.tempPose['trackpadClick']
				temp['trackpadTouch'] = self.tempPose['trackpadTouch']
				temp['triggerClick'] = self.tempPose['triggerClick']

				# if self.mode != 0:
				temp['x'] = self.tempPose['x']
				temp['y'] = self.tempPose['y']
				temp['z'] = self.tempPose['z']

				temp['rW'] = self.tempPose['rW']
				temp['rX'] = self.tempPose['rX']
				temp['rY'] = self.tempPose['rY']
				temp['rZ'] = self.tempPose['rZ']

				# apply_noise(self.pose)
				# apply_noise(self.poseControllerL)
				# apply_noise(self.poseControllerR)

				msg = u.convv(' '.join([str(i) for _, i in self.pose.items()] + [str(i) for _, i in self.poseControllerR.items()] + [str(i) for _, i in self.poseControllerL.items()]))
				# msg = u.convv(' '.join([str(random.random()) for _ in range(35)]))
				self.writer.write(msg)

				await asyncio.sleep(self._sendDelay)

			except:
				self._send = False
				break
		print (f'{self.send.__name__} stop')

	async def getLocation(self):
		while self._track:
			try:
				# a = time.time()
				self.t1.getFrame()
				self.t1.solvePose()

				u.rotateX(self.t1.poses, 0.6981317007977318)
				# u.rotateY(self.t1.poses, -1.0471975511965976)


				self.tempPose['x'] = round(-self.t1.poses['green']['x'], 6)
				self.tempPose['y'] = round(self.t1.poses['green']['y'] + 1, 6)
				self.tempPose['z'] = round(self.t1.poses['green']['z'], 6)


				self.pose['x'] = round(-self.t1.poses['blue']['x'] - 0.01, 6)
				self.pose['y'] = round(self.t1.poses['blue']['y'] + 1 - 0.07, 6)
				self.pose['z'] = round(self.t1.poses['blue']['z'] - 0.03, 6)

				# print (f'{time.time() - a}s/{self._trackDelay}')

				# self.pose['x'] = round(-self.t1.poses['red']['x'], 6)
				# self.pose['y'] = round(self.t1.poses['red']['y'] + 1, 6)
				# self.pose['z'] = round(self.t1.poses['red']['z'], 6)

				await asyncio.sleep(self._trackDelay)

			except Exception as e:
				print ('stopping getLocation:', e)
				self._track = False
				break
		print (f'{self.getLocation.__name__} stop')

	async def serialListener2(self):
		while self._retrySerial:
			try:
				self._serialListen = True

				with serial.Serial(self.serialPaths['blue'], 115200, timeout=0) as ser:
					startTime = time.time()


					while self._serialListen:
						try:
							if 2 < time.time() - startTime < 4:
								ser.write(b'nut\n')


							respB = ser.readline()

							if len(respB) > 5:
								rrr = u.getOnlyNums(respB.decode())

								if rrr != [0, 0, 0, 0]:
									self.tempPose['rW'] = rrr[0]
									self.tempPose['rX'] = -rrr[1]
									self.tempPose['rY'] = rrr[2]
									self.tempPose['rZ'] = -rrr[3]

								try:
									if len(rrr) > 4:
										self.tempPose['triggerValue'] = int(abs(rrr[4] - 1))
										if len(rrr) > 6:
											self.tempPose['grip'] = int(abs(rrr[5] - 1))

											tempMode = int(abs(rrr[6] - 1))

										if len(rrr) > 8:
											self.tempPose['system'] = int(abs(rrr[7] - 1))
											self.tempPose['menu'] = int(abs(rrr[8] - 1))
											self.tempPose['trackpadClick'] = int(abs(rrr[9] - 1))

										if len(rrr) > 10:
											self.tempPose['trackpadY'] = round((rrr[10] - 524)/530, 3)*(-1)
											self.tempPose['trackpadX'] = round((rrr[11] - 506)/512, 3)*(-1)

										if abs(self.tempPose['trackpadX']) > 0.07 or abs(self.tempPose['trackpadY']) > 0.07:
											self.tempPose['trackpadTouch'] = 1

										else:
											self.tempPose['trackpadTouch'] = 0

										if self.tempPose['trackpadX'] > 0.6 and tempMode:
											self.mode = 1

										elif self.tempPose['trackpadX'] < -0.6 and tempMode:
											self.mode = 0

										elif tempMode:
											self._serialListen = False
								except IndexError:
										self.tempPose['triggerValue'] = 0

										self.tempPose['grip'] = 0

										self.tempPose['system'] = 0

										self.tempPose['menu'] = 0

										self.tempPose['trackpadClick'] = 0

										self.tempPose['trackpadY'] = 0

										self.tempPose['trackpadX'] = 0

							if self.tempPose['triggerValue'] > 0.9:
								self.tempPose['triggerClick'] = 1

							else:
								self.tempPose['triggerClick'] = 0

							await asyncio.sleep(self._serialListenDelay)

						except Exception as e:
							print (f'{self.serialListener2.__name__}: {e}')
							self._serialListen = False
							break

			except Exception as e:
				print (f'{self.serialListener2.__name__}: {e}')

			await asyncio.sleep(1)
		print (f'{self.serialListener2.__name__} stop')


	async def serialListener(self):
		while self._retrySerial:
			try:
				self._serialListen = True

				with serial.Serial(self.serialPaths['green'], 115200, timeout=0) as ser2:
					startTime = time.time()


					while self._serialListen:
						try:
							if 2 < time.time() - startTime < 4:
								ser2.write(b'nut\n')

							respG = ser2.readline()

							if len(respG) > 5:
								rrr2 = u.getOnlyNums(respG.decode())

								self.pose['rW'] = rrr2[0]
								self.pose['rX'] = -rrr2[2]
								self.pose['rY'] = rrr2[3]
								self.pose['rZ'] = -rrr2[1]

							await asyncio.sleep(self._serialListenDelay)

						except Exception as e:
							print (f'{self.serialListener.__name__}: {e}')
							self._serialListen = False
							break

			except Exception as e:
				print (f'{self.serialListener.__name__}: {e}')

			await asyncio.sleep(1)
		print (f'{self.serialListener.__name__} stop')


	async def keyListener(self):
		while self._keyListen:
			try:

				await asyncio.sleep(self._keyListenDelay)

			except:
				self._keyListen = False
				break
		print (f'{self.keyListener.__name__} stop')

	async def recv(self):
		while self._keyListen:
			try:
				data = await u.newRead(self.reader)
				self.incomingData_readonly = data
				print ([data])

				await asyncio.sleep(self._recvDelay)

			except:
				self._recv = False
				break
		print (f'{self.recv.__name__} stop')


	async def main(self):
		await self._socketInit()

		await asyncio.gather(
				self.send(),
				self.recv(),
				self.getLocation(),
				# self.keyListener(),
				self.serialListener(),
				self.serialListener2(),
				self.close(),
			)



t = Poser('192.168.31.60')

asyncio.run(t.main())
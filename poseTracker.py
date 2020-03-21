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

		self.serialPaths = {'red':'/dev/ttyUSB0'}

		self.mode = 0

		self.ypr = {'roll':0, 'yaw':0, 'pitch':0} # yaw is real roll, pitch is real yaw, roll is real pitch

		self._send = True
		self._read = True
		self._listen = True
		self._keyListen = True
		self._serialListen = True
		self._retrySerial = True

		self._trackDelay = 0.01# should be less than 0.3
		self._yprListenDelay = 0.006
		self._sendDelay = 1/19.5 # should be less than 0.5
		self._keyListenDelay = 0.01
		self._serialListenDelay = 0.0005 # don't change this

		try:
			self.t1 = u.Tracker()

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
		self._read = False
		self._track = False
		self._listen = False
		self._keyListen = False
		self._serialListen = False
		self._retrySerial = False

		await asyncio.sleep(1)

		self.writer.write(u.convv('CLOSE'))
		self.writer.close()
		self.t1.close()

		# for i in self._tasks:
		# 	if not i.done() or not i.cancelled():
		# 		i.cancel()

	# async def recv(self):
	# 	while self._read:
	# 		try:
	# 			self.readBuffer = await u.newRead(self.reader)

	# 			await asyncio.sleep(self._sendDelay)

	# 		except:
	# 			self._read = False
	# 			break

	async def send(self):
		while self._send:
			try:
				if self.mode == 0:
					temp = self.pose

				else:
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
				self.t1.getFrame()
				self.t1.solvePose()

				u.rotateX(self.t1.poses, 0.6981317007977318)
				u.rotateY(self.t1.poses, -1.0471975511965976)


				self.tempPose['x'] = round(-self.t1.poses['blue']['x'], 6)
				self.tempPose['y'] = round(self.t1.poses['blue']['y'] + 1, 6)
				self.tempPose['z'] = round(self.t1.poses['blue']['z'], 6)

				# self.pose['x'] = round(-self.t1.poses['red']['x'], 6)
				# self.pose['y'] = round(self.t1.poses['red']['y'] + 1, 6)
				# self.pose['z'] = round(self.t1.poses['red']['z'], 6)

				await asyncio.sleep(self._trackDelay)

			except Exception as e:
				print ('stopping getLocation:', e)
				self._track = False
				break
		print (f'{self.getLocation.__name__} stop')

	async def yprListener(self):
		while self._listen:
			try:
				data = await u.newRead(self.reader)
				if not u.hasCharsInString(data.lower(), 'aqzwsxedcrfvtgbyhnujmikolp[]{}'):
					x, y, z = [float(i) for i in data.strip('\n').strip('\r').split(',')]

					# x -= 0.5
					# y += 0.5
					# z -= 0.5

					x, y = swapByOffset(x, y, z)

					self.ypr['roll'] = (z * m.pi)	# yaw
					self.ypr['yaw'] = (y * m.pi) * (-1)	# roll
					self.ypr['pitch'] = (x * m.pi)	#pitch

					# print (self.ypr)

					# for key, val in self.ypr.items():
					# 	if val < 0:
					# 		self.ypr[key] = 2*m.pi - abs(val)


					# print (quaternion2ypr(self.pose['rW'], self.pose['rX'], self.pose['rY'], self.pose['rZ']))

					cy = np.cos(self.ypr['roll'] * 0.5)
					sy = np.sin(self.ypr['roll'] * 0.5)
					cp = np.cos(self.ypr['pitch'] * 0.5)
					sp = np.sin(self.ypr['pitch'] * 0.5)
					cr = np.cos(self.ypr['yaw'] * 0.5)
					sr = np.sin(self.ypr['yaw'] * 0.5)

					self.tempPose['rW'] = round(cy * cp * cr + sy * sp * sr, 4)
					self.tempPose['rZ'] = round(cy * cp * sr - sy * sp * cr, 4)
					self.tempPose['rX'] = round(sy * cp * sr + cy * sp * cr, 4)
					self.tempPose['rY'] = round(sy * cp * cr - cy * sp * sr, 4)

				elif data == 'driver:buzz\n':
					print (data)

				await asyncio.sleep(self._yprListenDelay)

			except Exception as e:
				print ('stopping yprListener:',e)
				self._listen = False
				break
		print (f'{self.yprListener.__name__} stop')

	async def serialListener(self):
		while self._retrySerial:
			try:
				with serial.Serial(self.serialPaths['red'], 115200, timeout=0) as ser:
					startTime = time.time()

					while self._serialListen:
						try:
							if 2 < time.time() - startTime < 4:
								ser.write(b'nut\n')

							resp = ser.readline()

							rrr = u.getOnlyNums(resp.decode())

							self.tempPose['rW'] = rrr[0]
							self.tempPose['rX'] = rrr[1]
							self.tempPose['rY'] = rrr[3]
							self.tempPose['rZ'] = -rrr[2]

							if keyboard.is_pressed('r'):
								break

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
				self.tempPose['grip'] = 1 if keyboard.is_pressed('1') else 0
				self.tempPose['menu'] = 1 if keyboard.is_pressed('2') else 0
				self.tempPose['system'] = 1 if keyboard.is_pressed('3') else 0
				self.tempPose['triggerValue'] = 1 if keyboard.is_pressed('4') else 0
				self.tempPose['trackpadClick'] = 1 if keyboard.is_pressed('ctrl') else 0


				if self.tempPose['triggerValue'] > 0.9:
					self.tempPose['triggerClick'] = 1

				else:
					self.tempPose['triggerClick'] = 0


				self.tempPose['trackpadTouch'] = 0

				self.tempPose['trackpadX'] = 0
				if keyboard.is_pressed('right arrow'):
					self.tempPose['trackpadX'] = 0.8
					self.tempPose['trackpadTouch'] = 1
				elif keyboard.is_pressed('left arrow'):
					self.tempPose['trackpadX'] = -0.8
					self.tempPose['trackpadTouch'] = 1

				self.tempPose['trackpadY'] = 0
				if keyboard.is_pressed('up arrow'):
					self.tempPose['trackpadY'] = 0.8
					self.tempPose['trackpadTouch'] = 1
				elif keyboard.is_pressed('down arrow'):
					self.tempPose['trackpadY'] = -0.8
					self.tempPose['trackpadTouch'] = 1

				if keyboard.is_pressed('k'):
					self.mode = 0

				elif keyboard.is_pressed('j'):
					self.mode = 2

				elif keyboard.is_pressed('l'):
					self.mode = 1
 
				if keyboard.is_pressed('7'):
					self.tempPose['x'] = 0
					self.tempPose['y'] = 1
					self.tempPose['z'] = 0

				await asyncio.sleep(self._keyListenDelay)

			except:
				self._keyListen = False
				break
		print (f'{self.keyListener.__name__} stop')

	async def main(self):
		await self._socketInit()

		await asyncio.gather(
				self.send(),
				# self.recv(),
				# self.getLocation(),
				# self.yprListener(),
				self.keyListener(),
				# self.serialListener(),
				self.close(),
			)



t = Poser()

asyncio.run(t.main())
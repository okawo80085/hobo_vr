import asyncio
import pyVr.utilz as u
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
import squaternion as sq

class Poser:
	def __init__(self, addr='127.0.0.1', port=6969):
		self.pose = {
			# location in meters and orientation in quaternion
			'x':0,	# +(x) is right in meters
			'y':1,	# +(y) is up in meters
			'z':0,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,
		}

		self.poseControllerR = {
			# location in meters and orientation in quaternion
			'x':0.5,	# +(x) is right in meters
			'y':1,	# +(y) is up in meters
			'z':-1,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,

			# inputs
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,	# 0 or 1
			'triggerClick':0,	# 0 or 1
		}

		self.poseControllerL = {
			# location in meters and orientation in quaternion
			'x':0.5,	# +(x) is right in meters
			'y':1.1,	# +(y) is up in meters
			'z':-1,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,

			# inputs
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,	# 0 or 1
			'triggerClick':0,	# 0 or 1
		}

		self.tempPose = {
			# location in meters and orientation in quaternion
			'x':0.5,	# +(x) is right in meters
			'y':1.1,	# +(y) is up in meters
			'z':-1,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,

			# inputs
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,	# 0 or 1
			'triggerClick':0,	# 0 or 1
		}

		self.incomingData_readonly = ''

		self.serialPaths = {'blue':'/dev/ttyUSB1', 'green': '/dev/ttyUSB0'}

		self.mode = 0

		self.ypr = {'roll':0, 'yaw':0, 'pitch':0} # yaw is real roll, pitch is real yaw, roll is real pitch

		self._send = True
		self._recv = True
		self._listen = True
		self._retrySerial = True
		self._serialResetYaw = False

		self._recvDelay = 1/1000
		self._trackDelay = 1/60
		self._yprListenDelay = 0.006
		self._sendDelay = 1/60
		self._serialListenDelay = 1/1000 # don't change this
  
		try:
			self.t1 = u.Tracker()#offsets={'translation':[0, 0, 0], 'rotation':[0.6981317007977318, 0, 0]})

			self.t1.markerMasks = {
				'blue':{
					'h':(98, 10),
					's':(240, 55),
					'v':(250, 32)
						},
				'green':{
					'h':(68, 15),
					's':(135, 53),
					'v':(255, 50)
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

				temp['velX'] = self.tempPose['velX']
				temp['velY'] = self.tempPose['velY']
				temp['velZ'] = self.tempPose['velZ']


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
				a = time.time()
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

				slepDel = self._trackDelay - time.time()

				if slepDel < 0.0001:
					slepDel = 0.0001

				await asyncio.sleep(slepDel)

			except Exception as e:
				print ('stopping getLocation:', e)
				self._track = False
				break
		print (f'{self.getLocation.__name__} stop')

	def serialListener2(self):
		pastVelocity = [0, 0, 0]
		velocityUntilReset = 0
		tempForOffz = {'x':0, 'y':0, 'z':0}
		while self._retrySerial:
			try:
				yawOffset = 0
				with serial.Serial(self.serialPaths['blue'], 115200, timeout=1/4) as ser:
					startTime = time.time()

					while self._retrySerial:
						try:
							if 2 < time.time() - startTime < 4:
								ser.write(b'nut\n')


							respB = ser.readline()

							gg = u.decodeSerial(respB)
							if len(gg) > 0:
								y, p, r, az, ax, ay, trgr, grp, util, sys, menu, padClk, padY, padX = gg

								if velocityUntilReset < 60:

									u.rotateY({'':tempForOffz}, u.angle2rad(-yawOffset))
									pastVelocity = [tempForOffz['x'], tempForOffz['y'], tempForOffz['z']]
									tempForOffz['x'] = round(pastVelocity[0] - ax*self._serialListenDelay * 0.01, 4)
									tempForOffz['y'] = round(pastVelocity[1] - ay*self._serialListenDelay * 0.01, 4)
									tempForOffz['z'] = round(pastVelocity[2] - az*self._serialListenDelay * 0.01, 4)
									u.rotateY({'':tempForOffz}, u.angle2rad(yawOffset))

									self.tempPose['velX'] = tempForOffz['x']
									self.tempPose['velY'] = tempForOffz['y']
									self.tempPose['velZ'] = tempForOffz['z']

									velocityUntilReset += 1

								else:
									velocityUntilReset = 0
									tempForOffz = {'x':0, 'y':0, 'z':0}
									self.tempPose['velX'] = 0
									self.tempPose['velY'] = 0
									self.tempPose['velZ'] = 0

								yaw = y
								w, x, y, z = sq.euler2quat(y + yawOffset, p, r, degrees=True)

								self.tempPose['rW'] = round(w, 4)
								self.tempPose['rX'] = round(y, 4)
								self.tempPose['rY'] = round(-x, 4)
								self.tempPose['rZ'] = round(-z, 4)

								self.tempPose['triggerValue'] = abs(trgr - 1)
								self.tempPose['grip'] = abs(grp - 1)
								self.tempPose['menu'] = abs(menu - 1)
								self.tempPose['system'] = abs(sys - 1)
								self.tempPose['trackpadClick'] = abs(padClk - 1)

								self.tempPose['trackpadX'] = round((padX - 524)/530, 3) * (-1)
								self.tempPose['trackpadY'] = round((padY - 506)/512, 3) * (-1)

								tempMode = abs(util - 1)

								self._serialResetYaw = False
								if self.tempPose['trackpadX'] > 0.6 and tempMode:
									self.mode = 1

								elif self.tempPose['trackpadX'] < -0.6 and tempMode:
									self.mode = 0

								elif tempMode:
									yawOffset = 0 - yaw
									self._serialResetYaw = True

							if abs(self.tempPose['trackpadX']) > 0.07 or abs(self.tempPose['trackpadY']) > 0.07:
								self.tempPose['trackpadTouch'] = 1

							else:
								self.tempPose['trackpadTouch'] = 0

							if self.tempPose['triggerValue'] > 0.9:
								self.tempPose['triggerClick'] = 1

							else:
								self.tempPose['triggerClick'] = 0

							time.sleep(self._serialListenDelay)

						except Exception as e:
							print (f'{self.serialListener2.__name__}: {e}')
							break

			except Exception as e:
				print (f'{self.serialListener2.__name__}: {e}')

			time.sleep(1)
		print (f'{self.serialListener2.__name__} stop')


	def serialListener(self):
		while self._retrySerial:
			try:
				yawOffset = 0
				with serial.Serial(self.serialPaths['green'], 115200, timeout=1/5) as ser2:
					startTime = time.time()

					while self._retrySerial:
						try:
							if 2 < time.time() - startTime < 4:
								ser2.write(b'nut\n')

							respG = ser2.readline()

							gg = u.decodeSerial(respG)

							if len(gg) > 0:
								ypr = gg

								w, x, y, z = sq.euler2quat(ypr[0] + yawOffset, ypr[1], ypr[2], degrees=True)

								# self.pose['rW'] = rrr2[0]
								# self.pose['rX'] = -rrr2[2]
								# self.pose['rY'] = rrr2[3]
								# self.pose['rZ'] = -rrr2[1]

								self.pose['rW'] = round(w, 4)
								self.pose['rX'] = round(y, 4)
								self.pose['rY'] = round(-x, 4)
								self.pose['rZ'] = round(-z, 4)

								if self._serialResetYaw:
									yawOffset = 0-ypr[0]

							time.sleep(self._serialListenDelay)

						except Exception as e:
							print (f'{self.serialListener.__name__}: {e}')
							break

			except Exception as e:
				print (f'{self.serialListener.__name__}: {e}')

			time.sleep(1)
		print (f'{self.serialListener.__name__} stop')

	async def recv(self):
		while self._recv:
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

		loop = asyncio.get_running_loop()

		# _ = await loop.run_in_executor(None, self.serialListener)


		await asyncio.gather(
				self.send(),
				self.recv(),
				# self.keyListener(),
				loop.run_in_executor(None, self.serialListener2),
				loop.run_in_executor(None, self.serialListener),
				self.getLocation(),
				self.close(),
			)

t = Poser('192.168.31.60')

asyncio.run(t.main())
import asyncio
import virtualreality.utilz as u
from virtualreality import templates as template

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
import serial.threaded
import squaternion as sq

class Poser(template.PoserTemplate):
	def __init__(self, *args, **kwargs):
		super().__init__(*args, **kwargs)

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

		self.serialPaths = {'green':'/dev/ttyUSB1', 'blue': '/dev/ttyUSB0'}

		self.mode = 0
		self._serialResetYaw = False

	@template.thread_register(1/90)
	async def modeSwitcher(self):
		while self.coro_keepAlive['modeSwitcher'][0]:
			try:
				if self.mode == 1:
					self.poseControllerR = self.tempPose

				else:
					self.poseControllerL = self.tempPose

				await asyncio.sleep(self.coro_keepAlive['modeSwitcher'][1])

			except Exception as e:
				print (f'failed modeSwitcher: {e}')
				self.coro_keepAlive['modeSwitcher'][0] = False
				break

	@template.thread_register(1/60)
	async def getLocation(self):
		try:
			self.t1 = u.Tracker()#offsets={'translation':[0, 0, 0], 'rotation':[0.6981317007977318, 0, 0]})

			self.t1.markerMasks = {
				'blue':{
					'h':(98, 10),
					's':(200, 55),
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
  
		if self.t1 is None:
			self.coro_keepAlive['getLocation'][0] = False

		while self.coro_keepAlive['getLocation'][0]:
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

				elapsed = time.time() - a

				slepDel = self.coro_keepAlive['getLocation'][1]
				slepDel = slepDel - elapsed if (slepDel - elapsed) > 0 else 0

				await asyncio.sleep(slepDel)

			except Exception as e:
				print ('stopping getLocation:', e)
				self.coro_keepAlive['getLocation'][0] = False
				break

		if self.t1 is not None:
			self.t1.close()

	@template.thread_register(1/100)
	async def serialListener2(self):
		pastVelocity = [0, 0, 0]
		velocityUntilReset = 0
		tempForOffz = {'x':0, 'y':0, 'z':0}
		while self.coro_keepAlive['serialListener2'][0]:
			try:
				yawOffset = 0
				with serial.Serial(self.serialPaths['green'], 115200, timeout=1/4) as ser:
					with serial.threaded.ReaderThread(ser, u.SerialReaderFactory) as protocol:
						for _ in range(10):
							protocol.write_line('nut')
							await asyncio.sleep(1)

						while self.coro_keepAlive['serialListener2'][0]:
							try:
								gg = u.decodeSerial(protocol.lastRead)

								if len(gg) > 0:
									y, p, r, az, ax, ay, trgr, grp, util, sys, menu, padClk, padY, padX = gg

									if velocityUntilReset < 20:

										u.rotateY({'':tempForOffz}, u.angle2rad(-yawOffset))
										pastVelocity = [tempForOffz['x'], tempForOffz['y'], tempForOffz['z']]
										tempForOffz['x'] = round(pastVelocity[0] - ax*self.coro_keepAlive['serialListener2'][1] * 0.005, 4)
										tempForOffz['y'] = round(pastVelocity[1] - ay*self.coro_keepAlive['serialListener2'][1] * 0.005, 4)
										tempForOffz['z'] = round(pastVelocity[2] - az*self.coro_keepAlive['serialListener2'][1] * 0.005, 4)
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

								await asyncio.sleep(self.coro_keepAlive['serialListener2'][1])

							except Exception as e:
								print (f'{self.serialListener2.__name__}: {e}')
								break

			except Exception as e:
				print (f'serialListener2: {e}')

			await asyncio.sleep(1)

	@template.thread_register(1/100)
	async def serialListener(self):
		while self.coro_keepAlive['serialListener'][0]:
			try:
				with serial.Serial(self.serialPaths['blue'], 115200, timeout=1/5) as ser:
					with serial.threaded.ReaderThread(ser, u.SerialReaderFactory) as protocol:
						yawOffset = 0
						for _ in range(10):
							protocol.write_line('nut')
							await asyncio.sleep(1)

						while self.coro_keepAlive['serialListener'][0]:
							try:
								gg = u.decodeSerial(protocol.lastRead)

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

								await asyncio.sleep(self.coro_keepAlive['serialListener'][1])

							except Exception as e:
								print (f'{self.serialListener.__name__}: {e}')
								break

			except Exception as e:
				print (f'serialListener: {e}')

			await asyncio.sleep(1)

t = Poser(addr='192.168.31.60')

asyncio.run(t.main())